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
        return 1;
    }

    const double startUm = static_cast<double>(params.scanRangeStart) * 1000.0;
    const double endUm = static_cast<double>(params.scanRangeEnd) * 1000.0;
    const double spanUm = qMax(0.0, endUm - startUm);
    return qMax(1, static_cast<int>(qFloor(spanUm / stepUm)) + 1);
}

ExperimentScanProfile ExperimentSessionService::buildScanProfile(const ExperimentParams& params) const
{
    ExperimentScanProfile profile;
    profile.expectedPointCount = calculateExpectedPointCount(params);
    profile.startHeightUm = static_cast<double>(params.scanRangeStart) * 1000.0;
    profile.stepUm = static_cast<double>(params.scanStep);
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
    context.sequence = ++m_nextScanSequences[channel];
    context.expectedPointCount = profile.expectedPointCount;
    context.savedPointCount = 0;
    context.startHeightUm = profile.startHeightUm;
    context.stepUm = profile.stepUm;
    context.startedAtMs = QDateTime::currentMSecsSinceEpoch();

    m_scanContexts[channel].append(context);
    refreshCurrentScanCount(channel);

    qDebug() << "[ExperimentSessionService][ScanCycle] channel=" << channel
             << "sequence=" << context.sequence
             << "expectedPointCount=" << context.expectedPointCount
             << "startHeightUm=" << context.startHeightUm
             << "stepUm=" << context.stepUm
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
                     << "sequence=" << completed.sequence
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
                                                             context.savedPointCount);
        dataList += batch;

        const int savedPairs = batch.size();
        context.savedPointCount += savedPairs;
        consumedPairs += savedPairs;

        qDebug() << "[ExperimentSessionService][ScanCycle] channel=" << channel
                 << "sequence=" << context.sequence
                 << "area=" << (areaA ? "A" : "B")
                 << "savedPairs=" << savedPairs
                 << "progress=" << context.savedPointCount << "/" << context.expectedPointCount;

        if (context.savedPointCount >= context.expectedPointCount) {
            const ScanCycleContext completed = contexts.first();
            contexts.remove(0);
            qDebug() << "[ExperimentSessionService][ScanCycle] channel=" << channel
                     << "sequence=" << completed.sequence
                     << "completed"
                     << "totalSavedPoints=" << completed.savedPointCount;
        }
    }

    refreshCurrentScanCount(channel);
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
                                                                 int startPointIndex) const
{
    QVector<QVariantMap> dataList;
    const int pairCount = raw.size() / 2;
    dataList.reserve(pairCount);
    const int baseTs = static_cast<int>(QDateTime::currentSecsSinceEpoch());

    for (int i = 0; i < pairCount; ++i) {
        const int pointIndex = startPointIndex + i;

        QVariantMap row;
        row["timestamp"] = baseTs;
        row["height"] = startHeightUm + (static_cast<double>(pointIndex) * stepUm);
        row["transmission_intensity"] = raw[(i * 2)] / 10.0;
        row["backscatter_intensity"] = raw[(i * 2) + 1] / 10.0;
        row["channel"] = channel;
        row["point_index"] = pointIndex;
        row["storage_area"] = areaA ? "A" : "B";
        dataList.append(row);
    }

    return dataList;
}
