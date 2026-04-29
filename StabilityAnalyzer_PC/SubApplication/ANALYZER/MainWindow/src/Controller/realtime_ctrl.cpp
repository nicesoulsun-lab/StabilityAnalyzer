#include "Controller/realtime_ctrl.h"

#include "Analysis/CurveChartAnalysisEngine.h"
#include "Analysis/LightCurveAnalysisEngine.h"
#include "datatransmitcontroller.h"

#include <QDebug>
#include <QPointF>
#include <QtMath>
#include <algorithm>
#include <limits>

namespace {

constexpr int kRealtimeCurvePointLimit = 512;
constexpr int kRealtimeCurveMaxScansPerExperiment = 20;
constexpr int kRealtimeCurveMaxExperimentCaches = 8;

struct RealtimeLightCurveRow {
    double heightMm = 0.0;
    double backscatter = 0.0;
    double transmission = 0.0;
};

QVariantList makePointList(const QVector<QPointF> &points)
{
    QVariantList result;
    result.reserve(points.size());
    for (const QPointF &point : points) {
        QVariantList pair;
        pair.reserve(2);
        pair.append(point.x());
        pair.append(point.y());
        result.append(QVariant(pair));
    }
    return result;
}

QVector<QPointF> extractPoints(const QVariantList &points)
{
    QVector<QPointF> result;
    result.reserve(points.size());

    for (const QVariant &value : points) {
        if (value.canConvert<QPointF>()) {
            result.append(value.toPointF());
            continue;
        }

        const QVariantList list = value.toList();
        if (list.size() >= 2) {
            bool xOk = false;
            bool yOk = false;
            const double xValue = list.at(0).toDouble(&xOk);
            const double yValue = list.at(1).toDouble(&yOk);
            if (xOk && yOk) {
                result.append(QPointF(xValue, yValue));
                continue;
            }
        }

        const QVariantMap map = value.toMap();
        if (!map.isEmpty()) {
            bool xOk = false;
            bool yOk = false;
            const double xValue = map.value(QStringLiteral("x"), map.value(QStringLiteral("timestamp"))).toDouble(&xOk);
            const double yValue = map.value(QStringLiteral("y"), map.value(QStringLiteral("value"))).toDouble(&yOk);
            if (xOk && yOk) {
                result.append(QPointF(xValue, yValue));
            }
        }
    }

    return result;
}

QVector<QPointF> filterPointsByHeight(const QVector<QPointF> &points, double lowerMm, double upperMm)
{
    const double safeLower = qMin(lowerMm, upperMm);
    const double safeUpper = qMax(lowerMm, upperMm);

    QVector<QPointF> filtered;
    filtered.reserve(points.size());
    for (const QPointF &point : points) {
        if (point.x() >= safeLower && point.x() <= safeUpper) {
            filtered.append(point);
        }
    }
    return filtered;
}

QVariantList downsampleRealtimeCurvePoints(const QVector<RealtimeLightCurveRow> &rows,
                                           bool useTransmission,
                                           int maxPoints)
{
    if (rows.isEmpty()) {
        return QVariantList();
    }

    if (maxPoints <= 2 || rows.size() <= maxPoints) {
        QVector<QPointF> points;
        points.reserve(rows.size());
        for (const RealtimeLightCurveRow &row : rows) {
            points.append(QPointF(row.heightMm, useTransmission ? row.transmission : row.backscatter));
        }
        return makePointList(points);
    }

    const int bucketCount = qMax(1, maxPoints / 2);
    QVector<QPointF> sampled;
    sampled.reserve(bucketCount * 2 + 2);
    sampled.append(QPointF(rows.first().heightMm, useTransmission ? rows.first().transmission : rows.first().backscatter));

    const int innerCount = rows.size() - 2;
    for (int bucket = 0; bucket < bucketCount; ++bucket) {
        const int startIndex = 1 + bucket * innerCount / bucketCount;
        const int endIndex = 1 + (bucket + 1) * innerCount / bucketCount;
        if (startIndex >= rows.size() - 1) {
            break;
        }

        const int safeEnd = qMax(startIndex + 1, qMin(endIndex, rows.size() - 1));
        int minIndex = startIndex;
        int maxIndex = startIndex;
        double minValue = useTransmission ? rows.at(startIndex).transmission : rows.at(startIndex).backscatter;
        double maxValue = minValue;

        for (int index = startIndex + 1; index < safeEnd; ++index) {
            const double value = useTransmission ? rows.at(index).transmission : rows.at(index).backscatter;
            if (value < minValue) {
                minValue = value;
                minIndex = index;
            }
            if (value > maxValue) {
                maxValue = value;
                maxIndex = index;
            }
        }

        if (minIndex <= maxIndex) {
            sampled.append(QPointF(rows.at(minIndex).heightMm, useTransmission ? rows.at(minIndex).transmission : rows.at(minIndex).backscatter));
            if (maxIndex != minIndex) {
                sampled.append(QPointF(rows.at(maxIndex).heightMm, useTransmission ? rows.at(maxIndex).transmission : rows.at(maxIndex).backscatter));
            }
        } else {
            sampled.append(QPointF(rows.at(maxIndex).heightMm, useTransmission ? rows.at(maxIndex).transmission : rows.at(maxIndex).backscatter));
            sampled.append(QPointF(rows.at(minIndex).heightMm, useTransmission ? rows.at(minIndex).transmission : rows.at(minIndex).backscatter));
        }
    }

    sampled.append(QPointF(rows.last().heightMm, useTransmission ? rows.last().transmission : rows.last().backscatter));
    std::sort(sampled.begin(), sampled.end(), [](const QPointF &left, const QPointF &right) {
        if (qFuzzyCompare(left.x(), right.x())) {
            return left.y() < right.y();
        }
        return left.x() < right.x();
    });
    return makePointList(sampled);
}

bool sanitizeCurvePoints(QVariantList *points)
{
    if (!points) {
        return false;
    }

    QVariantList sanitized;
    sanitized.reserve(points->size());
    const QVector<QPointF> extracted = extractPoints(*points);
    if (extracted.isEmpty()) {
        points->clear();
        return false;
    }

    for (const QPointF &point : extracted) {
        if (!qIsFinite(point.x()) || !qIsFinite(point.y())) {
            continue;
        }
        QVariantList pair;
        pair.reserve(2);
        pair.append(point.x());
        pair.append(point.y());
        sanitized.append(QVariant(pair));
    }

    *points = sanitized;
    return !points->isEmpty();
}

QVariantMap buildRealtimeLightCurve(int scanId, const QVariantList &rows)
{
    QVariantMap curve;
    if (scanId < 0 || rows.isEmpty()) {
        return curve;
    }

    QVector<RealtimeLightCurveRow> curveRows;
    curveRows.reserve(rows.size());

    int minTimestamp = std::numeric_limits<int>::max();
    int maxElapsedMs = 0;
    double minHeightMm = std::numeric_limits<double>::max();
    double maxHeightMm = std::numeric_limits<double>::lowest();
    double minBackscatter = std::numeric_limits<double>::max();
    double maxBackscatter = std::numeric_limits<double>::lowest();
    double minTransmission = std::numeric_limits<double>::max();
    double maxTransmission = std::numeric_limits<double>::lowest();

    for (const QVariant &rowVariant : rows) {
        const QVariantMap row = rowVariant.toMap();
        if (row.isEmpty()) {
            continue;
        }

        RealtimeLightCurveRow curveRow;
        curveRow.heightMm = row.value(QStringLiteral("height")).toDouble() / 1000.0;
        curveRow.backscatter = row.value(QStringLiteral("backscatter_intensity")).toDouble();
        curveRow.transmission = row.value(QStringLiteral("transmission_intensity")).toDouble();
        if (!qIsFinite(curveRow.heightMm) || !qIsFinite(curveRow.backscatter) || !qIsFinite(curveRow.transmission)) {
            continue;
        }

        curveRows.append(curveRow);
        minTimestamp = qMin(minTimestamp, row.value(QStringLiteral("timestamp"), 0).toInt());
        maxElapsedMs = qMax(maxElapsedMs, row.value(QStringLiteral("scan_elapsed_ms"), 0).toInt());
        minHeightMm = qMin(minHeightMm, curveRow.heightMm);
        maxHeightMm = qMax(maxHeightMm, curveRow.heightMm);
        minBackscatter = qMin(minBackscatter, curveRow.backscatter);
        maxBackscatter = qMax(maxBackscatter, curveRow.backscatter);
        minTransmission = qMin(minTransmission, curveRow.transmission);
        maxTransmission = qMax(maxTransmission, curveRow.transmission);
    }

    if (curveRows.size() < 2) {
        return QVariantMap();
    }

    std::sort(curveRows.begin(), curveRows.end(), [](const RealtimeLightCurveRow &left, const RealtimeLightCurveRow &right) {
        if (qFuzzyCompare(left.heightMm, right.heightMm)) {
            if (qFuzzyCompare(left.backscatter, right.backscatter)) {
                return left.transmission < right.transmission;
            }
            return left.backscatter < right.backscatter;
        }
        return left.heightMm < right.heightMm;
    });

    QVariantList backscatterPoints = downsampleRealtimeCurvePoints(curveRows, false, kRealtimeCurvePointLimit);
    QVariantList transmissionPoints = downsampleRealtimeCurvePoints(curveRows, true, kRealtimeCurvePointLimit);
    if (!sanitizeCurvePoints(&backscatterPoints) || !sanitizeCurvePoints(&transmissionPoints)) {
        return QVariantMap();
    }

    curve.insert(QStringLiteral("scan_id"), scanId);
    curve.insert(QStringLiteral("timestamp"), minTimestamp == std::numeric_limits<int>::max() ? 0 : minTimestamp);
    curve.insert(QStringLiteral("scan_elapsed_ms"), maxElapsedMs);
    curve.insert(QStringLiteral("point_count"), curveRows.size());
    curve.insert(QStringLiteral("min_height_mm"), minHeightMm);
    curve.insert(QStringLiteral("max_height_mm"), maxHeightMm);
    curve.insert(QStringLiteral("min_backscatter"), minBackscatter);
    curve.insert(QStringLiteral("max_backscatter"), maxBackscatter);
    curve.insert(QStringLiteral("min_transmission"), minTransmission);
    curve.insert(QStringLiteral("max_transmission"), maxTransmission);
    curve.insert(QStringLiteral("backscatter_points"), backscatterPoints);
    curve.insert(QStringLiteral("transmission_points"), transmissionPoints);
    return curve;
}

double averageValue(const QVector<QPointF> &points)
{
    if (points.isEmpty()) {
        return 0.0;
    }

    double sum = 0.0;
    for (const QPointF &point : points) {
        sum += point.y();
    }
    return sum / points.size();
}

double computeUniformityIndex(const QVector<QPointF> &points)
{
    if (points.isEmpty()) {
        return 0.0;
    }

    double sum = 0.0;
    double sumSquares = 0.0;
    for (const QPointF &point : points) {
        sum += point.y();
        sumSquares += point.y() * point.y();
    }

    const double average = sum / points.size();
    if (average <= 0.0) {
        return 0.0;
    }

    const double averageSquare = sumSquares / points.size();
    const double stdValue = qSqrt(qMax(0.0, averageSquare - average * average));
    return qBound(0.0, 1.0 - stdValue / average, 1.0);
}

double valueAtHeight(const QVector<QPointF> &points, double heightMm)
{
    if (points.isEmpty()) {
        return 0.0;
    }
    if (heightMm <= points.first().x()) {
        return points.first().y();
    }
    if (heightMm >= points.last().x()) {
        return points.last().y();
    }

    for (int index = 1; index < points.size(); ++index) {
        const QPointF &left = points.at(index - 1);
        const QPointF &right = points.at(index);
        if (heightMm > right.x()) {
            continue;
        }
        if (qFuzzyCompare(left.x(), right.x())) {
            return right.y();
        }

        const double ratio = (heightMm - left.x()) / (right.x() - left.x());
        return left.y() + (right.y() - left.y()) * ratio;
    }

    return points.last().y();
}

double computeInstabilityIntegral(const QVector<QPointF> &referencePoints,
                                  const QVector<QPointF> &currentPoints)
{
    if (referencePoints.size() < 2 || currentPoints.size() < 2) {
        return 0.0;
    }

    const double overlapStart = qMax(referencePoints.first().x(), currentPoints.first().x());
    const double overlapEnd = qMin(referencePoints.last().x(), currentPoints.last().x());
    if (overlapEnd <= overlapStart) {
        return 0.0;
    }

    QVector<double> sampleHeights;
    sampleHeights.reserve(referencePoints.size() + currentPoints.size() + 2);
    sampleHeights.append(overlapStart);
    for (const QPointF &point : referencePoints) {
        if (point.x() > overlapStart && point.x() < overlapEnd) {
            sampleHeights.append(point.x());
        }
    }
    for (const QPointF &point : currentPoints) {
        if (point.x() > overlapStart && point.x() < overlapEnd) {
            sampleHeights.append(point.x());
        }
    }
    sampleHeights.append(overlapEnd);

    std::sort(sampleHeights.begin(), sampleHeights.end());
    sampleHeights.erase(std::unique(sampleHeights.begin(), sampleHeights.end(), [](double left, double right) {
        return qAbs(left - right) < 1e-9;
    }), sampleHeights.end());

    double accumulated = 0.0;
    for (int index = 1; index < sampleHeights.size(); ++index) {
        const double x0 = sampleHeights.at(index - 1);
        const double x1 = sampleHeights.at(index);
        const double diff0 = qAbs(valueAtHeight(referencePoints, x0) - valueAtHeight(currentPoints, x0));
        const double diff1 = qAbs(valueAtHeight(referencePoints, x1) - valueAtHeight(currentPoints, x1));
        accumulated += 0.5 * (diff0 + diff1) * (x1 - x0);
    }

    const double totalHeight = overlapEnd - overlapStart;
    return totalHeight > 0.0 ? accumulated / totalHeight : 0.0;
}

QVector<QVariantMap> buildUniformityRows(const QVector<QVariantMap> &curves)
{
    QVector<QVariantMap> rows;
    rows.reserve(curves.size());

    for (const QVariantMap &curve : curves) {
        const QVector<QPointF> bsPoints = extractPoints(curve.value(QStringLiteral("backscatter_points")).toList());
        const QVector<QPointF> tPoints = extractPoints(curve.value(QStringLiteral("transmission_points")).toList());

        QVariantMap row;
        row.insert(QStringLiteral("scan_id"), curve.value(QStringLiteral("scan_id")));
        row.insert(QStringLiteral("scan_elapsed_ms"), curve.value(QStringLiteral("scan_elapsed_ms")));
        row.insert(QStringLiteral("ui_backscatter"), computeUniformityIndex(bsPoints));
        row.insert(QStringLiteral("ui_transmission"), computeUniformityIndex(tPoints));
        rows.append(row);
    }

    return rows;
}

QVector<QVariantMap> buildInstabilityRows(const QVector<QVariantMap> &curves,
                                          double lowerMm,
                                          double upperMm,
                                          const QString &segmentKey)
{
    QVector<QVariantMap> rows;
    if (curves.isEmpty()) {
        return rows;
    }

    QVector<QPointF> referenceBackscatter;
    QVector<QPointF> referenceTransmission;
    bool hasReference = false;

    const double safeLower = qMin(lowerMm, upperMm);
    const double safeUpper = qMax(lowerMm, upperMm);

    rows.reserve(curves.size());
    for (int index = 0; index < curves.size(); ++index) {
        const QVariantMap &curve = curves.at(index);
        const QVector<QPointF> filteredBackscatter = filterPointsByHeight(
            extractPoints(curve.value(QStringLiteral("backscatter_points")).toList()), safeLower, safeUpper);
        const QVector<QPointF> filteredTransmission = filterPointsByHeight(
            extractPoints(curve.value(QStringLiteral("transmission_points")).toList()), safeLower, safeUpper);

        if (filteredBackscatter.size() < 2 || filteredTransmission.size() < 2) {
            continue;
        }

        const bool useTransmission = averageValue(filteredTransmission) > 0.2;
        QVariantMap row;
        row.insert(QStringLiteral("segment_key"), segmentKey);
        row.insert(QStringLiteral("scan_id"), curve.value(QStringLiteral("scan_id")));
        row.insert(QStringLiteral("scan_elapsed_ms"), curve.value(QStringLiteral("scan_elapsed_ms")));

        if (!hasReference) {
            referenceBackscatter = filteredBackscatter;
            referenceTransmission = filteredTransmission;
            row.insert(QStringLiteral("channel_used"), QStringLiteral("T"));
            row.insert(QStringLiteral("instability_value"), 0.0);
            hasReference = true;
        } else {
            const double instabilityValue = useTransmission
                ? computeInstabilityIntegral(referenceTransmission, filteredTransmission)
                : computeInstabilityIntegral(referenceBackscatter, filteredBackscatter);
            row.insert(QStringLiteral("channel_used"), useTransmission ? QStringLiteral("T") : QStringLiteral("BS"));
            row.insert(QStringLiteral("instability_value"), instabilityValue);
        }
        rows.append(row);
    }

    return rows;
}

bool curveLessThan(const QVariantMap &left, const QVariantMap &right)
{
    const int leftElapsed = left.value(QStringLiteral("scan_elapsed_ms")).toInt();
    const int rightElapsed = right.value(QStringLiteral("scan_elapsed_ms")).toInt();
    if (leftElapsed != rightElapsed) {
        return leftElapsed < rightElapsed;
    }

    const int leftTimestamp = left.value(QStringLiteral("timestamp")).toInt();
    const int rightTimestamp = right.value(QStringLiteral("timestamp")).toInt();
    if (leftTimestamp != rightTimestamp) {
        return leftTimestamp < rightTimestamp;
    }

    return left.value(QStringLiteral("scan_id")).toInt() < right.value(QStringLiteral("scan_id")).toInt();
}

} // namespace

