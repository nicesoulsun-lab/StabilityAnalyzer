#ifndef EXPERIMENT_CTRL_H
#define EXPERIMENT_CTRL_H

#include <QObject>
#include <QVariantMap>
#include <QVector>
#include <QTimer>
#include "mainwindow_global.h"
#include "modbustaskscheduler.h"

class SqlOrmManager;

/**
 * @class ExperimentCtrl
 * @brief 实验控制器类，负责管理实验参数、实验流程和Modbus通信
 * 
 * 该类实现了以下功能：
 * 1. 实验参数的保存和加载
 * 2. 实验的启动和停止控制
 * 3. 实验数据的记录和保存
 * 4. 通过TaskScheduler任务调度框架与4个下位机通道的Modbus通信
 * 5. 温度控制和扫描区间设置
 */
class MAINWINDOW_EXPORT ExperimentCtrl : public QObject
{
    Q_OBJECT

public:
    /**
     * @enum Channel
     * @brief 通道枚举，表示4个下位机通道
     */
    enum Channel {
        ChannelA = 0,  ///< 通道A
        ChannelB,      ///< 通道B
        ChannelC,      ///< 通道C
        ChannelD       ///< 通道D
    };
    Q_ENUM(Channel)

    /**
     * @struct ExperimentParams
     * @brief 实验参数结构体，包含所有实验配置参数
     */
    struct ExperimentParams {
        int projectId;                ///< 项目ID
        QString sampleName;           ///< 样品名称
        QString operatorName;         ///< 操作员姓名
        QString description;          ///< 实验描述
        
        int durationDays;             ///< 实验持续天数
        int durationHours;            ///< 实验持续小时数
        int durationMinutes;          ///< 实验持续分钟数
        int durationSeconds;          ///< 实验持续秒数
        
        int intervalHours;            ///< 扫描间隔小时数
        int intervalMinutes;          ///< 扫描间隔分钟数
        int intervalSeconds;          ///< 扫描间隔秒数
        
        int scanCount;                ///< 扫描次数（0表示无限次）
        
        bool temperatureControl;      ///< 是否启用温度控制
        double targetTemperature;     ///< 目标温度
        
        int scanRangeStart;        ///< 扫描区间起始值
        int scanRangeEnd;          ///< 扫描区间结束值

        int scanStep;              ///< 扫描步长
    };

    /**
     * @brief 构造函数
     * @param parent 父对象指针
     */
    explicit ExperimentCtrl(QObject *parent = nullptr);
    
    /**
     * @brief 析构函数
     */
    ~ExperimentCtrl();

    /**
     * @brief 保存实验参数
     * @param channel 通道
     * @param params 参数映射
     */
    Q_INVOKABLE void saveParams(int channel, const QVariantMap& params);
    
    /**
     * @brief 加载实验参数
     * @param channel 通道（0-3）
     * @return 参数映射
     */
    Q_INVOKABLE QVariantMap loadParams(int channel);

    /**
     * @brief 启动实验
     * @param channel 通道（0-3）
     * @param creatorId 创建者ID
     * @return 是否启动成功
     */
    Q_INVOKABLE bool startExperiment(int channel, int creatorId);
    
    /**
     * @brief 停止实验
     * @param channel 通道（0-3）
     * @return 是否停止成功
     */
    Q_INVOKABLE bool stopExperiment(int channel);
    
    /**
     * @brief 检查实验是否正在运行
     * @param channel 通道（0-3）
     * @return 是否正在运行
     */
    Q_INVOKABLE bool isExperimentRunning(int channel) const;

    /**
     * @brief 保存实验数据
     * @param experimentId 实验ID
     * @param data 实验数据
     */
    Q_INVOKABLE void saveExperimentData(int experimentId, const QVariantMap& data);
    
    /**
     * @brief 批量保存实验数据
     * @param experimentId 实验ID
     * @param dataList 实验数据列表
     */
    Q_INVOKABLE void batchSaveExperimentData(int experimentId, const QVector<QVariantMap>& dataList);

    /**
     * @brief 获取当前扫描次数
     * @param channel 通道（0-3）
     * @return 扫描次数
     */
    Q_INVOKABLE int getCurrentScanCount(int channel) const;
    
    /**
     * @brief 获取已运行时间
     * @param channel 通道（0-3）
     * @return 已运行时间（秒）
     */
    Q_INVOKABLE qint64 getElapsedTime(int channel) const;
    
    /**
     * @brief 设置串口配置（更新调度器配置并重连）
     * @param channel 通道（0-3）
     * @param portName 端口名
     * @param baudRate 波特率
     * @param dataBits 数据位
     * @param parity 校验位
     * @param stopBits 停止位
     */
    Q_INVOKABLE void setSerialConfig(int channel, const QString& portName, int baudRate, int dataBits, int parity, int stopBits);
    
    /**
     * @brief 设置Modbus从站地址
     * @param channel 通道（0-3）
     * @param slaveId 从站地址
     */
    Q_INVOKABLE void setSlaveId(int channel, int slaveId);
    
