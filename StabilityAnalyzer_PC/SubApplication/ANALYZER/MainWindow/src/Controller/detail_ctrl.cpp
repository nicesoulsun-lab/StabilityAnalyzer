#include "Controller/detail_ctrl.h"

#include "Analysis/CurveChartAnalysisEngine.h"
#include "Analysis/LightCurveAnalysisEngine.h"
#include "SqlOrmManager.h"
#include "../../../SqlOrm/inc/SqlOrmManager.h"

#include <QDebug>
#include <QPointF>
#include <QtMath>
#include <algorithm>
#include <limits>

namespace {

constexpr int kDetailCurveWindowLimit = 32;
constexpr int kDetailCurvePointLimit = 512;

QVariantMap buildInstabilityOverviewPayload(const QVariantMap &overallSeries,
                                            const QVariantMap &bottomSeries,
                                            const QVariantMap &middleSeries,
                                            const QVariantMap &topSeries,
                                            const QVariantMap &radarData,
                                            double minHeightMm,
                                            double maxHeightMm)
{
    QVariantMap payload;
    payload.insert(QStringLiteral("overallSeries"), overallSeries);
    payload.insert(QStringLiteral("bottomSeries"), bottomSeries);
    payload.insert(QStringLiteral("middleSeries"), middleSeries);
    payload.insert(QStringLiteral("topSeries"), topSeries);
    payload.insert(QStringLiteral("radarData"), radarData);
    payload.insert(QStringLiteral("minHeightValue"), minHeightMm);
    payload.insert(QStringLiteral("maxHeightValue"), maxHeightMm);
    return payload;
}

QVariantMap buildInstabilityCustomPayload(const QVariantMap &series, double lowerMm, double upperMm)
{
    QVariantMap payload;
    payload.insert(QStringLiteral("series"), series);
    payload.insert(QStringLiteral("lowerMm"), lowerMm);
    payload.insert(QStringLiteral("upperMm"), upperMm);
    return payload;
}

int seriesPointCount(const QVariantMap &series)
{
    return series.value(QStringLiteral("points")).toList().size();
}

QVariantList toVariantList(const QVector<QVariantMap> &rows)
{
    QVariantList result;
    result.reserve(rows.size());
    for (const QVariantMap &row : rows) {
        result.append(row);
    }
    return result;
}

QVariantMap buildLightCurvePayload(const QVector<QVariantMap> &curves,
                                   int totalCurveCount,
                                   int windowStartIndex)
{
    QVariantMap payload;
    payload.insert(QStringLiteral("curves"), toVariantList(curves));
    payload.insert(QStringLiteral("loadedCurveCount"), curves.size());
    payload.insert(QStringLiteral("totalCurveCount"), totalCurveCount);
    payload.insert(QStringLiteral("truncated"), curves.size() < totalCurveCount);
    payload.insert(QStringLiteral("windowStartIndex"), windowStartIndex);

    double minHeight = std::numeric_limits<double>::infinity();
    double maxHeight = -std::numeric_limits<double>::infinity();
    double minLight = std::numeric_limits<double>::infinity();
    double maxLight = -std::numeric_limits<double>::infinity();

    for (const QVariantMap &curve : curves) {
        const double curveMinHeight = curve.value(QStringLiteral("min_height_mm")).toDouble();
        const double curveMaxHeight = curve.value(QStringLiteral("max_height_mm")).toDouble();
        const double curveMinBackscatter = curve.value(QStringLiteral("min_backscatter")).toDouble();
        const double curveMaxBackscatter = curve.value(QStringLiteral("max_backscatter")).toDouble();
        const double curveMinTransmission = curve.value(QStringLiteral("min_transmission")).toDouble();
        const double curveMaxTransmission = curve.value(QStringLiteral("max_transmission")).toDouble();

        if (qIsFinite(curveMinHeight)) {
            minHeight = qMin(minHeight, curveMinHeight);
        }
        if (qIsFinite(curveMaxHeight)) {
            maxHeight = qMax(maxHeight, curveMaxHeight);
        }
        if (qIsFinite(curveMinBackscatter)) {
            minLight = qMin(minLight, curveMinBackscatter);
        }
        if (qIsFinite(curveMaxBackscatter)) {
            maxLight = qMax(maxLight, curveMaxBackscatter);
        }
        if (qIsFinite(curveMinTransmission)) {
            minLight = qMin(minLight, curveMinTransmission);
        }
        if (qIsFinite(curveMaxTransmission)) {
            maxLight = qMax(maxLight, curveMaxTransmission);
        }
    }

    payload.insert(QStringLiteral("minHeightValue"), qIsFinite(minHeight) ? minHeight : 0.0);
    payload.insert(QStringLiteral("maxHeightValue"), qIsFinite(maxHeight) ? maxHeight : 10.0);
    payload.insert(QStringLiteral("minLightValue"), qIsFinite(minLight) ? minLight : 0.0);
    payload.insert(QStringLiteral("maxLightValue"), qIsFinite(maxLight) ? maxLight : 100.0);
    return payload;
}

} // namespace

