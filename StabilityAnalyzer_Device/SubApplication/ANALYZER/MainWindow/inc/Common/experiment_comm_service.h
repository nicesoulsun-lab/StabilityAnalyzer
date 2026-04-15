#ifndef EXPERIMENT_COMM_SERVICE_H
#define EXPERIMENT_COMM_SERVICE_H

/**
 * @file experiment_comm_service.h
 * @brief 实验控制与任务调度器之间的通信适配层。
 */

#include <QVariantMap>
#include <QVector>
#include <functional>

#include "mainwindow_global.h"
#include "modbustaskscheduler.h"

/**
 * @brief 封装实验业务层与 ModbusTaskScheduler 的交互。
 *
 * 该类负责：
 * 1. 调度器初始化与默认配置生成；
 * 2. 控制命令到任务名的映射；
 * 3. 实时状态块读取与字段组装。
 *
 * 该类不管理实验生命周期，也不负责数据落库。
 */
class MAINWINDOW_EXPORT ExperimentCommService
{
public:
    using SerialConfigProvider = std::function<SerialConfig(int)>;
    using SlaveIdProvider = std::function<int(int)>;
    using DeviceIdProvider = std::function<QString(int)>;
    using RunningCheck = std::function<bool(void)>;

    /**
     * @brief 构造通信服务。
     * @param scheduler 共享的任务调度器实例。
     */
    explicit ExperimentCommService(ModbusTaskScheduler* scheduler);

    /**
     * @brief 初始化调度器并确保默认 JSON 配置存在。
     */
    bool initializeScheduler(const QString& configDirPath,
                             const SerialConfigProvider& serialConfigProvider,
                             const SlaveIdProvider& slaveIdProvider,
                             bool* schedulerInitialized) const;

    /**
     * @brief 在配置文件不存在时生成四通道默认配置。
     */
    void generateDefaultConfig(const QString& configDirPath,
                               const SerialConfigProvider& serialConfigProvider,
                               const SlaveIdProvider& slaveIdProvider) const;

    /**
     * @brief 启动或恢复 Modbus 调度器。
     */
    bool connectModbusDevice(int channel, bool* schedulerInitialized, const QString& defaultConfigPath,
                             const SerialConfigProvider& serialConfigProvider,
                             const SlaveIdProvider& slaveIdProvider) const;

    /**
     * @brief 在没有实验运行时停止调度器。
     */
    void disconnectModbusDevice(bool schedulerInitialized, const RunningCheck& anyChannelRunning) const;

    /**
     * @brief 查询指定通道对应设备的连接状态。
     */
    bool isModbusConnected(int channel, bool schedulerInitialized,
                           const DeviceIdProvider& deviceIdProvider) const;

    /**
     * @brief 下发控制命令。
     *
     * 业务层传入抽象命令名，本类负责映射到具体任务名与写入数据。
     */
    bool sendControlCommand(int channel, const QString& command, const QVariantMap& params,
                            bool schedulerInitialized,
                            const DeviceIdProvider& deviceIdProvider) const;

    /**
     * @brief 读取 0~23 状态块并组装为业务字段。
     */
    QVariantMap readRealtimeStatus(int channel, bool schedulerInitialized,
                                   const DeviceIdProvider& deviceIdProvider) const;

    /**
     * @brief 读取首页需要的实时传感器数据。
     */
    QVariantMap readSensorData(int channel, bool schedulerInitialized,
                               const DeviceIdProvider& deviceIdProvider) const;

private:
    ModbusTaskScheduler* m_scheduler;
};

#endif
