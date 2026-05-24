#include "inc/Controller/realtime_curve_ctrl.h"
#include "inc/Controller/experiment_ctrl.h"
#include "inc/Controller/data_ctrl.h"

#include <QDebug>
#include <QPointF>
#include <QtMath>
#include <algorithm>
#include <limits>

RealtimeCurveCtrl::RealtimeCurveCtrl(ExperimentCtrl *experimentCtrl, dataCtrl *dataCtrl, QObject *parent)
    : QObject(parent)
    , m_experimentCtrl(experimentCtrl)
    , m_dataCtrl(dataCtrl)
    , m_channel(-1)
    , m_currentExperimentId(0)
    , m_minHeight(0)
    , m_maxHeight(55)
    , m_minTransmission(0)
    , m_maxTransmission(100)
    , m_minBackscatter(0)
    , m_maxBackscatter(100)
    , m_hasData(false)
{
    if (m_experimentCtrl) {
        connect(m_experimentCtrl, &ExperimentCtrl::scanDataChunkReady,
                this, &RealtimeCurveCtrl::onScanDataChunkReady);
        qDebug() << "[RealtimeCurveCtrl] created, connected to scanDataChunkReady";
    } else {
        qWarning() << "[RealtimeCurveCtrl] created BUT experimentCtrl is NULL!";
    }
}

RealtimeCurveCtrl::~RealtimeCurveCtrl()
{
}

int RealtimeCurveCtrl::channel() const
{
    return m_channel;
}

void RealtimeCurveCtrl::setChannel(int ch)
{
    if (m_channel == ch)
        return;

    m_channel = ch;
    m_currentExperimentId = 0;
    clearData();
    emit channelChanged();

    loadLatestScanData();
}

bool RealtimeCurveCtrl::hasData() const
{
    return m_hasData;
}

qreal RealtimeCurveCtrl::minHeight() const
{
    return m_minHeight;
}

qreal RealtimeCurveCtrl::maxHeight() const
{
    return m_maxHeight;
}

qreal RealtimeCurveCtrl::minTransmission() const
{
    return m_minTransmission;
}

qreal RealtimeCurveCtrl::maxTransmission() const
{
    return m_maxTransmission;
}

qreal RealtimeCurveCtrl::minBackscatter() const
{
    return m_minBackscatter;
}

qreal RealtimeCurveCtrl::maxBackscatter() const
{
    return m_maxBackscatter;
}

QVariantList RealtimeCurveCtrl::transmissionPoints() const
{
    return m_transmissionPoints;
}

QVariantList RealtimeCurveCtrl::backscatterPoints() const
{
    return m_backscatterPoints;
}

void RealtimeCurveCtrl::clearData()
{
    m_transmissionPoints.clear();
    m_backscatterPoints.clear();
    m_minHeight = 0;
    m_maxHeight = 55;
    m_minTransmission = 0;
    m_maxTransmission = 100;
    m_minBackscatter = 0;
    m_maxBackscatter = 100;
    m_hasData = false;
    emit dataUpdated();
}

void RealtimeCurveCtrl::onScanDataChunkReady(int channel, int experimentId, int scanId,
                                              bool scanCompleted, const QVariantList &rows)
{
    Q_UNUSED(scanId)

    qDebug() << "[RealtimeCurveCtrl] onScanDataChunkReady"
             << "channel=" << channel << "m_channel=" << m_channel
             << "experimentId=" << experimentId
             << "scanCompleted=" << scanCompleted
             << "rows=" << rows.size();

    if (channel != m_channel)
        return;

    if (!scanCompleted)
        return;

    if (rows.isEmpty())
        return;

    m_currentExperimentId = experimentId;
    rebuildCurve(rows);
}

