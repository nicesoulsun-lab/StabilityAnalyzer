#include "Controller/compare_ctrl.h"
#include "Analysis/CurveChartAnalysisEngine.h"

#include "../../../SqlOrm/inc/SqlOrmManager.h"

#include <QColor>
#include <QDebug>
#include <QPointer>
#include <QPointF>
#include <QtMath>

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace {

enum class CompareYRangeMode {
    FixedZeroOne,
    ZeroToMax,
    PaddedMinMax
};

struct CompareSeries {
    int experimentId = 0;
    QString label;
    QString color;
    QVector<QPointF> points;
};

struct HeightRange {
    double minMm = 0.0;
    double maxMm = 0.0;
};

double toNumber(const QVariant &value, double fallback = 0.0)
{
    bool ok = false;
    const double number = value.toDouble(&ok);
    return ok ? number : fallback;
}

QVector<int> sanitizeExperimentIds(const QVariantList &experimentIds)
{
    QVector<int> result;
    result.reserve(experimentIds.size());

    for (const QVariant &value : experimentIds) {
        const int experimentId = value.toInt();
        if (experimentId <= 0 || result.contains(experimentId)) {
            continue;
        }
        result.append(experimentId);
    }

    return result;
}

QVariantList toVariantIdList(const QVector<int> &experimentIds)
{
    QVariantList result;
    result.reserve(experimentIds.size());
    for (int experimentId : experimentIds) {
        result.append(experimentId);
    }
    return result;
}

QVariantList toPointList(const QVector<QPointF> &points)
{
    QVariantList result;
    result.reserve(points.size());
    for (const QPointF &point : points) {
        QVariantMap row;
        row.insert(QStringLiteral("x"), point.x());
        row.insert(QStringLiteral("y"), point.y());
        result.append(row);
    }
    return result;
}

QVector<QPointF> toPointVector(const QVariantList &points)
{
    QVector<QPointF> result;
    result.reserve(points.size());

    for (const QVariant &pointValue : points) {
        const QVariantMap point = pointValue.toMap();
        result.append(QPointF(toNumber(point.value(QStringLiteral("x"))),
                              toNumber(point.value(QStringLiteral("y")))));
    }

    return result;
}

QVector<QPointF> buildTimedPoints(const QVector<QVariantMap> &rows, const QString &yKey)
{
    QVector<QVariantMap> sortedRows = rows;
    std::sort(sortedRows.begin(), sortedRows.end(), [](const QVariantMap &left, const QVariantMap &right) {
        const double elapsedDiff = toNumber(left.value(QStringLiteral("scan_elapsed_ms")))
                                 - toNumber(right.value(QStringLiteral("scan_elapsed_ms")));
        if (!qFuzzyIsNull(elapsedDiff)) {
            return elapsedDiff < 0.0;
        }
        return toNumber(left.value(QStringLiteral("scan_id")))
             < toNumber(right.value(QStringLiteral("scan_id")));
    });

    QVector<QPointF> points;
    points.reserve(sortedRows.size());
    for (const QVariantMap &row : sortedRows) {
        const double xValue = toNumber(row.value(QStringLiteral("scan_elapsed_ms"))) / 60000.0;
        const double yValue = toNumber(row.value(yKey));
        points.append(QPointF(xValue, yValue));
    }
    return points;
}

QVariantList buildTimeTicks(double minValue, double maxValue, int desiredTickCount)
{
    QVariantList ticks;
    if (desiredTickCount <= 1 || maxValue <= minValue) {
        ticks.append(minValue);
        ticks.append(maxValue > minValue ? maxValue : minValue + 1.0);
        return ticks;
    }

    const double interval = (maxValue - minValue) / (desiredTickCount - 1);
    for (int i = 0; i < desiredTickCount; ++i) {
        ticks.append(minValue + interval * i);
    }
    return ticks;
}

QVariantList makeAxisLabels(double minValue, double maxValue, int desiredTickCount, int precision)
{
    QVariantList labels;
    if (desiredTickCount <= 1 || !qIsFinite(minValue) || !qIsFinite(maxValue)) {
        labels.append(QString::number(0.0, 'f', precision));
        labels.append(QString::number(1.0, 'f', precision));
        return labels;
    }

    if (qFuzzyCompare(minValue, maxValue)) {
        maxValue = minValue + 1.0;
    }

    const double interval = (maxValue - minValue) / (desiredTickCount - 1);
    for (int i = desiredTickCount - 1; i >= 0; --i) {
        labels.append(QString::number(minValue + interval * i, 'f', precision));
    }
    return labels;
}

double paddedMin(double minValue, double maxValue, double fallbackSpan)
{
    if (!qIsFinite(minValue)) {
        return 0.0;
    }

    const double span = qMax(qAbs(maxValue - minValue), fallbackSpan);
    return minValue - span * 0.08;
}

double paddedMax(double maxValue, double minValue, double fallbackSpan)
{
    if (!qIsFinite(maxValue)) {
        return 1.0;
    }

    const double span = qMax(qAbs(maxValue - minValue), fallbackSpan);
    return maxValue + span * 0.08;
}

HeightRange experimentHeightRange(const QVariantMap &experiment)
{
    const double startMm = toNumber(experiment.value(QStringLiteral("scan_range_start")), 0.0);
    const double endMm = toNumber(experiment.value(QStringLiteral("scan_range_end")), 55.0);
    const double stepUm = toNumber(experiment.value(QStringLiteral("scan_step")), 20.0);

    double effectiveEndMm = endMm;
    if (stepUm > 0.0 && endMm > startMm) {
        effectiveEndMm = endMm - stepUm / 1000.0;
    }

    HeightRange range;
    range.minMm = qMin(startMm, endMm);
    range.maxMm = qMax(startMm, effectiveEndMm);
    if (range.maxMm <= range.minMm) {
        range.maxMm = range.minMm + 10.0;
    }
    return range;
}

QString buildExperimentLabel(const QVariantMap &experiment, int experimentId)
{
    const QString projectName = experiment.value(QStringLiteral("project_name")).toString().trimmed();
    const QString sampleName = experiment.value(QStringLiteral("sample_name")).toString().trimmed();

    if (!projectName.isEmpty() && !sampleName.isEmpty()) {
        return projectName + QStringLiteral(" / ") + sampleName;
    }
    if (!sampleName.isEmpty()) {
        return sampleName;
    }
    if (!projectName.isEmpty()) {
        return projectName;
    }
    return QStringLiteral("实验 #%1").arg(experimentId);
}

QString compareSeriesColor(int index, int total)
{
    if (total <= 1) {
        return QStringLiteral("#2F7CF6");
    }

    const double ratio = static_cast<double>(index) / static_cast<double>(qMax(total - 1, 1));
    const QColor color = QColor::fromHslF(0.62 * (1.0 - ratio), 0.70, 0.50, 1.0);
    return color.name(QColor::HexRgb);
}

QString currentThreadTag()
{
    return QString::number(static_cast<qulonglong>(reinterpret_cast<quintptr>(QThread::currentThreadId())));
}

double firstElapsedMs(const QVector<QVariantMap> &rows)
{
    if (rows.isEmpty()) {
        return -1.0;
    }
    return toNumber(rows.first().value(QStringLiteral("scan_elapsed_ms")), -1.0);
}

double lastElapsedMs(const QVector<QVariantMap> &rows)
{
    if (rows.isEmpty()) {
        return -1.0;
    }
    return toNumber(rows.last().value(QStringLiteral("scan_elapsed_ms")), -1.0);
}

QVariantMap buildCompareChartData(const QVector<CompareSeries> &seriesList,
                                  CompareYRangeMode rangeMode,
                                  int yPrecision,
                                  const QString &title = QString(),
                                  const QString &rangeLabel = QString())
{
    QVariantMap payload;
    QVariantList serializedSeries;
    serializedSeries.reserve(seriesList.size());

    double minX = std::numeric_limits<double>::infinity();
    double maxX = -std::numeric_limits<double>::infinity();
    double minY = std::numeric_limits<double>::infinity();
    double maxY = -std::numeric_limits<double>::infinity();

    for (const CompareSeries &series : seriesList) {
        QVariantMap serialized;
        serialized.insert(QStringLiteral("experiment_id"), series.experimentId);
        serialized.insert(QStringLiteral("label"), series.label);
        serialized.insert(QStringLiteral("color"), series.color);
        serialized.insert(QStringLiteral("point_count"), series.points.size());
        serialized.insert(QStringLiteral("has_data"), !series.points.isEmpty());
        serialized.insert(QStringLiteral("points"), toPointList(series.points));
        serializedSeries.append(serialized);

        for (const QPointF &point : series.points) {
            minX = qMin(minX, point.x());
            maxX = qMax(maxX, point.x());
            minY = qMin(minY, point.y());
            maxY = qMax(maxY, point.y());
        }
    }

    if (!qIsFinite(minX) || !qIsFinite(maxX)) {
        minX = 0.0;
        maxX = 1.0;
    } else if (maxX <= minX) {
        maxX = minX + 1.0;
    }

    double chartMinY = 0.0;
    double chartMaxY = 1.0;
    switch (rangeMode) {
    case CompareYRangeMode::FixedZeroOne:
        chartMinY = 0.0;
        chartMaxY = 1.0;
        break;
    case CompareYRangeMode::ZeroToMax:
        chartMinY = 0.0;
        chartMaxY = qIsFinite(maxY) ? qMax(1.0, maxY * 1.12) : 1.0;
        break;
    case CompareYRangeMode::PaddedMinMax:
        if (!qIsFinite(minY) || !qIsFinite(maxY)) {
            chartMinY = 0.0;
            chartMaxY = 1.0;
        } else {
            chartMinY = paddedMin(minY, maxY, 1.0);
            chartMaxY = paddedMax(maxY, minY, 1.0);
        }
        break;
    }

    payload.insert(QStringLiteral("title"), title);
    payload.insert(QStringLiteral("rangeLabel"), rangeLabel);
    payload.insert(QStringLiteral("seriesList"), serializedSeries);
    payload.insert(QStringLiteral("chartMinX"), minX);
    payload.insert(QStringLiteral("chartMaxX"), maxX);
    payload.insert(QStringLiteral("chartMinY"), chartMinY);
    payload.insert(QStringLiteral("chartMaxY"), chartMaxY);
    payload.insert(QStringLiteral("xAxisTickValues"), buildTimeTicks(minX, maxX, 6));
    payload.insert(QStringLiteral("yAxisLabels"), makeAxisLabels(chartMinY, chartMaxY, 6, yPrecision));
    return payload;
}

QVariantMap buildUniformityPayload(SqlOrmManager *dbManager, const QVector<int> &experimentIds)
{
    QVector<CompareSeries> bsSeries;
    QVector<CompareSeries> tSeries;
    bsSeries.reserve(experimentIds.size());
    tSeries.reserve(experimentIds.size());

    for (int index = 0; index < experimentIds.size(); ++index) {
        const int experimentId = experimentIds.at(index);
        const QVariantMap experiment = dbManager->getExperimentById(experimentId);
        const QVector<QVariantMap> rawRows = dbManager->getUniformityIndicesByExperiment(experimentId);
        const QVariantMap chartData = CurveChartAnalysisEngine::buildUniformityChartData(rawRows);
        const QString label = buildExperimentLabel(experiment, experimentId);
        const QString color = compareSeriesColor(index, experimentIds.size());

        CompareSeries bsLine;
        bsLine.experimentId = experimentId;
        bsLine.label = label;
        bsLine.color = color;
        bsLine.points = toPointVector(chartData.value(QStringLiteral("bsPoints")).toList());
        bsSeries.append(bsLine);

        CompareSeries tLine = bsLine;
        tLine.points = toPointVector(chartData.value(QStringLiteral("tPoints")).toList());
        tSeries.append(tLine);

        qDebug() << "[compareCtrl][uniformity source]"
                 << "thread=" << currentThreadTag()
                 << "experimentId=" << experimentId
                 << "rawRows=" << rawRows.size()
                 << "firstElapsedMs=" << firstElapsedMs(rawRows)
                 << "lastElapsedMs=" << lastElapsedMs(rawRows);
        qDebug() << "[compareCtrl][uniformity series]"
                 << "experimentId=" << experimentId
                 << "label=" << label
                 << "bsPoints=" << bsLine.points.size()
                 << "tPoints=" << tLine.points.size();
    }

    QVariantMap payload;
    payload.insert(QStringLiteral("bsChart"),
                   buildCompareChartData(bsSeries,
                                         CompareYRangeMode::FixedZeroOne,
                                         2,
                                         QStringLiteral("背射光")));
    payload.insert(QStringLiteral("tChart"),
                   buildCompareChartData(tSeries,
                                         CompareYRangeMode::FixedZeroOne,
                                         2,
                                         QStringLiteral("透射光")));
    return payload;
}

QVariantMap buildLightIntensityAveragePayload(SqlOrmManager *dbManager, const QVector<int> &experimentIds)
{
    QVector<CompareSeries> bsSeries;
    QVector<CompareSeries> tSeries;
    bsSeries.reserve(experimentIds.size());
    tSeries.reserve(experimentIds.size());

    for (int index = 0; index < experimentIds.size(); ++index) {
        const int experimentId = experimentIds.at(index);
        const QVariantMap experiment = dbManager->getExperimentById(experimentId);
        const QVector<QVariantMap> rawRows = dbManager->getLightIntensityAveragesByExperiment(experimentId);
        const QVariantMap chartData = CurveChartAnalysisEngine::buildLightIntensityAverageChartData(rawRows);
        const QString label = buildExperimentLabel(experiment, experimentId);
        const QString color = compareSeriesColor(index, experimentIds.size());

        CompareSeries bsLine;
        bsLine.experimentId = experimentId;
        bsLine.label = label;
        bsLine.color = color;
        bsLine.points = toPointVector(chartData.value(QStringLiteral("bsPoints")).toList());
        bsSeries.append(bsLine);

        CompareSeries tLine = bsLine;
        tLine.points = toPointVector(chartData.value(QStringLiteral("tPoints")).toList());
        tSeries.append(tLine);

        qDebug() << "[compareCtrl][average source]"
                 << "thread=" << currentThreadTag()
                 << "experimentId=" << experimentId
                 << "rawRows=" << rawRows.size()
                 << "firstElapsedMs=" << firstElapsedMs(rawRows)
                 << "lastElapsedMs=" << lastElapsedMs(rawRows);
        qDebug() << "[compareCtrl][average series]"
                 << "experimentId=" << experimentId
                 << "label=" << label
                 << "bsPoints=" << bsLine.points.size()
                 << "tPoints=" << tLine.points.size();
    }

    QVariantMap payload;
    payload.insert(QStringLiteral("bsChart"),
                   buildCompareChartData(bsSeries,
                                         CompareYRangeMode::PaddedMinMax,
                                         1,
                                         QStringLiteral("背射光")));
    payload.insert(QStringLiteral("tChart"),
                   buildCompareChartData(tSeries,
                                         CompareYRangeMode::PaddedMinMax,
                                         1,
                                         QStringLiteral("透射光")));
    return payload;
}

QVariantMap buildInstabilityOverviewPayload(SqlOrmManager *dbManager, const QVector<int> &experimentIds)
{
    QVector<CompareSeries> overallSeries;
    QVector<CompareSeries> bottomSeries;
    QVector<CompareSeries> middleSeries;
    QVector<CompareSeries> topSeries;

    overallSeries.reserve(experimentIds.size());
    bottomSeries.reserve(experimentIds.size());
    middleSeries.reserve(experimentIds.size());
    topSeries.reserve(experimentIds.size());

    for (int index = 0; index < experimentIds.size(); ++index) {
        const int experimentId = experimentIds.at(index);
        const QVariantMap experiment = dbManager->getExperimentById(experimentId);
        const HeightRange range = experimentHeightRange(experiment);
        const double sectionHeight = qMax((range.maxMm - range.minMm) / 3.0, 0.0);
        const double firstSplit = range.minMm + sectionHeight;
        const double secondSplit = range.minMm + sectionHeight * 2.0;

        const QString label = buildExperimentLabel(experiment, experimentId);
        const QString color = compareSeriesColor(index, experimentIds.size());

        CompareSeries overallLine;
        overallLine.experimentId = experimentId;
        overallLine.label = label;
        overallLine.color = color;
        overallLine.points = toPointVector(
                    CurveChartAnalysisEngine::buildInstabilitySeriesChartData(
                        dbManager->getOrComputeInstabilityCurveDataByExperiment(experimentId),
                        QStringLiteral("整体"),
                        range.minMm,
                        range.maxMm)
                    .value(QStringLiteral("points")).toList());
        overallSeries.append(overallLine);

        CompareSeries bottomLine = overallLine;
        bottomLine.points = toPointVector(
                    CurveChartAnalysisEngine::buildInstabilitySeriesChartData(
                        dbManager->getOrComputeInstabilityCurveDataByHeightRange(
                            experimentId, range.minMm, firstSplit, QStringLiteral("bottom")),
                        QStringLiteral("底部"),
                        range.minMm,
                        firstSplit)
                    .value(QStringLiteral("points")).toList());
        bottomSeries.append(bottomLine);

        CompareSeries middleLine = overallLine;
        middleLine.points = toPointVector(
                    CurveChartAnalysisEngine::buildInstabilitySeriesChartData(
                        dbManager->getOrComputeInstabilityCurveDataByHeightRange(
                            experimentId, firstSplit, secondSplit, QStringLiteral("middle")),
                        QStringLiteral("中部"),
                        firstSplit,
                        secondSplit)
                    .value(QStringLiteral("points")).toList());
        middleSeries.append(middleLine);

        CompareSeries topLine = overallLine;
        topLine.points = toPointVector(
                    CurveChartAnalysisEngine::buildInstabilitySeriesChartData(
                        dbManager->getOrComputeInstabilityCurveDataByHeightRange(
                            experimentId, secondSplit, range.maxMm, QStringLiteral("top")),
                        QStringLiteral("顶部"),
                        secondSplit,
                        range.maxMm)
                    .value(QStringLiteral("points")).toList());
        topSeries.append(topLine);

        qDebug() << "[compareCtrl][instability overview series]"
                 << "experimentId=" << experimentId
                 << "label=" << label
                 << "overallPoints=" << overallLine.points.size()
                 << "bottomPoints=" << bottomLine.points.size()
                 << "middlePoints=" << middleLine.points.size()
                 << "topPoints=" << topLine.points.size();
    }

    QVariantMap payload;
    payload.insert(QStringLiteral("overallChart"),
                   buildCompareChartData(overallSeries,
                                         CompareYRangeMode::ZeroToMax,
                                         1,
                                         QStringLiteral("整体")));
    payload.insert(QStringLiteral("bottomChart"),
                   buildCompareChartData(bottomSeries,
                                         CompareYRangeMode::ZeroToMax,
                                         1,
                                         QStringLiteral("底部")));
    payload.insert(QStringLiteral("middleChart"),
                   buildCompareChartData(middleSeries,
                                         CompareYRangeMode::ZeroToMax,
                                         1,
                                         QStringLiteral("中部")));
    payload.insert(QStringLiteral("topChart"),
                   buildCompareChartData(topSeries,
                                         CompareYRangeMode::ZeroToMax,
                                         1,
                                         QStringLiteral("顶部")));
    return payload;
}

QVariantMap buildInstabilityCustomPayload(SqlOrmManager *dbManager,
                                          const QVector<int> &experimentIds,
                                          double lowerMm,
                                          double upperMm)
{
    const double safeLower = qMin(lowerMm, upperMm);
    const double safeUpper = qMax(lowerMm, upperMm);

    QVector<CompareSeries> seriesList;
    seriesList.reserve(experimentIds.size());

    for (int index = 0; index < experimentIds.size(); ++index) {
        const int experimentId = experimentIds.at(index);
        const QVariantMap experiment = dbManager->getExperimentById(experimentId);

        CompareSeries line;
        line.experimentId = experimentId;
        line.label = buildExperimentLabel(experiment, experimentId);
        line.color = compareSeriesColor(index, experimentIds.size());
        line.points = toPointVector(
                    CurveChartAnalysisEngine::buildInstabilitySeriesChartData(
                        dbManager->getOrComputeInstabilityCurveDataByHeightRange(
                            experimentId, safeLower, safeUpper, QStringLiteral("compare_custom")),
                        QStringLiteral("自定义"),
                        safeLower,
                        safeUpper)
                    .value(QStringLiteral("points")).toList());
        seriesList.append(line);

        qDebug() << "[compareCtrl][instability custom series]"
                 << "experimentId=" << experimentId
                 << "label=" << line.label
                 << "pointCount=" << line.points.size()
                 << "lowerMm=" << safeLower
                 << "upperMm=" << safeUpper;
    }

    QVariantMap payload;
    payload.insert(QStringLiteral("chart"),
                   buildCompareChartData(seriesList,
                                         CompareYRangeMode::ZeroToMax,
                                         1,
                                         QStringLiteral("自定义"),
                                         QStringLiteral("%1 - %2 mm")
                                             .arg(QString::number(safeLower, 'f', 1))
                                             .arg(QString::number(safeUpper, 'f', 1))));
    payload.insert(QStringLiteral("lowerMm"), safeLower);
    payload.insert(QStringLiteral("upperMm"), safeUpper);
    return payload;
}

} // namespace