detailCtrl::detailCtrl(QObject *parent)
    : QObject(parent)
    , m_dbManager(SqlOrmManager::instance())
{
}

detailCtrl::~detailCtrl()
{
    cancelLightCurveRequest();
    cancelInstabilityRequest();
    if (m_lightCurveRequestThread) {
        m_lightCurveRequestThread->requestInterruption();
        m_lightCurveRequestThread->wait(3000);
    }
    if (m_instabilityOverviewRequestThread) {
        m_instabilityOverviewRequestThread->requestInterruption();
        m_instabilityOverviewRequestThread->wait(3000);
    }
    if (m_instabilityCustomRequestThread) {
        m_instabilityCustomRequestThread->requestInterruption();
        m_instabilityCustomRequestThread->wait(3000);
    }
}

int detailCtrl::requestLightCurves(int experimentId, int pointsPerCurve)
{
    if (experimentId <= 0 || !m_dbManager) {
        qWarning() << "[detailCtrl][light request] invalid request"
                   << "experimentId=" << experimentId
                   << "pointsPerCurve=" << pointsPerCurve;
        return 0;
    }

    cancelLightCurveRequest();

    const int requestId = ++m_requestSerial;
    const int safePointLimit = qBound(128, pointsPerCurve, kDetailCurvePointLimit);
    m_activeLightCurveRequestId = requestId;
    m_activeLightCurveExperimentId = experimentId;

    qDebug() << "[detailCtrl][light request start]"
             << "requestId=" << requestId
             << "experimentId=" << experimentId
             << "pointsPerCurve=" << safePointLimit
             << "windowLimit=" << kDetailCurveWindowLimit;
    emit lightCurveRequestStarted(requestId, experimentId);

    QPointer<detailCtrl> self(this);
    QThread *workerThread = QThread::create([self, requestId, experimentId, safePointLimit]() {
        if (!self) {
            return;
        }

        QVector<QVariantMap> curves;
        QVariantMap payload;
        QString errorMessage;
        bool cancelled = false;

        try {
            QVector<int> scanIds = self->m_dbManager->getExperimentScanIds(experimentId);
            std::sort(scanIds.begin(), scanIds.end());

            const int totalCurveCount = scanIds.size();
            const int windowStartIndex = qMax(0, totalCurveCount - kDetailCurveWindowLimit);
            const QVector<int> windowScanIds = scanIds.mid(windowStartIndex);

            curves.reserve(windowScanIds.size());
            for (int scanId : windowScanIds) {
                if (!self || QThread::currentThread()->isInterruptionRequested()) {
                    cancelled = true;
                    break;
                }

                const QVector<QVariantMap> singleCurve = self->m_dbManager->getLightIntensityCurveByScan(
                    experimentId, scanId, safePointLimit);
                if (!singleCurve.isEmpty()) {
                    curves.append(singleCurve.first());
                }
            }

            if (!cancelled) {
                payload = buildLightCurvePayload(curves, totalCurveCount, windowStartIndex);
            }
        } catch (const std::bad_alloc &) {
            errorMessage = self->tr("璇︽儏鏇茬嚎鍔犺浇鍐呭瓨涓嶈冻");
        }

        QMetaObject::invokeMethod(self, [self, requestId, experimentId, curves, payload, errorMessage, cancelled]() {
            if (self) {
                self->finishLightCurveRequest(requestId, experimentId, curves, payload, errorMessage, cancelled);
            }
        }, Qt::QueuedConnection);
    });

    m_lightCurveRequestThread = workerThread;
    connect(workerThread, &QThread::finished, workerThread, &QObject::deleteLater);
    workerThread->start();
    return requestId;
}

