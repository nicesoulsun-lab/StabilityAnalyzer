#include "inc/Common/experiment_session_service.h"

#include <QDateTime>
#include <QDebug>
#include <QtMath>

ExperimentSessionService::ExperimentSessionService()
{
}

int ExperimentSessionService::calculateTotalSeconds(int days, int hours, int minutes, int seconds) const
{
    return days * 86400 + hours * 3600 + minutes * 60 + seconds;
}

int ExperimentSessionService::calculateExpectedPointCount(const ExperimentParams& params) const
{
    const double stepUm = static_cast<double>(params.scanStep);
    if (stepUm <= 0.0) {
        return 0;
    }

    const double startUm = static_cast<double>(params.scanRangeStart) * 1000.0;
    const double endUm = static_cast<double>(params.scanRangeEnd) * 1000.0;
    const double spanUm = qMax(0.0, endUm - startUm);
    return qMax(0, static_cast<int>(qFloor(spanUm / stepUm)));
}

ExperimentScanProfile ExperimentSessionService::buildScanProfile(const ExperimentParams& params) const
{
    ExperimentScanProfile profile;
    profile.expectedPointCount = calculateExpectedPointCount(params);
    profile.startHeightUm = static_cast<double>(params.scanRangeStart) * 1000.0;
    profile.stepUm = static_cast<double>(params.scanStep);
    const qint64 configuredIntervalMs = static_cast<qint64>(
                calculateTotalSeconds(0, params.intervalHours, params.intervalMinutes, params.intervalSeconds)) * 1000;
    const qint64 totalDurationMs = static_cast<qint64>(
                calculateTotalSeconds(params.durationDays, params.durationHours,
                                      params.durationMinutes, params.durationSeconds)) * 1000;
    if (params.scanCount > 0 && totalDurationMs > 0) {
        profile.idealScanIntervalMs = qMax<qint64>(0, totalDurationMs / params.scanCount);
    } else {
        profile.idealScanIntervalMs = qMax<qint64>(0, configuredIntervalMs);
    }
    return profile;
}

void ExperimentSessionService::resetScanContexts(int channel)
{
    m_scanContexts[channel].clear();
    m_scanProfiles.remove(channel);
    m_nextScanSequences[channel] = 0;
    m_currentScanCounts[channel] = 0;
}

void ExperimentSessionService::setScanProfile(int channel, const ExperimentScanProfile& profile)
{
    m_scanProfiles[channel] = profile;
}

void ExperimentSessionService::beginScanCycle(int channel, const ExperimentParams& params)
{
    const ExperimentScanProfile profile = m_scanProfiles.contains(channel)
            ? m_scanProfiles.value(channel)
            : buildScanProfile(params);

    ScanCycleContext context;
    const int scanId = m_nextScanSequences.value(channel, 0);
    context.sequence = scanId;
    context.scanId = scanId;
    context.expectedPointCount = profile.expectedPointCount;
    context.savedPointCount = 0;
    context.startHeightUm = profile.startHeightUm;
    context.stepUm = profile.stepUm;
    const qint64 experimentStartMs = (profile.experimentStartMs > 0)
            ? profile.experimentStartMs
            : QDateTime::currentMSecsSinceEpoch();
    context.elapsedSinceExperimentStartMs = qMax<qint64>(0, profile.idealScanIntervalMs * scanId);
    context.startedAtMs = experimentStartMs + context.elapsedSinceExperimentStartMs;
    m_nextScanSequences[channel] = scanId + 1;

    m_scanContexts[channel].append(context);
    refreshCurrentScanCount(channel);

    qDebug() << "[ExperimentSessionService][ScanCycle] channel=" << channel
             << "scanId=" << context.scanId
             << "expectedPointCount=" << context.expectedPointCount
             << "startHeightUm=" << context.startHeightUm
             << "stepUm=" << context.stepUm
             << "elapsedSinceExperimentStartMs=" << context.elapsedSinceExperimentStartMs
             << "pendingContexts=" << m_scanContexts.value(channel).size();
}

int ExperimentSessionService::currentScanCount(int channel) const
{
    return m_currentScanCounts.value(channel, 0);
}

int ExperimentSessionService::pendingContextCount(int channel) const
{
    return m_scanContexts.value(channel).size();
}

void ExperimentSessionService::setCalibration(int channel, int transmissionRef, int backscatterRef)
{
    m_transmissionCalibrations[channel] = transmissionRef;
    m_backscatterCalibrations[channel] = backscatterRef;
}

int ExperimentSessionService::transmissionCalibration(int channel) const
{
    return m_transmissionCalibrations.value(channel, 0);
}

int ExperimentSessionService::backscatterCalibration(int channel) const
{
    return m_backscatterCalibrations.value(channel, 0);
}