compareCtrl::compareCtrl(QObject *parent)
    : QObject(parent)
    , m_dbManager(SqlOrmManager::instance())
{
}

compareCtrl::~compareCtrl()
{
    cancelInstabilityCompareRequest();
    cancelUniformityCompareRequest();
    cancelLightIntensityAverageCompareRequest();

    if (m_instabilityOverviewThread) {
        m_instabilityOverviewThread->requestInterruption();
        m_instabilityOverviewThread->wait(3000);
    }
    if (m_instabilityCustomThread) {
        m_instabilityCustomThread->requestInterruption();
        m_instabilityCustomThread->wait(3000);
    }
    if (m_uniformityCompareThread) {
        m_uniformityCompareThread->requestInterruption();
        m_uniformityCompareThread->wait(3000);
    }
    if (m_lightIntensityAverageCompareThread) {
        m_lightIntensityAverageCompareThread->requestInterruption();
        m_lightIntensityAverageCompareThread->wait(3000);
    }
}

int compareCtrl::requestInstabilityCompareOverview(const QVariantList &experimentIds)
{
    const QVector<int> sanitizedIds = sanitizeExperimentIds(experimentIds);
    if (sanitizedIds.size() < 2 || !m_dbManager) {
        qWarning() << "[compareCtrl][instability overview request] invalid request"
                   << "experimentCount=" << sanitizedIds.size();
        return 0;
    }

    cancelInstabilityCompareRequest(m_activeInstabilityOverviewRequestId);

    const int requestId = ++m_requestSerial;
    const QVariantList requestExperimentIds = toVariantIdList(sanitizedIds);
    m_activeInstabilityOverviewRequestId = requestId;
    m_activeInstabilityOverviewExperimentIds = requestExperimentIds;

    qDebug() << "[compareCtrl][instability overview start]"
             << "requestId=" << requestId
             << "experimentIds=" << requestExperimentIds;
    emit instabilityCompareOverviewRequestStarted(requestId, requestExperimentIds);

    QPointer<compareCtrl> self(this);
    QThread *workerThread = QThread::create([self, requestId, sanitizedIds, requestExperimentIds]() {
        if (!self) {
            return;
        }

        qDebug() << "[compareCtrl][instability overview worker entered]"
                 << "requestId=" << requestId
                 << "thread=" << currentThreadTag()
                 << "experimentCount=" << sanitizedIds.size();

        QVariantMap payload;
        QString errorMessage;
        bool cancelled = false;

        try {
            if (QThread::currentThread()->isInterruptionRequested()) {
                cancelled = true;
            } else {
                payload = buildInstabilityOverviewPayload(self->m_dbManager, sanitizedIds);
                qDebug() << "[compareCtrl][instability overview worker built]"
                         << "requestId=" << requestId
                         << "thread=" << currentThreadTag()
                         << "overallSeries="
                         << payload.value(QStringLiteral("overallChart")).toMap().value(QStringLiteral("seriesList")).toList().size();
            }
        } catch (const std::bad_alloc &) {
            errorMessage = QStringLiteral("多实验不稳定性对比加载内存不足");
        } catch (const std::exception &error) {
            errorMessage = QString::fromLocal8Bit(error.what());
        } catch (...) {
            errorMessage = QStringLiteral("多实验不稳定性对比加载失败");
        }

        QMetaObject::invokeMethod(self, [self, requestId, requestExperimentIds, payload, errorMessage, cancelled]() {
            if (self) {
                self->finishInstabilityOverviewRequest(requestId,
                                                       requestExperimentIds,
                                                       payload,
                                                       errorMessage,
                                                       cancelled);
            }
        }, Qt::QueuedConnection);
    });

    m_instabilityOverviewThread = workerThread;
    connect(workerThread, &QThread::finished, workerThread, &QObject::deleteLater);
    workerThread->start();
    return requestId;
}