int detailCtrl::requestInstabilityOverview(int experimentId, double minHeightMm, double maxHeightMm)
{
    if (experimentId <= 0 || !m_dbManager) {
        qWarning() << "[detailCtrl][instability overview request] invalid request"
                   << "experimentId=" << experimentId
                   << "minHeightMm=" << minHeightMm
                   << "maxHeightMm=" << maxHeightMm;
        return 0;
    }

    cancelInstabilityRequest(m_activeInstabilityOverviewRequestId);

    const int requestId = ++m_requestSerial;
    const double safeMinHeight = qMin(minHeightMm, maxHeightMm);
    const double safeMaxHeight = qMax(minHeightMm, maxHeightMm);
    const double sectionHeight = qMax((safeMaxHeight - safeMinHeight) / 3.0, 0.0);
    const double firstSplit = safeMinHeight + sectionHeight;
    const double secondSplit = safeMinHeight + sectionHeight * 2.0;

    m_activeInstabilityOverviewRequestId = requestId;
    m_activeInstabilityOverviewExperimentId = experimentId;

    qDebug() << "[detailCtrl][instability overview start]"
             << "requestId=" << requestId
             << "experimentId=" << experimentId
             << "minHeightMm=" << safeMinHeight
             << "maxHeightMm=" << safeMaxHeight;
    emit instabilityOverviewRequestStarted(requestId, experimentId);

    QPointer<detailCtrl> self(this);
    QThread *workerThread = QThread::create([self, requestId, experimentId, safeMinHeight, safeMaxHeight, firstSplit, secondSplit]() {
        if (!self) {
            return;
        }

        QVariantMap payload;
        QString errorMessage;
        bool cancelled = false;

        try {
            const QVector<QVariantMap> overallRows =
                self->m_dbManager->getOrComputeInstabilityCurveDataByExperiment(experimentId);
            if (!self || QThread::currentThread()->isInterruptionRequested()) {
                cancelled = true;
            }

            QVector<QVariantMap> bottomRows;
            QVector<QVariantMap> middleRows;
            QVector<QVariantMap> topRows;

            if (!cancelled) {
                bottomRows = self->m_dbManager->getOrComputeInstabilityCurveDataByHeightRange(
                    experimentId, safeMinHeight, firstSplit, QStringLiteral("bottom"));
                if (!self || QThread::currentThread()->isInterruptionRequested()) {
                    cancelled = true;
                }
            }

            if (!cancelled) {
                middleRows = self->m_dbManager->getOrComputeInstabilityCurveDataByHeightRange(
                    experimentId, firstSplit, secondSplit, QStringLiteral("middle"));
                if (!self || QThread::currentThread()->isInterruptionRequested()) {
                    cancelled = true;
                }
            }

            if (!cancelled) {
                topRows = self->m_dbManager->getOrComputeInstabilityCurveDataByHeightRange(
                    experimentId, secondSplit, safeMaxHeight, QStringLiteral("top"));
                if (!self || QThread::currentThread()->isInterruptionRequested()) {
                    cancelled = true;
                }
            }

            if (!cancelled) {
                const QVariantMap overallSeries = CurveChartAnalysisEngine::buildInstabilitySeriesChartData(
                    overallRows, QStringLiteral("\u6574\u4f53"), safeMinHeight, safeMaxHeight);
                const QVariantMap bottomSeries = CurveChartAnalysisEngine::buildInstabilitySeriesChartData(
                    bottomRows, QStringLiteral("\u5e95\u90e8"), safeMinHeight, firstSplit);
                const QVariantMap middleSeries = CurveChartAnalysisEngine::buildInstabilitySeriesChartData(
                    middleRows, QStringLiteral("\u4e2d\u90e8"), firstSplit, secondSplit);
                const QVariantMap topSeries = CurveChartAnalysisEngine::buildInstabilitySeriesChartData(
                    topRows, QStringLiteral("\u9876\u90e8"), secondSplit, safeMaxHeight);
                const QVariantMap radarData = CurveChartAnalysisEngine::buildInstabilityRadarChartData(
                    overallSeries, bottomSeries, middleSeries, topSeries);
                payload = buildInstabilityOverviewPayload(overallSeries,
                                                         bottomSeries,
                                                         middleSeries,
                                                         topSeries,
                                                         radarData,
                                                         safeMinHeight,
                                                         safeMaxHeight);
            }
        } catch (const std::bad_alloc &) {
            errorMessage = QStringLiteral("\u4e0d\u7a33\u5b9a\u6027\u6570\u636e\u52a0\u8f7d\u5185\u5b58\u4e0d\u8db3");
        }

        QMetaObject::invokeMethod(self, [self, requestId, experimentId, payload, errorMessage, cancelled]() {
            if (self) {
                self->finishInstabilityOverviewRequest(requestId, experimentId, payload, errorMessage, cancelled);
            }
        }, Qt::QueuedConnection);
    });

    m_instabilityOverviewRequestThread = workerThread;
    connect(workerThread, &QThread::finished, workerThread, &QObject::deleteLater);
    workerThread->start();
    return requestId;
}