realtimeCtrl::realtimeCtrl(QObject *parent)
    : QObject(parent)
{
}

void realtimeCtrl::setDataTransmitController(DataTransmitController *controller)
{
    if (m_dataTransmitCtrl == controller) {
        return;
    }

    if (m_dataTransmitCtrl) {
        disconnect(m_dataTransmitCtrl, &DataTransmitController::streamMessageReceived,
                   this, &realtimeCtrl::handleStreamMessage);
    }

    m_dataTransmitCtrl = controller;
    if (m_dataTransmitCtrl) {
        connect(m_dataTransmitCtrl, &DataTransmitController::streamMessageReceived,
                this, &realtimeCtrl::handleStreamMessage);
    }

    qDebug() << "[realtimeCtrl] data transmit controller updated"
             << "available=" << (m_dataTransmitCtrl != nullptr);
}

void realtimeCtrl::setActiveSession(int channel, int experimentId, int tabIndex, bool active)
{
    const bool validChartTab = tabIndex >= 0 && tabIndex <= 2;
    const bool nextActive = active && channel >= 0 && experimentId > 0 && validChartTab;
    const bool changed = m_activeSession.active != nextActive
            || m_activeSession.channel != (nextActive ? channel : -1)
            || m_activeSession.experimentId != (nextActive ? experimentId : 0)
            || m_activeSession.tabIndex != (nextActive ? tabIndex : -1);

    if (!changed) {
        return;
    }

    m_activeSession.active = nextActive;
    m_activeSession.channel = nextActive ? channel : -1;
    m_activeSession.experimentId = nextActive ? experimentId : 0;
    m_activeSession.tabIndex = nextActive ? tabIndex : -1;
    ++m_activeSession.subscriptionId;

    qDebug() << (nextActive ? "[realtimeCtrl][subscribe]" : "[realtimeCtrl][unsubscribe]")
             << "channel=" << m_activeSession.channel
             << "experimentId=" << m_activeSession.experimentId
             << "tabIndex=" << m_activeSession.tabIndex
             << "subscriptionId=" << m_activeSession.subscriptionId;

    trimGlobalCache();
}

