#include "inc/Common/experiment_data_service.h"

/**
 * @file experiment_data_service.cpp
 * @brief ExperimentDataService 的实现。
 */

#include "../../../SqlOrm/inc/SqlOrmManager.h"

#include <QDateTime>
#include <QDebug>

namespace {
constexpr int kStorageChunkRegisterCount = 100;
constexpr int kStorageMaxPairCount = 250;
constexpr int kStorageMaxDataCount = kStorageMaxPairCount * 2;
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
void ExperimentDataService::saveExperimentData(int experimentId, const QVariantMap& data) const
{
    if (!m_dbManager) {
        return;
    }

    QVariantMap payload = data;
    payload["experiment_id"] = experimentId;
    m_dbManager->addExperimentData(payload);
}

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
                                               const CurrentScanCountFn& currentScanCountFn) const
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

        const int readableDataCount = qBound(0, readableCount, kStorageMaxDataCount);
        if (readableDataCount <= 0) {
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

        bool hasTakenAnyBatch = false;
        int totalWords = 0;
        int totalPairs = 0;
        int remainingDataCount = readableDataCount;

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

            const int wantedWords = qMax(0, qMin(kStorageChunkRegisterCount, remainingDataCount));
            const int effectiveWords = qMin(raw.size(), wantedWords);
            const int alignedWords = effectiveWords - (effectiveWords % 2);
            const QVector<quint16> sliced = raw.mid(0, alignedWords);
            remainingDataCount -= effectiveWords;
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

            const QVector<QVariantMap> batch = buildRowsFn(channel, sliced, areaA);
            if (!batch.isEmpty()) {
                hasTakenAnyBatch = true;
                *memoryCache += batch;
                batchSaveExperimentData(experimentId, batch);
                totalPairs += batch.size();
                qDebug() << "[ExperimentDataService][Fetch] channel=" << channel
                         << (areaA ? "A" : "B")
                         << "batchPairs=" << batch.size()
                         << "scanCount=" << (currentScanCountFn ? currentScanCountFn() : 0);
            }
        }

        if (hasTakenAnyBatch) {
            sendControlFn(channel, ackCommand, {{"value", kStorageTakenState}});
            qDebug() << "[ExperimentDataService][Fetch] channel=" << channel
                     << (areaA ? "A" : "B")
                     << "readableDataCount=" << readableDataCount
                     << "done words=" << totalWords
                     << "pairs=" << totalPairs
                     << "ackState=" << kStorageTakenState;
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
QVector<QVariantMap> ExperimentDataService::parseStoragePairs(int channel, const QVector<quint16>& raw,
                                                              bool areaA, double startHeightUm,
                                                              double stepUm, int startPointIndex) const
{
    QVector<QVariantMap> dataList;
    const int pairCount = raw.size() / 2;
    dataList.reserve(pairCount);
    const int baseTs = static_cast<int>(QDateTime::currentSecsSinceEpoch());

    for (int i = 0; i < pairCount; ++i) {
        QVariantMap row;
        row["timestamp"] = baseTs;
        row["height"] = startHeightUm + (static_cast<double>(startPointIndex + i) * stepUm);
        row["transmission_intensity"] = raw[(i * 2)] / 10.0;
        row["backscatter_intensity"] = raw[(i * 2) + 1] / 10.0;
        row["channel"] = channel;
        row["point_index"] = startPointIndex + i;
        row["storage_area"] = areaA ? "A" : "B";
        dataList.append(row);
    }

    return dataList;
}
