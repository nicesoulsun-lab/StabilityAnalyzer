#ifndef EXPERIMENT_DATA_SERVICE_H
#define EXPERIMENT_DATA_SERVICE_H

/**
 * @file experiment_data_service.h
 * @brief 实验存储区数据读取、裁剪与落库服务。
 */

#include <QVariantMap>
#include <QVector>
#include <functional>

#include "mainwindow_global.h"
#include "../../../TaskScheduler/inc/modbustaskscheduler.h"

class SqlOrmManager;

/**
 * @brief 管理存储区取数与实验数据落库。
 *
 * 该类负责：
 * 1. 从 A/B 存储区按既定任务分块读取原始寄存器；
 * 2. 根据 readable count 裁剪本次有效数据；
 * 3. 调用上层提供的“按扫描上下文构造行数据”回调；
 * 4. 写内存缓存并批量入库。
 */
class MAINWINDOW_EXPORT ExperimentDataService
{
public:
    using DeviceIdProvider = std::function<QString(int)>;
    using BuildRowsFn = std::function<QVector<QVariantMap>(int, const QVector<quint16>&, bool)>;
    using SendControlFn = std::function<bool(int, const QString&, const QVariantMap&)>;
    using CurrentScanCountFn = std::function<int(void)>;

    /**
     * @brief 构造数据服务。
     * @param dbManager ORM 管理器，用于实验数据写库。
     * @param scheduler 任务调度器，用于读取存储区任务。
     */
    ExperimentDataService(SqlOrmManager* dbManager, ModbusTaskScheduler* scheduler);

    /**
     * @brief 保存单条实验数据。
     */
    void saveExperimentData(int experimentId, const QVariantMap& data) const;

    /**
     * @brief 批量保存实验数据。
     */
    void batchSaveExperimentData(int experimentId, const QVector<QVariantMap>& dataList) const;

    /**
     * @brief 根据存储区状态尝试读取 A/B 区数据。
     *
     * 存储区仍按固定 500 个字读取，但只处理 readable count 对应的前 N 个数据。
     */
    void tryFetchStoredData(int channel,
                            int experimentId,
                            int storageAReadableCount,
                            int storageBReadableCount,
                            int storageAState,
                            int storageBState,
                            QVector<QVariantMap>* memoryCache,
                            const DeviceIdProvider& deviceIdProvider,
                            const BuildRowsFn& buildRowsFn,
                            const SendControlFn& sendControlFn,
                            const CurrentScanCountFn& currentScanCountFn) const;

private:
    QVector<QVariantMap> parseStoragePairs(int channel, const QVector<quint16>& raw, bool areaA,
                                           double startHeightUm, double stepUm,
                                           int startPointIndex) const;

    SqlOrmManager* m_dbManager;
    ModbusTaskScheduler* m_scheduler;
};

#endif