QVariantList realtimeCtrl::getLightCurves(int experimentId) const
{
    QVariantList result;
    const QVector<QVariantMap> curves = sortedCurves(experimentId);
    result.reserve(curves.size());
    for (const QVariantMap &curve : curves) {
        result.append(curve);
    }
    return result;
}

QVariantList realtimeCtrl::getProcessedLightIntensityCurves(int experimentId,
                                                            int referenceScanId,
                                                            double lowerMm,
                                                            double upperMm,
                                                            bool useReference) const
{
    const QVector<QVariantMap> curves = sortedCurves(experimentId);
    if (curves.isEmpty()) {
        return QVariantList();
    }

    const QVector<LightCurveAnalysisCurve> analysisCurves = LightCurveAnalysisEngine::fromVariantMaps(curves);
    const QVector<QVariantMap> processedCurves = LightCurveAnalysisEngine::processCurves(
        analysisCurves, referenceScanId, lowerMm, upperMm, useReference);

    QVariantList result;
    result.reserve(processedCurves.size());
    for (const QVariantMap &curve : processedCurves) {
        result.append(curve);
    }
    return result;
}

QVariantMap realtimeCtrl::getUniformityChartData(int experimentId) const
{
    return CurveChartAnalysisEngine::buildUniformityChartData(
        buildUniformityRows(sortedCurves(experimentId)));
}

