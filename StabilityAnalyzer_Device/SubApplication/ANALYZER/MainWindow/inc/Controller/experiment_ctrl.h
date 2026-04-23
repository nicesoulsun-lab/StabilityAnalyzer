#ifndef EXPERIMENT_CTRL_H
#define EXPERIMENT_CTRL_H

#include <QObject>
#include <QVariantMap>
#include <QVector>
#include <QMap>
#include <QTimer>
#include "Common/experiment_types.h"
#include "mainwindow_global.h"
#include "modbustaskscheduler.h"

class SqlOrmManager;
class ExperimentCommService;
class ExperimentDataService;
class ExperimentStateStore;
class ExperimentSessionService;

/**
 * @brief 实验控制器（四通道）
 *
 * 设计目标：
 * 1. 所有与下位机的通信都通过任务调度器执行；
 * 2. 所有寄存器映射由 JSON taskList 管理，业务层只调用“任务名”；
 * 3. 对 QML 暴露统一接口，支持首页状态实时刷新与实验控制。
 */
class MAINWINDOW_EXPORT ExperimentCtrl : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief 通道枚举（与首页卡片、参数页、数据库记录保持一致）
     */
    enum Channel {
        ChannelA = 0,
        ChannelB,
        ChannelC,
        ChannelD
    };
    Q_ENUM(Channel)

    explicit ExperimentCtrl(QObject *parent = nullptr);
    ~ExperimentCtrl();

    // ==================== 参数管理（QML调用） ====================

    /**
     * @brief 保存某通道实验参数（持久化到QSettings）
     */
    Q_INVOKABLE void saveParams(int channel, const QVariantMap& params);

    /**
     * @brief 读取某通道实验参数（优先QSettings，必要时回填内存缓存）
     */
    Q_INVOKABLE QVariantMap loadParams(int channel);

    // ==================== 实验生命周期（QML调用） ====================

    /**
     * @brief 启动实验
     *
     * 关键流程：
     * 1) 参数校验；
     * 2) 确保调度器已初始化；
     * 3) 写数据库创建实验记录；
     * 4) 下发开始相关控制任务（含开始标志）；
     * 5) 刷新通道状态并发信号。
     */
    Q_INVOKABLE bool startExperiment(int channel, int creatorId);

    /**
     * @brief 停止实验
     *
     * 关键流程：停止计时器 -> 清开始标志 -> 更新数据库状态 -> 刷新首页状态。
     */
    Q_INVOKABLE bool stopExperiment(int channel);

    /**
     * @brief 查询通道实验是否运行（主机侧标志）
     */
    Q_INVOKABLE bool isExperimentRunning(int channel) const;

    // ==================== 数据写库（C++内部/QML可调用） ====================

    /**
     * @brief 保存单条实验数据
     */
    Q_INVOKABLE void saveExperimentData(int experimentId, const QVariantMap& data);

    /**
     * @brief 批量保存实验数据
     */
    Q_INVOKABLE void batchSaveExperimentData(int experimentId, const QVector<QVariantMap>& dataList);

    // ==================== 运行态查询（QML可调用） ====================

    /**
     * @brief 查询当前已采样/已入库计数
     */
    Q_INVOKABLE int getCurrentScanCount(int channel) const;

    /**
     * @brief 查询已运行时长（秒）
     */
    Q_INVOKABLE qint64 getElapsedTime(int channel) const;

    // ==================== 串口与从站配置 ====================

    /**
     * @brief 设置串口参数（内存）
     */
    Q_INVOKABLE void setSerialConfig(int channel, const QString& portName, int baudRate, int dataBits, int parity, int stopBits);

    /**
     * @brief 设置从站ID（内存）
     */
    Q_INVOKABLE void setSlaveId(int channel, int slaveId);

    /**
     * @brief 初始化任务调度器并加载 JSON 配置目录
     */
    Q_INVOKABLE bool initializeScheduler(const QString& configDirPath = "");

    /**
     * @brief 连接 Modbus 设备（通过调度器）
     */
    Q_INVOKABLE bool connectModbusDevice(int channel);

    /**
     * @brief 断开 Modbus 设备（当前无实验运行时停止调度器）
     */
    Q_INVOKABLE void disconnectModbusDevice(int channel);

    /**
     * @brief 查询 Modbus 连接状态
     */
    Q_INVOKABLE bool isModbusConnected(int channel) const;

    /**
     * @brief 保存串口配置到QSettings
     */
    Q_INVOKABLE void saveSerialConfig(int channel);

    /**
     * @brief 从QSettings加载串口配置
     */
    Q_INVOKABLE void loadSerialConfig(int channel);

    // ==================== 首页状态模型接口 ====================

    /**
     * @brief 获取某通道首页状态快照（给QML初次渲染）
     */
    Q_INVOKABLE QVariantMap getChannelStatus(int channel) const;