int compareCtrl::requestInstabilityCompareCustom(const QVariantList &experimentIds, double lowerMm, double upperMm)
{
    const QVector<int> sanitizedIds = sanitizeExperimentIds(experimentIds);
    const double safeLower = qMin(lowerMm, upperMm);
    const double safeUpper = qMax(lowerMm, upperMm);
    if (sanitizedIds.size() < 2 || !m_dbManager || safeUpper - safeLower <= 1e-6) {
        qWarning() << "[compareCtrl][instability custom request] invalid request"
                   << "experimentCount=" << sanitizedIds.size()
                   << "lowerMm=" << safeLower
                   << "upperMm=" << safeUpper;
        return 0;
    }

    cancelInstabilityCompareRequest(m_activeInstabilityCustomRequestId);

    const int requestId = ++m_requestSerial;
    const QVariantList requestExperimentIds = toVariantIdList(sanitizedIds);
    m_activeInstabilityCustomRequestId = requestId;
    m_activeInstabilityCustomExperimentIds = requestExperimentIds;
    m_activeInstabilityCustomLowerMm = safeLower;
    m_activeInstabilityCustomUpperMm = safeUpper;

    qDebug() << "[compareCtrl][instability custom start]"
             << "requestId=" << requestId
             << "experimentIds=" << requestExperimentIds
             << "lowerMm=" << safeLower
             << "upperMm=" << safeUpper;
    emit instabilityCompareCustomRequestStarted(requestId,
                                                requestExperimentIds,
                                                safeLower,
                                                safeUpper);

    QPointer<compareCtrl> self(this);
    QThread *workerThread = QThread::create([self, requestId, sanitizedIds, requestExperimentIds, safeLower, safeUpper]() {
        if (!self) {
            return;
        }

        qDebug() << "[compareCtrl][instability custom worker entered]"
                 << "requestId=" << requestId
                 << "thread=" << currentThreadTag()
                 << "experimentCount=" << sanitizedIds.size()
                 << "lowerMm=" << safeLower
                 << "upperMm=" << safeUpper;

        QVariantMap payload;
        QString errorMessage;
        bool cancelled = false;

        try {
            if (QThread::currentThread()->isInterruptionRequested()) {
                cancelled = true;
            } else {
                payload = buildInstabilityCustomPayload(self->m_dbManager,
                                                        sanitizedIds,
                                                        safeLower,
                                                        safeUpper);
                qDebug() << "[compareCtrl][instability custom worker built]"
                         << "requestId=" << requestId
                         << "thread=" << currentThreadTag()
                         << "series="
                         << payload.value(QStringLiteral("chart")).toMap().value(QStringLiteral("seriesList")).toList().size();
            }
        } catch (const std::bad_alloc &) {
            errorMessage = QStringLiteral("多实验自定义不稳定性对比加载内存不足");
        } catch (const std::exception &error) {
            errorMessage = QString::fromLocal8Bit(error.what());
        } catch (...) {
            errorMessage = QStringLiteral("多实验自定义不稳定性对比加载失败");
        }

        QMetaObject::invokeMethod(self, [self, requestId, requestExperimentIds, safeLower, safeUpper, payload, errorMessage, cancelled]() {
            if (self) {
                self->finishInstabilityCustomRequest(requestId,
                                                     requestExperimentIds,
                                                     safeLower,
                                                     safeUpper,
                                                     payload,
                                                     errorMessage,
                                                     cancelled);
            }
        }, Qt::QueuedConnection);
    });

    m_instabilityCustomThread = workerThread;
    connect(workerThread, &QThread::finished, workerThread, &QObject::deleteLater);
    workerThread->start();
    return requestId;
}