int detailCtrl::requestInstabilityCustomSeries(int experimentId, double lowerMm, double upperMm)
{
    if (experimentId <= 0 || !m_dbManager) {
        qWarning() << "[detailCtrl][instability custom request] invalid request"
                   << "experimentId=" << experimentId
                   << "lowerMm=" << lowerMm
                   << "upperMm=" << upperMm;
        return 0;
    }

    cancelInstabilityRequest(m_activeInstabilityCustomRequestId);

    const int requestId = ++m_requestSerial;
    const double safeLower = qMin(lowerMm, upperMm);
    const double safeUpper = qMax(lowerMm, upperMm);

    m_activeInstabilityCustomRequestId = requestId;
    m_activeInstabilityCustomExperimentId = experimentId;
    m_activeInstabilityCustomLowerMm = safeLower;
    m_activeInstabilityCustomUpperMm = safeUpper;

    qDebug() << "[detailCtrl][instability custom start]"
             << "requestId=" << requestId
             << "experimentId=" << experimentId
             << "lowerMm=" << safeLower
             << "upperMm=" << safeUpper;
    emit instabilityCustomSeriesRequestStarted(requestId, experimentId, safeLower, safeUpper);

    QPointer<detailCtrl> self(this);
    QThread *workerThread = QThread::create([self, requestId, experimentId, safeLower, safeUpper]() {
        if (!self) {
            return;
        }

        QVariantMap payload;
        QString errorMessage;
        bool cancelled = false;

        try {
            const QVector<QVariantMap> rows = self->m_dbManager->getOrComputeInstabilityCurveDataByHeightRange(
                experimentId, safeLower, safeUpper, QStringLiteral("custom"));
            if (!self || QThread::currentThread()->isInterruptionRequested()) {
                cancelled = true;
            }

            if (!cancelled) {
                const QVariantMap customSeries = CurveChartAnalysisEngine::buildInstabilitySeriesChartData(
                    rows, QStringLiteral("\u81ea\u5b9a\u4e49"), safeLower, safeUpper);
                payload = buildInstabilityCustomPayload(customSeries, safeLower, safeUpper);
            }
        } catch (const std::bad_alloc &) {
            errorMessage = QStringLiteral("\u81ea\u5b9a\u4e49\u4e0d\u7a33\u5b9a\u6027\u6570\u636e\u52a0\u8f7d\u5185\u5b58\u4e0d\u8db3");
        }

        QMetaObject::invokeMethod(self, [self, requestId, experimentId, safeLower, safeUpper, payload, errorMessage, cancelled]() {
            if (self) {
                self->finishInstabilityCustomRequest(requestId,
                                                     experimentId,
                                                     safeLower,
                                                     safeUpper,
                                                     payload,
                                                     errorMessage,
                                                     cancelled);
            }
        }, Qt::QueuedConnection);
    });

    m_instabilityCustomRequestThread = workerThread;
    connect(workerThread, &QThread::finished, workerThread, &QObject::deleteLater);
    workerThread->start();
    return requestId;
}