QVector<QVariantMap> ExperimentSessionService::buildRowsFromStorageData(int channel,
                                                                        const QVector<quint16>& raw,
                                                                        bool areaA)
{
    QVector<QVariantMap> dataList;
    QVector<ScanCycleContext>& contexts = m_scanContexts[channel];
    const int totalPairs = raw.size() / 2;
    int consumedPairs = 0;

    while (consumedPairs < totalPairs) {
        while (!contexts.isEmpty() &&
               contexts.first().savedPointCount >= contexts.first().expectedPointCount) {
            const ScanCycleContext completed = contexts.first();
            contexts.remove(0);
            qDebug() << "[ExperimentSessionService][ScanCycle] channel=" << channel
                     << "scanId=" << completed.scanId
                     << "completed before consume"
                     << "savedPointCount=" << completed.savedPointCount;
        }

        if (contexts.isEmpty()) {
            qWarning() << "[ExperimentSessionService][Fetch] channel=" << channel
                       << (areaA ? "A" : "B")
                       << "drop pairs because no pending scan context, droppedPairs="
                       << (totalPairs - consumedPairs);
            break;
        }

        ScanCycleContext& context = contexts[0];
        const int remainingPoints = qMax(0, context.expectedPointCount - context.savedPointCount);
        if (remainingPoints <= 0) {
            continue;
        }

        const int takePairs = qMin(remainingPoints, totalPairs - consumedPairs);
        const QVector<quint16> slice = raw.mid(consumedPairs * 2, takePairs * 2);
        const QVector<QVariantMap> batch = parseStoragePairs(channel, slice, areaA,
                                                             context.startHeightUm,
                                                             context.stepUm,
                                                             context.savedPointCount,
                                                             context.startedAtMs,
                                                             context.elapsedSinceExperimentStartMs,
                                                             context.scanId);
        dataList += batch;

        const int savedPairs = batch.size();
        context.savedPointCount += savedPairs;
        consumedPairs += savedPairs;

        qDebug() << "[ExperimentSessionService][ScanCycle] channel=" << channel
                 << "scanId=" << context.scanId
                 << "area=" << (areaA ? "A" : "B")
                 << "savedPairs=" << savedPairs
                 << "progress=" << context.savedPointCount << "/" << context.expectedPointCount;

        if (context.savedPointCount >= context.expectedPointCount) {
            const ScanCycleContext completed = contexts.first();
            for (int i = dataList.size() - savedPairs; i < dataList.size(); ++i) {
                dataList[i]["scan_completed"] = true;
            }
            contexts.remove(0);
            qDebug() << "[ExperimentSessionService][ScanCycle] channel=" << channel
                     << "scanId=" << completed.scanId
                     << "completed"
                     << "totalSavedPoints=" << completed.savedPointCount;
        }
    }

    refreshCurrentScanCount(channel);
    return dataList;
}

QVector<QVariantMap> ExperimentSessionService::buildCalibrationRows(
    int channel, const QVector<quint16>& raw, bool areaA,
    double scanRangeStartMm, double scanStepUm) const
{
    Q_UNUSED(channel)
    QVector<QVariantMap> dataList;
    const int pairCount = raw.size() / 2;
    // A 区起始点索引为 0，B 区起始点索引为 250
    const int startPointIndex = areaA ? 0 : 250;
    dataList.reserve(pairCount);

    for (int i = 0; i < pairCount; ++i) {
        QVariantMap row;
        // 高度 = 扫描起始位置(um) + 点索引 * 步长(um)，再转为 mm
        row["height"] = (scanRangeStartMm * 1000.0
                         + static_cast<double>(startPointIndex + i) * scanStepUm) / 1000.0;
        // 校准数据不应用校准转换，直接返回原始值
        row["transmission_intensity"] = static_cast<double>(raw[i * 2]);
        row["backscatter_intensity"] = static_cast<double>(raw[i * 2 + 1]);
        dataList.append(row);
    }

    return dataList;
}

void ExperimentSessionService::refreshCurrentScanCount(int channel)
{
    const QVector<ScanCycleContext>& contexts = m_scanContexts.value(channel);
    m_currentScanCounts[channel] = contexts.isEmpty() ? 0 : contexts.first().savedPointCount;
}

QVector<QVariantMap> ExperimentSessionService::parseStoragePairs(int channel,
                                                                 const QVector<quint16>& raw,
                                                                 bool areaA,
                                                                 double startHeightUm,
                                                                 double stepUm,
                                                                 int startPointIndex,
                                                                 qint64 scanStartedAtMs,
                                                                 qint64 elapsedSinceExperimentStartMs,
                                                                 int scanId) const
{
    QVector<QVariantMap> dataList;
    const int pairCount = raw.size() / 2;
    dataList.reserve(pairCount);
    const int baseTs = static_cast<int>(scanStartedAtMs / 1000);

    for (int i = 0; i < pairCount; ++i) {
        const int pointIndex = startPointIndex + i;

        QVariantMap row;
        row["timestamp"] = baseTs;
        row["scan_id"] = scanId;
        row["scan_elapsed_ms"] = elapsedSinceExperimentStartMs;
        row["height"] = startHeightUm + (static_cast<double>(pointIndex) * stepUm);
        const int rawTransmission = raw[(i * 2)];
        const int rawBackscatter = raw[(i * 2) + 1];
        const int transRef = m_transmissionCalibrations.value(channel, 0);
        const int backRef = m_backscatterCalibrations.value(channel, 0);

        if (transRef > 0) {
            row["transmission_intensity"] = qRound(static_cast<double>(rawTransmission)
                                            / static_cast<double>(transRef) * 1000.0) / 10.0;
        } else {
            row["transmission_intensity"] = static_cast<double>(rawTransmission);
        }

        if (backRef > 0) {
            row["backscatter_intensity"] = qRound(static_cast<double>(rawBackscatter)
                                           / static_cast<double>(backRef) * 1000.0) / 10.0;
        } else {
            row["backscatter_intensity"] = static_cast<double>(rawBackscatter);
        }
        row["channel"] = channel;
        row["point_index"] = pointIndex;
        row["storage_area"] = areaA ? "A" : "B";
        dataList.append(row);
    }

    return dataList;
}