void compareCtrl::cancelInstabilityCompareRequest(int requestId)
{
    if (m_instabilityOverviewThread
            && (requestId == 0 || requestId == m_activeInstabilityOverviewRequestId)) {
        qDebug() << "[compareCtrl][instability overview cancel]"
                 << "requestId=" << m_activeInstabilityOverviewRequestId
                 << "experimentIds=" << m_activeInstabilityOverviewExperimentIds;
        m_instabilityOverviewThread->requestInterruption();
        m_activeInstabilityOverviewRequestId = 0;
        m_activeInstabilityOverviewExperimentIds.clear();
    }

    if (m_instabilityCustomThread
            && (requestId == 0 || requestId == m_activeInstabilityCustomRequestId)) {
        qDebug() << "[compareCtrl][instability custom cancel]"
                 << "requestId=" << m_activeInstabilityCustomRequestId
                 << "experimentIds=" << m_activeInstabilityCustomExperimentIds
                 << "lowerMm=" << m_activeInstabilityCustomLowerMm
                 << "upperMm=" << m_activeInstabilityCustomUpperMm;
        m_instabilityCustomThread->requestInterruption();
        m_activeInstabilityCustomRequestId = 0;
        m_activeInstabilityCustomExperimentIds.clear();
        m_activeInstabilityCustomLowerMm = 0.0;
        m_activeInstabilityCustomUpperMm = 0.0;
    }
}