    /**
     * @brief 初始化并启动任务调度器
     * @param configDirPath 配置文件目录路径
     * @return 是否初始化成功
     */
    Q_INVOKABLE bool initializeScheduler(const QString& configDirPath = "");
    
    /**
     * @brief 连接指定通道的Modbus设备
     * @param channel 通道（0-3）
     * @return 是否连接成功
     */
    Q_INVOKABLE bool connectModbusDevice(int channel);
    
    /**
     * @brief 断开Modbus设备连接
     * @param channel 通道（0-3）
     */
    Q_INVOKABLE void disconnectModbusDevice(int channel);
    
    /**
     * @brief 检查Modbus设备是否已连接
     * @param channel 通道（0-3）
     * @return 是否已连接
     */
    Q_INVOKABLE bool isModbusConnected(int channel) const;
    
    /**
     * @brief 保存串口配置到文件
     * @param channel 通道（0-3）
     */
    Q_INVOKABLE void saveSerialConfig(int channel);
    
    /**
     * @brief 从文件加载串口配置
     * @param channel 通道（0-3）
     */
    Q_INVOKABLE void loadSerialConfig(int channel);

signals:
    /**
     * @brief 实验开始信号
     * @param channel 通道（0-3）
     * @param experimentId 实验ID
     */
    void experimentStarted(int channel, int experimentId);
    
    /**
     * @brief 实验停止信号
     * @param channel 通道（0-3）
     * @param experimentID 实验ID
     */
    void experimentStopped(int channel, int experimentId);
    
    /**
     * @brief 扫描完成信号
     * @param channel 通道（0-3）
     * @param scanCount 扫描次数
     * @param data 扫描数据
     */
    void scanCompleted(int channel, int scanCount, const QVariantMap& data);
    
    /**
     * @brief 实验错误信号
     * @param channel 通道（0-3）
     * @param error 错误信息
     */
    void experimentError(int channel, const QString& error);
    
    /**
     * @brief 操作信息信号
     * @param message 信息内容
     */
    void operationInfo(const QString& message);
    
    /**
     * @brief 操作失败信号
     * @param message 失败信息
     */
    void operationFailed(const QString& message);

private slots:
    /**
     * @brief 扫描定时器回调
     * @param channel 通道（0-3）
     */
    void onScanTimer(int channel);
    
    /**
     * @brief 实验超时回调
     * @param channel 通道（0-3）
     */
    void onExperimentTimeout(int channel);
    
    /**
     * @brief 任务完成回调（来自TaskScheduler）
     * @param res 任务结果
     * @param data 数据
     */
    void onSchedulerTaskCompleted(TaskResult res, QVector<quint16> data);

private:
    /**
     * @brief 计算总秒数
     * @param days 天数
     * @param hours 小时数
     * @param minutes 分钟数
     * @param seconds 秒数
     * @return 总秒数
     */
    int calculateTotalSeconds(int days, int hours, int minutes, int seconds) const;
    
    /**
     * @brief 获取通道键名
     * @param channel 通道
     * @return 键名字符串
     */
    QString getChannelKey(int channel) const;
    
    /**
     * @brief 获取通道对应的设备ID（slaveId字符串）
     * @param channel 通道
     * @return 设备ID字符串
     */
    QString getDeviceId(int channel) const;
    
    /**
     * @brief 发送控制命令（通过TaskScheduler执行用户任务）
     * @param channel 通道（0-3）
     * @param command 命令名
     * @param params 命令参数
     * @return 是否发送成功
     */
    bool sendControlCommand(int channel, const QString& command, const QVariantMap& params);
    
    /**
     * @brief 读取传感器数据（通过TaskScheduler执行读任务）
     * @param channel 通道（0-3）
     * @return 传感器数据映射
     */
    QVariantMap readSensorData(int channel);
    
    /**
     * @brief 生成默认JSON配置文件（用于4个实验通道）
     * @param configDirPath 配置目录路径
     */
    void generateDefaultConfig(const QString& configDirPath);

    SqlOrmManager* m_dbManager;                     ///< 数据库管理器指针
    ModbusTaskScheduler* m_scheduler;               ///< 任务调度器（单例引用）
    QMap<Channel, int> m_slaveIds;                  ///< 从站地址映射
    QMap<Channel, SerialConfig> m_serialConfigs;    ///< 串口配置映射
    QMap<Channel, ExperimentParams> m_params;       ///< 实验参数映射
    QMap<Channel, QTimer*> m_scanTimers;            ///< 扫描定时器映射
    QMap<Channel, QTimer*> m_experimentTimers;      ///< 实验定时器映射
    QMap<Channel, int> m_experimentIds;             ///< 实验ID映射
    QMap<Channel, int> m_currentScanCounts;         ///< 当前扫描次数映射
    QMap<Channel, qint64> m_startTimes;             ///< 实验开始时间映射
    QMap<Channel, bool> m_runningFlags;             ///< 实验运行状态映射
    bool m_schedulerInitialized;                    ///< 调度器是否已初始化
};

#endif