QVariantMap realtimeCtrl::getInstabilitySeriesChartData(int experimentId,
                                                        double lowerMm,
                                                        double upperMm,
                                                        const QString &segmentKey,
                                                        const QString &title) const
{
    return CurveChartAnalysisEngine::buildInstabilitySeriesChartData(
        buildInstabilityRows(sortedCurves(experimentId), lowerMm, upperMm, segmentKey),
        title,
        lowerMm,
        upperMm);
}

QVariantMap realtimeCtrl::getInstabilityRadarChartData(int experimentId,
                                                       double minHeightMm,
                                                       double maxHeightMm) const
{
    const double safeMin = qMin(minHeightMm, maxHeightMm);
    const double safeMax = qMax(minHeightMm, maxHeightMm);
    const double sectionHeight = qMax((safeMax - safeMin) / 3.0, 0.0);
    const double firstSplit = safeMin + sectionHeight;
    const double secondSplit = safeMin + sectionHeight * 2.0;

    const QVariantMap overallSeries = getInstabilitySeriesChartData(
        experimentId, safeMin, safeMax, QStringLiteral("overall"), QStringLiteral("整体"));
    const QVariantMap bottomSeries = getInstabilitySeriesChartData(
        experimentId, safeMin, firstSplit, QStringLiteral("bottom"), QStringLiteral("底部"));
    const QVariantMap middleSeries = getInstabilitySeriesChartData(
        experimentId, firstSplit, secondSplit, QStringLiteral("middle"), QStringLiteral("中部"));
    const QVariantMap topSeries = getInstabilitySeriesChartData(
        experimentId, secondSplit, safeMax, QStringLiteral("top"), QStringLiteral("顶部"));
    return CurveChartAnalysisEngine::buildInstabilityRadarChartData(
        overallSeries, bottomSeries, middleSeries, topSeries);
}