int compareCtrl::requestUniformityCompare(const QVariantList &experimentIds)
{
    const QVector<int> sanitizedIds = sanitizeExperimentIds(experimentIds);
    if (sanitizedIds.size() < 2 || !m_dbManager) {
        qWarning() << "[compareCtrl][uniformity request] invalid request"
                   << "experimentCount=" << sanitizedIds.size();
        return 0;
    }

    cancelUniformityCompareRequest();

    const int requestId = ++m_requestSerial;
    const QVariantList requestExperimentIds = toVariantIdList(sanitizedIds);
    m_activeUniformityCompareRequestId = requestId;
    m_activeUniformityCompareExperimentIds = requestExperimentIds;

    qDebug() << "[compareCtrl][uniformity start]"
             << "requestId=" << requestId
             << "experimentIds=" << requestExperimentIds;
    emit uniformityCompareRequestStarted(requestId, requestExperimentIds);

    QPointer<compareCtrl> self(this);
    QThread *workerThread = QThread::create([self, requestId, sanitizedIds, requestExperimentIds]() {
        if (!self) {
            return;
        }

        qDebug() << "[compareCtrl][uniformity worker entered]"
                 << "requestId=" << requestId
                 << "thread=" << currentThreadTag()
                 << "experimentCount=" << sanitizedIds.size();

        QVariantMap payload;
        QString errorMessage;
        bool cancelled = false;

        try {
            if (QThread::currentThread()->isInterruptionRequested()) {
                cancelled = true;
            } else {
                payload = buildUniformityPayload(self->m_dbManager, sanitizedIds);
                qDebug() << "[compareCtrl][uniformity worker built]"
                         << "requestId=" << requestId
                         << "thread=" << currentThreadTag()
                         << "bsSeries="
                         << payload.value(QStringLiteral("bsChart")).toMap().value(QStringLiteral("seriesList")).toList().size()
                         << "tSeries="
                         << payload.value(QStringLiteral("tChart")).toMap().value(QStringLiteral("seriesList")).toList().size();
            }
        } catch (const std::bad_alloc &) {
            errorMessage = QStringLiteral("多实验均匀度对比加载内存不足");
        } catch (const std::exception &error) {
            errorMessage = QString::fromLocal8Bit(error.what());
        } catch (...) {
            errorMessage = QStringLiteral("多实验均匀度对比加载失败");
        }

        QMetaObject::invokeMethod(self, [self, requestId, requestExperimentIds, payload, errorMessage, cancelled]() {
            if (self) {
                self->finishUniformityCompareRequest(requestId,
                                                     requestExperimentIds,
                                                     payload,
                                                     errorMessage,
                                                     cancelled);
            }
        }, Qt::QueuedConnection);
    });

    m_uniformityCompareThread = workerThread;
    connect(workerThread, &QThread::finished, workerThread, &QObject::deleteLater);
    workerThread->start();
    return requestId;
}

