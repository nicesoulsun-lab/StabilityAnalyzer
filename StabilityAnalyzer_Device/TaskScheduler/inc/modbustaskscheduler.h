#ifndef MODBUSTASKSCHEDULER_H
#define MODBUSTASKSCHEDULER_H

#include <QObject>
#include <QString>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QTimer>
#include <QMap>
#include "portmanager.h"
#include "deviceconfig.h"
#include "taskqueuemanager.h"
#include "taskscheduler_global.h"
/**
 * @brief Modbus任务调度器主类
 *
 * 负责管理整个Modbus通信系统的任务调度，包括：
 * - 配置文件加载和解析
 * - 串口和设备管理
 * - 任务调度执行
 * - 连接状态监控
 * - 错误处理和日志记录
 */
class TASKSCHEDULER_EXPORT ModbusTaskScheduler : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool isRunning READ isRunning NOTIFY runningStatusChanged)
    Q_PROPERTY(int totalDevices READ totalDevices NOTIFY deviceCountChanged)
    Q_PROPERTY(int connectedDevices READ connectedDevices NOTIFY connectionStatusChanged)
    Q_PROPERTY(QString configFile READ configFile WRITE setConfigFile NOTIFY configFileChanged)

public:
    explicit ModbusTaskScheduler(QObject *parent = nullptr);
    static ModbusTaskScheduler*instance();

    // 属性访问器
    bool isRunning() const { return m_isRunning; }
    int totalDevices() const { return m_totalDevices; }
    int connectedDevices() const { return m_connectedDevices; }
    QString configFile() const { return m_configFile; }
    
    // 配置管理
    void setConfigFile(const QString &configFile);
    bool loadConfiguration();
    bool saveConfiguration(const QJsonObject &config);
    bool loadConfigurationFromDirectory(const QString &configDirPath);
    bool loadConfigurationFromFile(const QString &configFilePath);

    // 调度器控制
    bool startScheduler();
    bool stopScheduler();
    void pauseScheduler();
    void resumeScheduler();
    
    // 任务执行
    void executeInitTasks();
    void executeUserTask(const QString &slaveId, const QString &taskName,
                         int isSync, QVector<quint16>& result, const QVector<quint16>& writeData
                         , const QString& remark);
    bool waitForTaskCompletion(Task* task, QVector<quint16>& result, int timeoutMs = 5000);
    void schedulePollingTask(const QString &slaveId, const QString &taskName,
                             int isSync = 0, const QVector<quint16>& writeData = QVector<quint16>());
    //轮询任务的开启和停止
    void startPollingTasks();
    void stopPollingTasks();
    void pausePollingTask(const QString &deviceId, const QString &taskName);
    void resumePollingTask(const QString &deviceId, const QString &taskName);
    
    // 设备管理
    QList<QString> getDeviceList() const;
    QList<QString> getTaskList(const QString &deviceId) const;
    Device* getDevice(const QString &deviceId) const;
    
    // 状态查询
    QString getDeviceStatus(const QString &deviceId) const;
    QString getTaskStatus(const QString &deviceId, const QString &taskName) const;
    int getTaskInterval(const QString &deviceId, const QString &taskName) const;

public slots:
    void onConfigurationChanged(const QString &newConfigFile);

signals:
    void runningStatusChanged(bool isRunning);
    void deviceCountChanged(int count);
    void connectionStatusChanged(int connectedCount);
    void configFileChanged(const QString &configFile);
    void taskStarted(const QString &deviceId, const QString &taskName);
    //    void taskCompleted(const QString &deviceId, const QString &taskName, bool success, const QVariant &result);
    void taskCompleted(TaskResult res, QVector<quint16>data);
    void errorOccurred(const QString &error);
    void schedulerStarted();
    void schedulerStopped();

private slots:
    //    void onDeviceTaskCompleted(const QString &deviceId, const QString &taskName, bool success, const QVariant &result);
    void onDeviceTaskCompleted(TaskResult res, QVector<quint16>data);
    void onPortConnectionChanged(const QString &portName, bool connected);
    void updateConnectionStatistics();
    void onTaskStarted(const QString &deviceId, const QString &taskName);
    //    void onTaskCompleted(const QString &deviceId, const QString &taskName,
    //                         bool success, const QVariant &result);
    void onTaskCompleted(TaskResult res, QVector<quint16>data);
    void onQueueStatusChanged(int highPriorityCount, int pollingCount);
    void onPollingTimer();

private:
    bool parseConfiguration(const QJsonObject &config);
    bool validateConfiguration(const QJsonObject &config) const;
    //初始化串口连接，改为传递结构体
    void initializePortConnection(const SerialConfig &deviceConfig, const QString& deviceId);
    
    // 串口配置管理方法
    void updatePortConfigCache(const SerialConfig &serialConfig);
    bool isPortConfigChanged(const SerialConfig &serialConfig) const;
    void reconnectPortWithNewConfig(const SerialConfig &serialConfig);
    QString getPortNameForDevice(const QString &deviceId) const;
    QJsonObject getSerialConfigForDevice(const QString &deviceId) const;
    
    // 串口连接状态任务调度控制
    //暂停串口的任务调度
    void pauseTaskSchedulingForPort(const QString &portName);
    //继续串口的任务调度
    void resumeTaskSchedulingForPort(const QString &portName);
    //这个串口的任务调度是否是可用的
    bool isPortTaskSchedulingActive(const QString &portName) const;
    
    QString m_configFile;
    bool m_isRunning;
    bool m_isPaused;
    int m_totalDevices;
    int m_connectedDevices;
    QString m_lastUsedDeviceId; //上一次的设备id

    PortManager *m_portManager;
    QTimer *m_statusUpdateTimer;
    TaskQueueManager *m_taskQueueManager;  //< 任务队列管理器
    
    QHash<QString, QJsonObject> m_deviceConfigs;
    QHash<QString, QJsonObject> m_taskConfigs;
    QMap<QString, DeviceConfig*> m_deviceConfigMap;  //< 设备配置映射（设备ID,设备配置）
    QHash<QString, QTimer*> m_pollingTimers;  //< 轮询任务定时器映射（任务ID,定时器）
    
    // 串口配置缓存和映射
    QHash<QString, SerialConfig> m_portConfigCache;  //< 串口名称,串口配置缓存
    QHash<QString, QString> m_deviceToPortMap;      //< 设备ID,串口名称映射
    
    // 串口连接状态管理
    QHash<QString, bool> m_portConnectionStates;    //< 串口名称,连接状态映射
    QHash<QString, bool> m_portTaskSchedulingStates; //< 串口名称,任务调度状态映射
};
#define MODBUSTASKSCHEDULER ModbusTaskScheduler::instance()
#endif // MODBUSTASKSCHEDULER_H