void realtimeCtrl::clearExperimentSession(int experimentId)
{
    if (experimentId <= 0 || !m_curveCacheByExperiment.contains(experimentId)) {
        return;
    }

    const int channel = m_channelByExperiment.value(experimentId, -1);
    const int curveCount = m_curveCacheByExperiment.value(experimentId).size();
    m_curveCacheByExperiment.remove(experimentId);
    m_channelByExperiment.remove(experimentId);
    if (channel >= 0 && m_experimentByChannel.value(channel, 0) == experimentId) {
        m_experimentByChannel.remove(channel);
    }

    qDebug() << "[realtimeCtrl][clear]"
             << "channel=" << channel
             << "experimentId=" << experimentId
             << "curveCount=" << curveCount;

    emit experimentSessionCleared(channel, experimentId, QStringLiteral("clearExperimentSession"));
}

void realtimeCtrl::handleExperimentStopped(int channel, int experimentId)
{
    if (channel < 0 || experimentId <= 0) {
        return;
    }

    qDebug() << "[realtimeCtrl][stop]"
             << "channel=" << channel
             << "experimentId=" << experimentId;
    clearExperimentSession(experimentId);
}

void realtimeCtrl::handleStreamMessage(const QVariantMap &message)
{
    if (message.value(QStringLiteral("type")).toString() != QStringLiteral("experiment_scan_data")) {
        return;
    }

    const int experimentId = message.value(QStringLiteral("experiment_id")).toInt();
    const int channel = message.value(QStringLiteral("channel"), -1).toInt();
    const int scanId = message.value(QStringLiteral("scan_id")).toInt();
    const bool scanCompleted = message.value(QStringLiteral("scan_completed"), false).toBool();
    const QVariantList rows = message.value(QStringLiteral("rows")).toList();

    if (!shouldTrackMessage(channel, experimentId)) {
        return;
    }

    if (!scanCompleted) {
        return;
    }

    if (scanId < 0 || rows.isEmpty()) {
        qWarning() << "[realtimeCtrl][drop invalid stream]"
                   << "channel=" << channel
                   << "experimentId=" << experimentId
                   << "scanId=" << scanId
                   << "rowCount=" << rows.size();
        return;
    }

    const QVariantMap curve = buildRealtimeLightCurve(scanId, rows);
    if (curve.isEmpty()) {
        qWarning() << "[realtimeCtrl][drop invalid curve]"
                   << "channel=" << channel
                   << "experimentId=" << experimentId
                   << "scanId=" << scanId
                   << "rowCount=" << rows.size();
        return;
    }

    rememberChannelExperiment(channel, experimentId);
    m_curveCacheByExperiment[experimentId].insert(scanId, curve);
    m_channelByExperiment.insert(experimentId, channel);
    trimExperimentCache(experimentId);
    trimGlobalCache();

    qDebug() << "[realtimeCtrl][stream applied]"
             << "channel=" << channel
             << "experimentId=" << experimentId
             << "scanId=" << scanId
             << "curveCount=" << m_curveCacheByExperiment.value(experimentId).size()
             << "pointCount=" << curve.value(QStringLiteral("point_count")).toInt()
             << "subscriptionId=" << m_activeSession.subscriptionId;

    if (shouldNotifyActiveSession(channel, experimentId)) {
        emitLightCurvesChanged(channel, experimentId);
    }
}