void compareCtrl::cancelUniformityCompareRequest(int requestId)
{
    if (!m_uniformityCompareThread) {
        if (requestId == 0 || requestId == m_activeUniformityCompareRequestId) {
            m_activeUniformityCompareRequestId = 0;
            m_activeUniformityCompareExperimentIds.clear();
        }
        return;
    }

    if (requestId > 0 && requestId != m_activeUniformityCompareRequestId) {
        return;
    }

    qDebug() << "[compareCtrl][uniformity cancel]"
             << "requestId=" << m_activeUniformityCompareRequestId
             << "experimentIds=" << m_activeUniformityCompareExperimentIds;
    m_uniformityCompareThread->requestInterruption();
    m_activeUniformityCompareRequestId = 0;
    m_activeUniformityCompareExperimentIds.clear();
}

int compareCtrl::requestLightIntensityAverageCompare(const QVariantList &experimentIds)
{
    const QVector<int> sanitizedIds = sanitizeExperimentIds(experimentIds);
    if (sanitizedIds.size() < 2 || !m_dbManager) {
        qWarning() << "[compareCtrl][average request] invalid request"
                   << "experimentCount=" << sanitizedIds.size();
        return 0;
    }

    cancelLightIntensityAverageCompareRequest();

    const int requestId = ++m_requestSerial;
    const QVariantList requestExperimentIds = toVariantIdList(sanitizedIds);
    m_activeLightIntensityAverageCompareRequestId = requestId;
    m_activeLightIntensityAverageCompareExperimentIds = requestExperimentIds;

    qDebug() << "[compareCtrl][average start]"
             << "requestId=" << requestId
             << "experimentIds=" << requestExperimentIds;
    emit lightIntensityAverageCompareRequestStarted(requestId, requestExperimentIds);

    QPointer<compareCtrl> self(this);
    QThread *workerThread = QThread::create([self, requestId, sanitizedIds, requestExperimentIds]() {
        if (!self) {
            return;
        }

        qDebug() << "[compareCtrl][average worker entered]"
                 << "requestId=" << requestId
                 << "thread=" << currentThreadTag()
                 << "experimentCount=" << sanitizedIds.size();

        QVariantMap payload;
        QString errorMessage;
        bool cancelled = false;

        try {
            if (QThread::currentThread()->isInterruptionRequested()) {
                cancelled = true;
            } else {
                payload = buildLightIntensityAveragePayload(self->m_dbManager, sanitizedIds);
                qDebug() << "[compareCtrl][average worker built]"
                         << "requestId=" << requestId
                         << "thread=" << currentThreadTag()
                         << "bsSeries="
                         << payload.value(QStringLiteral("bsChart")).toMap().value(QStringLiteral("seriesList")).toList().size()
                         << "tSeries="
                         << payload.value(QStringLiteral("tChart")).toMap().value(QStringLiteral("seriesList")).toList().size();
            }
        } catch (const std::bad_alloc &) {
            errorMessage = QStringLiteral("多实验光强平均值对比加载内存不足");
        } catch (const std::exception &error) {
            errorMessage = QString::fromLocal8Bit(error.what());
        } catch (...) {
            errorMessage = QStringLiteral("多实验光强平均值对比加载失败");
        }

        QMetaObject::invokeMethod(self, [self, requestId, requestExperimentIds, payload, errorMessage, cancelled]() {
            if (self) {
                self->finishLightIntensityAverageCompareRequest(requestId,
                                                                requestExperimentIds,
                                                                payload,
                                                                errorMessage,
                                                                cancelled);
            }
        }, Qt::QueuedConnection);
    });

    m_lightIntensityAverageCompareThread = workerThread;
    connect(workerThread, &QThread::finished, workerThread, &QObject::deleteLater);
    workerThread->start();
    return requestId;
}