void detailCtrl::cancelLightCurveRequest(int requestId)
{
    if (!m_lightCurveRequestThread) {
        if (requestId == 0 || requestId == m_activeLightCurveRequestId) {
            m_activeLightCurveRequestId = 0;
            m_activeLightCurveExperimentId = 0;
        }
        return;
    }

    if (requestId > 0 && requestId != m_activeLightCurveRequestId) {
        return;
    }

    qDebug() << "[detailCtrl][light request cancel]"
             << "requestId=" << m_activeLightCurveRequestId
             << "experimentId=" << m_activeLightCurveExperimentId;
    m_lightCurveRequestThread->requestInterruption();
    m_activeLightCurveRequestId = 0;
    m_activeLightCurveExperimentId = 0;
}

void detailCtrl::cancelInstabilityRequest(int requestId)
{
    if (m_instabilityOverviewRequestThread
            && (requestId == 0 || requestId == m_activeInstabilityOverviewRequestId)) {
        qDebug() << "[detailCtrl][instability overview cancel]"
                 << "requestId=" << m_activeInstabilityOverviewRequestId
                 << "experimentId=" << m_activeInstabilityOverviewExperimentId;
        m_instabilityOverviewRequestThread->requestInterruption();
        m_activeInstabilityOverviewRequestId = 0;
        m_activeInstabilityOverviewExperimentId = 0;
    }

    if (m_instabilityCustomRequestThread
            && (requestId == 0 || requestId == m_activeInstabilityCustomRequestId)) {
        qDebug() << "[detailCtrl][instability custom cancel]"
                 << "requestId=" << m_activeInstabilityCustomRequestId
                 << "experimentId=" << m_activeInstabilityCustomExperimentId
                 << "lowerMm=" << m_activeInstabilityCustomLowerMm
                 << "upperMm=" << m_activeInstabilityCustomUpperMm;
        m_instabilityCustomRequestThread->requestInterruption();
        m_activeInstabilityCustomRequestId = 0;
        m_activeInstabilityCustomExperimentId = 0;
        m_activeInstabilityCustomLowerMm = 0.0;
        m_activeInstabilityCustomUpperMm = 0.0;
    }
}

void detailCtrl::clearExperimentCache(int experimentId)
{
    if (experimentId > 0) {
        m_lightCurveCacheByExperiment.remove(experimentId);
        qDebug() << "[detailCtrl][cache clear]" << "experimentId=" << experimentId;
        return;
    }

    if (!m_lightCurveCacheByExperiment.isEmpty()) {
        qDebug() << "[detailCtrl][cache clear all]"
                 << "experimentCount=" << m_lightCurveCacheByExperiment.size();
    }
    m_lightCurveCacheByExperiment.clear();
}

