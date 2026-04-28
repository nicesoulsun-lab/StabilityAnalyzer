#include "inc/Common/experiment_data_service.h"

/**
 * @file experiment_data_service.cpp
 * @brief ExperimentDataService 的实现。
 */

#include "../../../SqlOrm/inc/SqlOrmManager.h"

#include <QDebug>

namespace {
constexpr int kStorageChunkRegisterCount = 100;
constexpr int kStorageMaxWordCount = 500;
constexpr int kStorageMaxPairCount = kStorageMaxWordCount / 2;
constexpr int kStorageReadyState = 2;
constexpr int kStorageTakenState = 3;

/**
 * @brief 判断存储区状态是否允许主机取数。
 */
bool isStorageReadable(int state)
{
    return state == kStorageReadyState;
}
}

ExperimentDataService::ExperimentDataService(SqlOrmManager* dbManager, ModbusTaskScheduler* scheduler)
    : m_dbManager(dbManager)
    , m_scheduler(scheduler)
{
}

/**
 * @brief 保存单条实验数据。
 */

/**
 * @brief 批量保存实验数据。
 */
void ExperimentDataService::batchSaveExperimentData(int experimentId, const QVector<QVariantMap>& dataList) const
{
    if (!m_dbManager) {
        return;
    }

    QVector<QVariantMap> payload;
    payload.reserve(dataList.size());
    for (const QVariantMap& item : dataList) {
        QVariantMap row = item;
        row["experiment_id"] = experimentId;
        payload.append(row);
    }
    m_dbManager->batchAddExperimentData(payload);
}

/**
 * @brief 从 A/B 存储区读取数据并写入缓存与数据库。
 */
void ExperimentDataService::tryFetchStoredData(int channel,
                                               int experimentId,
                                               int storageAReadableCount,
                                               int storageBReadableCount,
                                               int storageAState,
                                               int storageBState,
                                               QVector<QVariantMap>* memoryCache,
                                               const DeviceIdProvider& deviceIdProvider,
                                               const BuildRowsFn& buildRowsFn,
                                               const SendControlFn& sendControlFn,
                                               const CurrentScanCountFn& currentScanCountFn,
                                               const StreamRowsFn& streamRowsFn) const
{
    if (!m_scheduler || experimentId <= 0 || !memoryCache || !buildRowsFn || !sendControlFn) {
        return;
    }

    const QString deviceId = deviceIdProvider ? deviceIdProvider(channel) : QString::number(channel + 1);

    auto fetchArea = [&](bool areaA, int readableCount, int initialState) {
        if (!isStorageReadable(initialState)) {
            qDebug() << "[ExperimentDataService][Fetch] channel=" << channel
                     << (areaA ? "A" : "B") << "state=" << initialState << "skip(not readable)";
            return;
        }

        // Lower device reports the readable size in "values/registers".
        // Two consecutive registers form one transmission/backscatter pair.
        const int readableWordCount = qBound(0, readableCount, kStorageMaxWordCount);
        const int readablePairCount = readableWordCount / 2;
        if (readableWordCount <= 0) {
            qWarning() << "[ExperimentDataService][Fetch] channel=" << channel
                       << (areaA ? "A" : "B") << "state=readable but readableCount=0";
            return;
        }

        const QVector<QString> readPlans = areaA
                ? QVector<QString>{"read_scan_data_a_0", "read_scan_data_a_100", "read_scan_data_a_200",
                                   "read_scan_data_a_300", "read_scan_data_a_400"}
                : QVector<QString>{"read_scan_data_b_500", "read_scan_data_b_600", "read_scan_data_b_700",
                                   "read_scan_data_b_800", "read_scan_data_b_900"};
        const QString ackCommand = areaA ? "write_storage_a_state" : "write_storage_b_state";

        bool hasReadAnyBatch = false;
        bool hasSavedAnyBatch = false;
        int totalWords = 0;
        int totalPairs = 0;
        int remainingWordCount = readableWordCount;

        for (const QString& taskName : readPlans) {
            QVector<quint16> raw;
            m_scheduler->executeUserTask(deviceId, taskName, 1, raw, {}, taskName);
            qDebug() << "[ExperimentDataService][Fetch] channel=" << channel
                     << (areaA ? "A" : "B")
                     << "task=" << taskName
                     << "rawSize=" << raw.size();
            if (raw.isEmpty()) {
                continue;
            }

            const int wantedWords = qMax(0, qMin(kStorageChunkRegisterCount, remainingWordCount));
            const int effectiveWords = qMin(raw.size(), wantedWords);
            const int alignedWords = effectiveWords - (effectiveWords % 2);
            const QVector<quint16> sliced = raw.mid(0, alignedWords);
            remainingWordCount -= effectiveWords;
            totalWords += sliced.size();

            if (sliced.isEmpty()) {
                if (wantedWords > 0 && (effectiveWords % 2) != 0) {
                    qWarning() << "[ExperimentDataService][Fetch] channel=" << channel
                               << (areaA ? "A" : "B")
                               << "odd readable data count ignored last word, task=" << taskName
                               << "wantedWords=" << wantedWords;
                }
                continue;
            }

            hasReadAnyBatch = true;
            const QVector<QVariantMap> batch = buildRowsFn(channel, sliced, areaA);
            if (!batch.isEmpty()) {
                hasSavedAnyBatch = true;
                *memoryCache += batch;
                batchSaveExperimentData(experimentId, batch);
                if (streamRowsFn) {
                    streamRowsFn(channel, experimentId, batch);
                }
                totalPairs += batch.size();
                qDebug() << "[ExperimentDataService][Fetch] channel=" << channel
                         << (areaA ? "A" : "B")
                         << "batchPairs=" << batch.size()
                         << "scanCount=" << (currentScanCountFn ? currentScanCountFn() : 0);
            }
        }

        const bool fullyFetched = (totalWords >= readableWordCount);
        if (hasReadAnyBatch && fullyFetched) {
            const bool ackWritten = sendControlFn(channel, ackCommand, {{"value", kStorageTakenState}});
            qDebug() << "[ExperimentDataService][Fetch] channel=" << channel
                     << (areaA ? "A" : "B")
                     << "readableCount=" << readableWordCount
                     << "readablePairCount=" << readablePairCount
                     << "readableWordCount=" << readableWordCount
                     << "done words=" << totalWords
                     << "pairs=" << totalPairs
                     << "savedRows=" << hasSavedAnyBatch
                     << "ackWritten=" << ackWritten
                     << "ackState=" << kStorageTakenState;
        } else if (hasReadAnyBatch) {
            qWarning() << "[ExperimentDataService][Fetch] channel=" << channel
                       << (areaA ? "A" : "B")
                       << "partial fetch, keep READY state"
                       << "readableCount=" << readableWordCount
                       << "readablePairCount=" << readablePairCount
                       << "readableWordCount=" << readableWordCount
                       << "done words=" << totalWords
                       << "pairs=" << totalPairs
                       << "savedRows=" << hasSavedAnyBatch;
        } else {
            qWarning() << "[ExperimentDataService][Fetch] channel=" << channel
                       << (areaA ? "A" : "B") << "readable but no data";
        }
    };

    fetchArea(true, storageAReadableCount, storageAState);
    fetchArea(false, storageBReadableCount, storageBState);
}

/**
 * @brief 将原始寄存器数据解析为透射/散射记录。
 *
 * 该函数当前保留用于后续复用，现阶段主要由会话服务负责高度分配。
 */
