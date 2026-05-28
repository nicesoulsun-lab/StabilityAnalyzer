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
    , m_lastScanId(-1)
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
    m_lastScanId = -1;
    m_accumulatedTransPoints.clear();
    m_accumulatedBackPoints.clear();
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
    if (channel != m_channel)
        return;

    if (rows.isEmpty())
        return;

    qDebug() << "[RealtimeCurveCtrl] onScanDataChunkReady"
             << "channel=" << channel << "m_channel=" << m_channel
             << "experimentId=" << experimentId
             << "scanId=" << scanId
             << "scanCompleted=" << scanCompleted
             << "rows=" << rows.size();

    m_currentExperimentId = experimentId;

    if (scanId != m_lastScanId && m_lastScanId >= 0) {
        qDebug() << "[RealtimeCurveCtrl] new scan detected, clearing old data"
                 << "oldScanId=" << m_lastScanId << "newScanId=" << scanId;
        m_accumulatedTransPoints.clear();
        m_accumulatedBackPoints.clear();
    }
    m_lastScanId = scanId;

    if (scanCompleted) {
        m_accumulatedTransPoints.clear();
        m_accumulatedBackPoints.clear();
        rebuildCurve(rows);
    } else {
        appendIncrementalData(rows);
    }
}

void RealtimeCurveCtrl::appendIncrementalData(const QVariantList &rows)
{
    for (const QVariant &v : rows) {
        const QVariantMap row = v.toMap();
        if (row.isEmpty())
            continue;

        const double heightMm = row.value(QStringLiteral("height")).toDouble() / 1000.0;
        const double bs = row.value(QStringLiteral("backscatter_intensity")).toDouble();
        const double tr = row.value(QStringLiteral("transmission_intensity")).toDouble();

        if (!qIsFinite(heightMm) || !qIsFinite(bs) || !qIsFinite(tr))
            continue;

        m_accumulatedTransPoints.append(QPointF(heightMm, tr));
        m_accumulatedBackPoints.append(QPointF(heightMm, bs));
    }

    flushIncrementalCurve();
}

void RealtimeCurveCtrl::flushIncrementalCurve()
{
    if (m_accumulatedTransPoints.size() < 2)
        return;

    QVector<QPointF> tSorted = m_accumulatedTransPoints;
    QVector<QPointF> bSorted = m_accumulatedBackPoints;

    std::sort(tSorted.begin(), tSorted.end(),
              [](const QPointF &a, const QPointF &b) { return a.x() < b.x(); });
    std::sort(bSorted.begin(), bSorted.end(),
              [](const QPointF &a, const QPointF &b) { return a.x() < b.x(); });

    double minH = tSorted.first().x();
    double maxH = tSorted.last().x();

    m_transmissionPoints = toVariantList(tSorted);
    m_backscatterPoints = toVariantList(bSorted);

    if (qIsFinite(minH) && qIsFinite(maxH)) {
        const double hPadding = std::max((maxH - minH) * 0.05, 1.0);
        m_minHeight = minH - hPadding;
        m_maxHeight = maxH + hPadding;
    }

    double minT = std::numeric_limits<double>::max();
    double maxT = std::numeric_limits<double>::lowest();
    double minB = std::numeric_limits<double>::max();
    double maxB = std::numeric_limits<double>::lowest();
    for (int i = 0; i < tSorted.size(); ++i) {
        if (tSorted[i].y() < minT) minT = tSorted[i].y();
        if (tSorted[i].y() > maxT) maxT = tSorted[i].y();
        if (bSorted[i].y() < minB) minB = bSorted[i].y();
        if (bSorted[i].y() > maxB) maxB = bSorted[i].y();
    }

    const double tPadding = std::max((maxT - minT) * 0.08, 1.0);
    const double bPadding = std::max((maxB - minB) * 0.08, 1.0);
    m_minTransmission = std::max(0.0, minT - tPadding);
    m_maxTransmission = maxT + tPadding;
    m_minBackscatter = std::max(0.0, minB - bPadding);
    m_maxBackscatter = maxB + bPadding;

    m_hasData = true;

    qDebug() << "[RealtimeCurveCtrl] incremental curve updated"
             << "channel=" << m_channel
             << "accumulated=" << m_accumulatedTransPoints.size()
             << "heightRange=" << m_minHeight << "~" << m_maxHeight;

    emit dataUpdated();
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

    m_lastScanId = latestScanId;

    const QVector<QVariantMap> dbRows = m_dataCtrl->getDataByExperimentAndScan(experimentId, latestScanId);
    qDebug() << "[RealtimeCurveCtrl] loadLatestScanData dbRows=" << dbRows.size()
             << "scanId=" << latestScanId;
    if (dbRows.isEmpty()) {
        qDebug() << "[RealtimeCurveCtrl] no data for scan" << latestScanId
                 << "in experiment" << experimentId;
        return;
    }

    QVariantList rowList;
    rowList.reserve(dbRows.size());
    for (const QVariantMap &row : dbRows) {
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
    if (!rows.isEmpty()) {
        m_accumulatedTransPoints.clear();
        m_accumulatedBackPoints.clear();

        appendIncrementalData(rows);
    }

    flushIncrementalCurve();
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