void compareCtrl::cancelLightIntensityAverageCompareRequest(int requestId)
{
    if (!m_lightIntensityAverageCompareThread) {
        if (requestId == 0 || requestId == m_activeLightIntensityAverageCompareRequestId) {
            m_activeLightIntensityAverageCompareRequestId = 0;
            m_activeLightIntensityAverageCompareExperimentIds.clear();
        }
        return;
    }

    if (requestId > 0 && requestId != m_activeLightIntensityAverageCompareRequestId) {
        return;
    }

    qDebug() << "[compareCtrl][average cancel]"
             << "requestId=" << m_activeLightIntensityAverageCompareRequestId
             << "experimentIds=" << m_activeLightIntensityAverageCompareExperimentIds;
    m_lightIntensityAverageCompareThread->requestInterruption();
    m_activeLightIntensityAverageCompareRequestId = 0;
    m_activeLightIntensityAverageCompareExperimentIds.clear();
}

void compareCtrl::finishInstabilityOverviewRequest(int requestId,
                                                   const QVariantList &experimentIds,
                                                   const QVariantMap &payload,
                                                   const QString &errorMessage,
                                                   bool cancelled)
{
    if (requestId != m_activeInstabilityOverviewRequestId) {
        qDebug() << "[compareCtrl][instability overview stale]"
                 << "requestId=" << requestId
                 << "activeRequestId=" << m_activeInstabilityOverviewRequestId;
        return;
    }

    m_instabilityOverviewThread = nullptr;
    m_activeInstabilityOverviewRequestId = 0;
    m_activeInstabilityOverviewExperimentIds.clear();

    if (cancelled) {
        qDebug() << "[compareCtrl][instability overview cancelled]"
                 << "requestId=" << requestId
                 << "experimentIds=" << experimentIds;
        emit instabilityCompareOverviewRequestCancelled(requestId,
                                                        experimentIds,
                                                        QStringLiteral("interrupted"));
        return;
    }

    if (!errorMessage.isEmpty()) {
        qWarning() << "[compareCtrl][instability overview failed]"
                   << "requestId=" << requestId
                   << "experimentIds=" << experimentIds
                   << "message=" << errorMessage;
        emit instabilityCompareOverviewRequestFailed(requestId, experimentIds, errorMessage);
        return;
    }

    qDebug() << "[compareCtrl][instability overview finished]"
             << "requestId=" << requestId
             << "experimentIds=" << experimentIds;
    emit instabilityCompareOverviewRequestFinished(requestId, experimentIds, payload);
}