bool realtimeCtrl::shouldTrackMessage(int channel, int experimentId) const
{
    return channel >= 0 && experimentId > 0;
}

bool realtimeCtrl::shouldNotifyActiveSession(int channel, int experimentId) const
{
    return m_activeSession.active
            && m_activeSession.channel == channel
            && m_activeSession.experimentId == experimentId
            && m_activeSession.tabIndex >= 0
            && m_activeSession.tabIndex <= 2;
}

QVector<QVariantMap> realtimeCtrl::sortedCurves(int experimentId) const
{
    QVector<QVariantMap> result;
    if (experimentId <= 0) {
        return result;
    }

    const QHash<int, QVariantMap> cache = m_curveCacheByExperiment.value(experimentId);
    result.reserve(cache.size());
    QHash<int, QVariantMap>::const_iterator it = cache.constBegin();
    for (; it != cache.constEnd(); ++it) {
        result.append(it.value());
    }

    std::sort(result.begin(), result.end(), curveLessThan);
    return result;
}

void realtimeCtrl::rememberChannelExperiment(int channel, int experimentId)
{
    if (channel < 0 || experimentId <= 0) {
        return;
    }

    const int previousExperimentId = m_experimentByChannel.value(channel, 0);
    if (previousExperimentId > 0 && previousExperimentId != experimentId) {
        qDebug() << "[realtimeCtrl][channel experiment switched]"
                 << "channel=" << channel
                 << "previousExperimentId=" << previousExperimentId
                 << "nextExperimentId=" << experimentId;
        clearExperimentSession(previousExperimentId);
    }

    m_experimentByChannel.insert(channel, experimentId);
}