QVariantList detailCtrl::getProcessedLightIntensityCurves(int experimentId,
                                                          int referenceScanId,
                                                          double lowerMm,
                                                          double upperMm,
                                                          bool useReference) const
{
    if (experimentId <= 0) {
        qWarning() << "[detailCtrl][processed curves] invalid experimentId";
        return QVariantList();
    }

    const QVector<QVariantMap> rawCurves = m_lightCurveCacheByExperiment.value(experimentId);
    if (rawCurves.isEmpty()) {
        qDebug() << "[detailCtrl][processed curves] empty cache"
                 << "experimentId=" << experimentId;
        return QVariantList();
    }

    qDebug() << "[detailCtrl][processed curves start]"
             << "experimentId=" << experimentId
             << "curveCount=" << rawCurves.size()
             << "referenceScanId=" << referenceScanId
             << "lowerMm=" << lowerMm
             << "upperMm=" << upperMm
             << "useReference=" << useReference;

    const QVector<LightCurveAnalysisCurve> parsedCurves = LightCurveAnalysisEngine::fromVariantMaps(rawCurves);
    const QVector<QVariantMap> rows = LightCurveAnalysisEngine::processCurves(parsedCurves,
                                                                              referenceScanId,
                                                                              lowerMm,
                                                                              upperMm,
                                                                              useReference);
    qDebug() << "[detailCtrl][processed curves ready]"
             << "experimentId=" << experimentId
             << "inputCurveCount=" << parsedCurves.size()
             << "outputCurveCount=" << rows.size();
    return toVariantList(rows);
}

void detailCtrl::finishLightCurveRequest(int requestId,
                                         int experimentId,
                                         const QVector<QVariantMap> &curves,
                                         const QVariantMap &payload,
                                         const QString &errorMessage,
                                         bool cancelled)
{
    if (requestId != m_activeLightCurveRequestId || experimentId != m_activeLightCurveExperimentId) {
        qDebug() << "[detailCtrl][light request stale]"
                 << "requestId=" << requestId
                 << "experimentId=" << experimentId
                 << "activeRequestId=" << m_activeLightCurveRequestId
                 << "activeExperimentId=" << m_activeLightCurveExperimentId;
        return;
    }

    m_lightCurveRequestThread = nullptr;
    m_activeLightCurveRequestId = 0;
    m_activeLightCurveExperimentId = 0;

    if (cancelled) {
        qDebug() << "[detailCtrl][light request cancelled]"
                 << "requestId=" << requestId
                 << "experimentId=" << experimentId;
        emit lightCurveRequestCancelled(requestId, experimentId, QStringLiteral("interrupted"));
        return;
    }

    if (!errorMessage.isEmpty()) {
        qWarning() << "[detailCtrl][light request failed]"
                   << "requestId=" << requestId
                   << "experimentId=" << experimentId
                   << "message=" << errorMessage;
        emit lightCurveRequestFailed(requestId, experimentId, errorMessage);
        return;
    }

    m_lightCurveCacheByExperiment.clear();
    m_lightCurveCacheByExperiment.insert(experimentId, curves);

    qDebug() << "[detailCtrl][light request finished]"
             << "requestId=" << requestId
             << "experimentId=" << experimentId
             << "loadedCurveCount=" << payload.value(QStringLiteral("loadedCurveCount")).toInt()
             << "totalCurveCount=" << payload.value(QStringLiteral("totalCurveCount")).toInt()
             << "truncated=" << payload.value(QStringLiteral("truncated")).toBool();
    emit lightCurveRequestFinished(requestId, experimentId, payload);
}