void RealtimeCurveCtrl::loadLatestScanData()
{
    qDebug() << "[RealtimeCurveCtrl] loadLatestScanData channel=" << m_channel;

    if (!m_experimentCtrl || !m_dataCtrl) {
        qWarning() << "[RealtimeCurveCtrl] loadLatestScanData: null ctrl, experimentCtrl=" << m_experimentCtrl << "dataCtrl=" << m_dataCtrl;
        return;
    }

    if (m_channel < 0)
        return;

    const int experimentId = m_experimentCtrl->getCurrentExperimentId(m_channel);
    qDebug() << "[RealtimeCurveCtrl] loadLatestScanData experimentId=" << experimentId;
    if (experimentId <= 0) {
        qDebug() << "[RealtimeCurveCtrl] no running experiment for channel" << m_channel;
        return;
    }

    m_currentExperimentId = experimentId;

    const QVector<int> scanIds = m_dataCtrl->getScanIdsByExperiment(experimentId);
    qDebug() << "[RealtimeCurveCtrl] loadLatestScanData scanIds=" << scanIds.size();
    if (scanIds.isEmpty()) {
        qDebug() << "[RealtimeCurveCtrl] no scans yet for experiment" << experimentId;
        return;
    }

    const int latestScanId = scanIds.last();

    const QVector<QVariantMap> rows = m_dataCtrl->getDataByExperimentAndScan(experimentId, latestScanId);
    qDebug() << "[RealtimeCurveCtrl] loadLatestScanData rows=" << rows.size()
             << "scanId=" << latestScanId;
    if (rows.isEmpty()) {
        qDebug() << "[RealtimeCurveCtrl] no data for scan" << latestScanId
                 << "in experiment" << experimentId;
        return;
    }

    QVariantList rowList;
    rowList.reserve(rows.size());
    for (const QVariantMap &row : rows) {
        rowList.append(row);
    }

    qDebug() << "[RealtimeCurveCtrl] loaded latest scan data"
             << "channel=" << m_channel
             << "experimentId=" << experimentId
             << "scanId=" << latestScanId
             << "rows=" << rowList.size();

    rebuildCurve(rowList);
}

void RealtimeCurveCtrl::rebuildCurve(const QVariantList &rows)
{
    QVector<QPointF> tPoints, bPoints;
    tPoints.reserve(rows.size());
    bPoints.reserve(rows.size());

    double minH = std::numeric_limits<double>::max();
    double maxH = std::numeric_limits<double>::lowest();
    double minT = std::numeric_limits<double>::max();
    double maxT = std::numeric_limits<double>::lowest();
    double minB = std::numeric_limits<double>::max();
    double maxB = std::numeric_limits<double>::lowest();

    for (const QVariant &v : rows) {
        const QVariantMap row = v.toMap();
        if (row.isEmpty())
            continue;

        const double heightMm = row.value(QStringLiteral("height")).toDouble() / 1000.0;
        const double bs = row.value(QStringLiteral("backscatter_intensity")).toDouble();
        const double tr = row.value(QStringLiteral("transmission_intensity")).toDouble();

        if (!qIsFinite(heightMm) || !qIsFinite(bs) || !qIsFinite(tr))
            continue;

        tPoints.append(QPointF(heightMm, tr));
        bPoints.append(QPointF(heightMm, bs));

        minH = std::min(minH, heightMm);
        maxH = std::max(maxH, heightMm);
        minT = std::min(minT, tr);
        maxT = std::max(maxT, tr);
        minB = std::min(minB, bs);
        maxB = std::max(maxB, bs);
    }

    if (tPoints.size() < 2) {
        qDebug() << "[RealtimeCurveCtrl] curve data too few, skip"
                 << "channel=" << m_channel << "points=" << tPoints.size();
        return;
    }

    std::sort(tPoints.begin(), tPoints.end(),
              [](const QPointF &a, const QPointF &b) { return a.x() < b.x(); });
    std::sort(bPoints.begin(), bPoints.end(),
              [](const QPointF &a, const QPointF &b) { return a.x() < b.x(); });

    m_transmissionPoints = toVariantList(tPoints);
    m_backscatterPoints = toVariantList(bPoints);

    if (qIsFinite(minH) && qIsFinite(maxH)) {
        const double hPadding = std::max((maxH - minH) * 0.05, 1.0);
        m_minHeight = minH - hPadding;
        m_maxHeight = maxH + hPadding;
    }

    if (qIsFinite(minT) && qIsFinite(maxT)) {
        const double tPadding = std::max((maxT - minT) * 0.08, 1.0);
        m_minTransmission = std::max(0.0, minT - tPadding);
        m_maxTransmission = maxT + tPadding;
    }

    if (qIsFinite(minB) && qIsFinite(maxB)) {
        const double bPadding = std::max((maxB - minB) * 0.08, 1.0);
        m_minBackscatter = minB - bPadding;
        m_maxBackscatter = maxB + bPadding;
    }

    m_hasData = true;

    qDebug() << "[RealtimeCurveCtrl] curve updated"
             << "channel=" << m_channel
             << "points=" << tPoints.size()
             << "heightRange=" << m_minHeight << "~" << m_maxHeight
             << "transRange=" << m_minTransmission << "~" << m_maxTransmission
             << "backRange=" << m_minBackscatter << "~" << m_maxBackscatter;

    emit dataUpdated();
}

QVariantList RealtimeCurveCtrl::toVariantList(const QVector<QPointF> &points) const
{
    QVariantList result;
    result.reserve(points.size());
    for (const QPointF &point : points) {
        result.append(QVariant(point));
    }
    return result;
}