void realtimeCtrl::trimExperimentCache(int experimentId)
{
    QHash<int, QVariantMap> &cache = m_curveCacheByExperiment[experimentId];
    QList<int> scanIds = cache.keys();
    std::sort(scanIds.begin(), scanIds.end());
    const int initialCount = scanIds.size();
    while (scanIds.size() > kRealtimeCurveMaxScansPerExperiment) {
        cache.remove(scanIds.takeFirst());
    }
    if (initialCount > kRealtimeCurveMaxScansPerExperiment) {
        qDebug() << "[realtimeCtrl][trim experiment cache]"
                 << "experimentId=" << experimentId
                 << "initialCurveCount=" << initialCount
                 << "retainedCurveCount=" << cache.size()
                 << "maxCurveCount=" << kRealtimeCurveMaxScansPerExperiment;
    }
}

void realtimeCtrl::trimGlobalCache()
{
    if (m_curveCacheByExperiment.size() <= kRealtimeCurveMaxExperimentCaches) {
        return;
    }

    QList<int> experimentIds = m_curveCacheByExperiment.keys();
    std::sort(experimentIds.begin(), experimentIds.end());
    for (int experimentId : experimentIds) {
        if (m_curveCacheByExperiment.size() <= kRealtimeCurveMaxExperimentCaches) {
            break;
        }
        if (m_activeSession.active && experimentId == m_activeSession.experimentId) {
            continue;
        }
        clearExperimentSession(experimentId);
    }
}

void realtimeCtrl::emitLightCurvesChanged(int channel, int experimentId)
{
    emit lightCurvesChanged(channel, experimentId, m_curveCacheByExperiment.value(experimentId).size());
}