void detailCtrl::finishInstabilityOverviewRequest(int requestId,
                                                  int experimentId,
                                                  const QVariantMap &payload,
                                                  const QString &errorMessage,
                                                  bool cancelled)
{
    if (requestId != m_activeInstabilityOverviewRequestId
            || experimentId != m_activeInstabilityOverviewExperimentId) {
        qDebug() << "[detailCtrl][instability overview stale]"
                 << "requestId=" << requestId
                 << "experimentId=" << experimentId
                 << "activeRequestId=" << m_activeInstabilityOverviewRequestId
                 << "activeExperimentId=" << m_activeInstabilityOverviewExperimentId;
        return;
    }

    m_instabilityOverviewRequestThread = nullptr;
    m_activeInstabilityOverviewRequestId = 0;
    m_activeInstabilityOverviewExperimentId = 0;

    if (cancelled) {
        qDebug() << "[detailCtrl][instability overview cancelled]"
                 << "requestId=" << requestId
                 << "experimentId=" << experimentId;
        emit instabilityOverviewRequestCancelled(requestId, experimentId, QStringLiteral("interrupted"));
        return;
    }

    if (!errorMessage.isEmpty()) {
        qWarning() << "[detailCtrl][instability overview failed]"
                   << "requestId=" << requestId
                   << "experimentId=" << experimentId
                   << "message=" << errorMessage;
        emit instabilityOverviewRequestFailed(requestId, experimentId, errorMessage);
        return;
    }

    const QVariantMap overallSeries = payload.value(QStringLiteral("overallSeries")).toMap();
    const QVariantMap bottomSeries = payload.value(QStringLiteral("bottomSeries")).toMap();
    const QVariantMap middleSeries = payload.value(QStringLiteral("middleSeries")).toMap();
    const QVariantMap topSeries = payload.value(QStringLiteral("topSeries")).toMap();
    const QVariantMap radarData = payload.value(QStringLiteral("radarData")).toMap();

    qDebug() << "[detailCtrl][instability overview finished]"
             << "requestId=" << requestId
             << "experimentId=" << experimentId
             << "overallPoints=" << seriesPointCount(overallSeries)
             << "bottomPoints=" << seriesPointCount(bottomSeries)
             << "middlePoints=" << seriesPointCount(middleSeries)
             << "topPoints=" << seriesPointCount(topSeries)
             << "radarPolygons=" << radarData.value(QStringLiteral("polygons")).toList().size();
    emit instabilityOverviewRequestFinished(requestId, experimentId, payload);
}

void detailCtrl::finishInstabilityCustomRequest(int requestId,
                                                int experimentId,
                                                double lowerMm,
                                                double upperMm,
                                                const QVariantMap &payload,
                                                const QString &errorMessage,
                                                bool cancelled)
{
    if (requestId != m_activeInstabilityCustomRequestId
            || experimentId != m_activeInstabilityCustomExperimentId) {
        qDebug() << "[detailCtrl][instability custom stale]"
                 << "requestId=" << requestId
                 << "experimentId=" << experimentId
                 << "activeRequestId=" << m_activeInstabilityCustomRequestId
                 << "activeExperimentId=" << m_activeInstabilityCustomExperimentId;
        return;
    }

    m_instabilityCustomRequestThread = nullptr;
    m_activeInstabilityCustomRequestId = 0;
    m_activeInstabilityCustomExperimentId = 0;
    m_activeInstabilityCustomLowerMm = 0.0;
    m_activeInstabilityCustomUpperMm = 0.0;

    if (cancelled) {
        qDebug() << "[detailCtrl][instability custom cancelled]"
                 << "requestId=" << requestId
                 << "experimentId=" << experimentId
                 << "lowerMm=" << lowerMm
                 << "upperMm=" << upperMm;
        emit instabilityCustomSeriesRequestCancelled(requestId, experimentId, QStringLiteral("interrupted"));
        return;
    }

    if (!errorMessage.isEmpty()) {
        qWarning() << "[detailCtrl][instability custom failed]"
                   << "requestId=" << requestId
                   << "experimentId=" << experimentId
                   << "lowerMm=" << lowerMm
                   << "upperMm=" << upperMm
                   << "message=" << errorMessage;
        emit instabilityCustomSeriesRequestFailed(requestId, experimentId, errorMessage);
        return;
    }

    const QVariantMap series = payload.value(QStringLiteral("series")).toMap();
    qDebug() << "[detailCtrl][instability custom finished]"
             << "requestId=" << requestId
             << "experimentId=" << experimentId
             << "lowerMm=" << lowerMm
             << "upperMm=" << upperMm
             << "pointCount=" << seriesPointCount(series);
    emit instabilityCustomSeriesRequestFinished(requestId, experimentId, payload);
}