signals:
    // 实验生命周期信号
    void experimentStarted(int channel, int experimentId);
    void experimentStopped(int channel, int experimentId);

    // 扫描完成（兼容旧流程）
    void scanCompleted(int channel, int scanCount, const QVariantMap& data);

    // 异常与提示
    void experimentError(int channel, const QString& error);
    void operationInfo(const QString& message);
    void operationFailed(const QString& message);

    /**
     * @brief 首页状态更新推送（每秒轮询后按需触发）
     */
    void channelStatusUpdated(int channel, const QVariantMap& status);

private slots:
    // 旧扫描计时器回调（保留兼容）
    void onScanTimer(int channel);

    // 实验总时长超时回调
    void onExperimentTimeout(int channel);

    // 调度器任务完成通知
    void onSchedulerTaskCompleted(TaskResult res, QVector<quint16> data);

    // 每秒轮询状态主入口
    void onStatusPollTimer();
    void onChannelStatusPollTimer(int channel);

private:
    // ==================== 工具方法 ====================

    /**
     * @brief 通道转从站设备ID字符串（给调度器）
     */
    QString getDeviceId(int channel) const;

    /**
     * @brief 下发控制命令（任务名 -> 调度器执行）
     */
    bool sendControlCommand(int channel, const QString& command, const QVariantMap& params);

    /**
     * @brief 读取传感器数据（当前基于实时状态字段组装）
     */
    QVariantMap readSensorData(int channel);

    /**
     * @brief 读取实时状态（全部通过JSON任务名）
     */
    QVariantMap readRealtimeStatus(int channel);
    void pollChannelStatus(int channel);
    void initializeSchedulerAfterStartup();
    void startDeferredStatusPolling();

    /**
     * @brief 根据存储区状态尝试取数（A/B区）
     */
    void tryFetchStoredData(int channel, int storageAReadableCount, int storageBReadableCount,
                            int storageAState, int storageBState);

    /**
     * @brief 生成默认JSON配置（设备+taskList）
     */
    void generateDefaultConfig(const QString& configDirPath);

private:
    // 数据库与调度器入口
    SqlOrmManager* m_dbManager;
    ModbusTaskScheduler* m_scheduler;
    ExperimentStateStore* m_stateStore;
    ExperimentSessionService* m_sessionService;
    ExperimentCommService* m_commService;
    ExperimentDataService* m_dataService;

    // 通道运行计时器
    QMap<Channel, QTimer*> m_scanTimers;
    QMap<Channel, QTimer*> m_experimentTimers;

    // 通道运行态
    QMap<Channel, int> m_experimentIds;
    QMap<Channel, qint64> m_startTimes;
    QMap<Channel, bool> m_runningFlags;

    // 调度器初始化状态
    bool m_schedulerInitialized;

    // 每通道独立轮询定时器（错峰触发）
    QMap<Channel, QTimer*> m_statusPollTimers;
};

#endif