void compareCtrl::finishInstabilityCustomRequest(int requestId,
                                                 const QVariantList &experimentIds,
                                                 double lowerMm,
                                                 double upperMm,
                                                 const QVariantMap &payload,
                                                 const QString &errorMessage,
                                                 bool cancelled)
{
    if (requestId != m_activeInstabilityCustomRequestId) {
        qDebug() << "[compareCtrl][instability custom stale]"
                 << "requestId=" << requestId
                 << "activeRequestId=" << m_activeInstabilityCustomRequestId;
        return;
    }

    m_instabilityCustomThread = nullptr;
    m_activeInstabilityCustomRequestId = 0;
    m_activeInstabilityCustomExperimentIds.clear();
    m_activeInstabilityCustomLowerMm = 0.0;
    m_activeInstabilityCustomUpperMm = 0.0;

    if (cancelled) {
        qDebug() << "[compareCtrl][instability custom cancelled]"
                 << "requestId=" << requestId
                 << "experimentIds=" << experimentIds
                 << "lowerMm=" << lowerMm
                 << "upperMm=" << upperMm;
        emit instabilityCompareCustomRequestCancelled(requestId,
                                                      experimentIds,
                                                      lowerMm,
                                                      upperMm,
                                                      QStringLiteral("interrupted"));
        return;
    }

    if (!errorMessage.isEmpty()) {
        qWarning() << "[compareCtrl][instability custom failed]"
                   << "requestId=" << requestId
                   << "experimentIds=" << experimentIds
                   << "lowerMm=" << lowerMm
                   << "upperMm=" << upperMm
                   << "message=" << errorMessage;
        emit instabilityCompareCustomRequestFailed(requestId,
                                                   experimentIds,
                                                   lowerMm,
                                                   upperMm,
                                                   errorMessage);
        return;
    }

    qDebug() << "[compareCtrl][instability custom finished]"
             << "requestId=" << requestId
             << "experimentIds=" << experimentIds
             << "lowerMm=" << lowerMm
             << "upperMm=" << upperMm;
    emit instabilityCompareCustomRequestFinished(requestId,
                                                 experimentIds,
                                                 lowerMm,
                                                 upperMm,
                                                 payload);
}

void compareCtrl::finishUniformityCompareRequest(int requestId,
                                                 const QVariantList &experimentIds,
                                                 const QVariantMap &payload,
                                                 const QString &errorMessage,
                                                 bool cancelled)
{
    if (requestId != m_activeUniformityCompareRequestId) {
        qDebug() << "[compareCtrl][uniformity stale]"
                 << "requestId=" << requestId
                 << "activeRequestId=" << m_activeUniformityCompareRequestId;
        return;
    }

    m_uniformityCompareThread = nullptr;
    m_activeUniformityCompareRequestId = 0;
    m_activeUniformityCompareExperimentIds.clear();

    if (cancelled) {
        qDebug() << "[compareCtrl][uniformity cancelled]"
                 << "requestId=" << requestId
                 << "experimentIds=" << experimentIds;
        emit uniformityCompareRequestCancelled(requestId,
                                               experimentIds,
                                               QStringLiteral("interrupted"));
        return;
    }

    if (!errorMessage.isEmpty()) {
        qWarning() << "[compareCtrl][uniformity failed]"
                   << "requestId=" << requestId
                   << "experimentIds=" << experimentIds
                   << "message=" << errorMessage;
        emit uniformityCompareRequestFailed(requestId, experimentIds, errorMessage);
        return;
    }

    qDebug() << "[compareCtrl][uniformity finished]"
             << "requestId=" << requestId
             << "experimentIds=" << experimentIds
             << "bsSeries="
             << payload.value(QStringLiteral("bsChart")).toMap().value(QStringLiteral("seriesList")).toList().size()
             << "tSeries="
             << payload.value(QStringLiteral("tChart")).toMap().value(QStringLiteral("seriesList")).toList().size();
    emit uniformityCompareRequestFinished(requestId, experimentIds, payload);
}

void compareCtrl::finishLightIntensityAverageCompareRequest(int requestId,
                                                            const QVariantList &experimentIds,
                                                            const QVariantMap &payload,
                                                            const QString &errorMessage,
                                                            bool cancelled)
{
    if (requestId != m_activeLightIntensityAverageCompareRequestId) {
        qDebug() << "[compareCtrl][average stale]"
                 << "requestId=" << requestId
                 << "activeRequestId=" << m_activeLightIntensityAverageCompareRequestId;
        return;
    }

    m_lightIntensityAverageCompareThread = nullptr;
    m_activeLightIntensityAverageCompareRequestId = 0;
    m_activeLightIntensityAverageCompareExperimentIds.clear();

    if (cancelled) {
        qDebug() << "[compareCtrl][average cancelled]"
                 << "requestId=" << requestId
                 << "experimentIds=" << experimentIds;
        emit lightIntensityAverageCompareRequestCancelled(requestId,
                                                          experimentIds,
                                                          QStringLiteral("interrupted"));
        return;
    }

    if (!errorMessage.isEmpty()) {
        qWarning() << "[compareCtrl][average failed]"
                   << "requestId=" << requestId
                   << "experimentIds=" << experimentIds
                   << "message=" << errorMessage;
        emit lightIntensityAverageCompareRequestFailed(requestId, experimentIds, errorMessage);
        return;
    }

    qDebug() << "[compareCtrl][average finished]"
             << "requestId=" << requestId
             << "experimentIds=" << experimentIds
             << "bsSeries="
             << payload.value(QStringLiteral("bsChart")).toMap().value(QStringLiteral("seriesList")).toList().size()
             << "tSeries="
             << payload.value(QStringLiteral("tChart")).toMap().value(QStringLiteral("seriesList")).toList().size();
    emit lightIntensityAverageCompareRequestFinished(requestId, experimentIds, payload);
}
