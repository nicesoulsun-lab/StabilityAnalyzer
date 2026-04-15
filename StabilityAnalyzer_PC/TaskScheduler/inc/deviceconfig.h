#ifndef DEVICECONFIG_H
#define DEVICECONFIG_H

#include <QObject>
#include <QString>
#include <QJsonObject>
#include <QList>
#include <QMap>
#include "taskscheduler_global.h"
#include "struct.h"
class Task;

/**
 * @brief 设备配置类
 * 
 * 负责管理单个设备的配置信息，包括：
 * - 设备通用信息（ID、名称、描述等）
 * - 通信配置（串口参数、Modbus参数等）
 * - 任务列表（轮询任务和用户任务）
 * - 设备状态管理
 */
class TASKSCHEDULER_EXPORT DeviceConfig : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString deviceId READ deviceId CONSTANT)
    Q_PROPERTY(QString deviceName READ deviceName CONSTANT)
    Q_PROPERTY(bool isConnected READ isConnected NOTIFY connectionStatusChanged)

public:
    explicit DeviceConfig(QObject *portManager = nullptr, QObject *parent = nullptr);
    
    // 属性访问器
    QString deviceId() const { return m_deviceId; }
    QString deviceName() const { return m_deviceName; }
    QString description() const { return m_description; }
    QString manufacturer() const { return m_manufacturer; }
    QString model() const { return m_model; }
    bool isConnected() const { return m_isConnected; }
    
    // 通信配置访问器
//    QString portName() const { return m_portName; }
//    int baudRate() const { return m_baudRate; }
//    int slaveId() const { return m_slaveId; }
    
    // 任务管理
    //读取配置文件，获取通用信息，串口信息，任务信息，获取任务信息的时候会直接new创建出来任务对象
    bool loadFromJson(const QJsonObject &config);
    QList<Task*> tasks() const { return m_tasks; }
    QList<Task*> initTasks() const;  //< 获取初始化任务（轮询任务）
    QList<Task*> userTasks() const;  //< 获取用户任务
    Task* findTask(const QString &taskName) const;
    
    // 连接状态管理
    void setConnected(bool connected);

    const SerialConfig &serialConfig() const;

    QObject *portManager() const;

signals:
    void connectionStatusChanged(bool connected);
    void configurationLoaded();
    void errorOccurred(const QString &error);

private:
    //解析配置文件数据
    void parseGeneralInfo(const QJsonObject &config);
    void parseSerialConfig(const QJsonObject &config);
    void parseTaskList(const QJsonArray &tasksArray);
    
    // 设备通用信息
    QString m_deviceId;
    QString m_deviceName;
    QString m_description;
    QString m_manufacturer;
    QString m_model;
    bool m_isConnected;
    int m_interFrameDelay;
    int m_responseTimeout;
    int m_maxRetries;

    QObject *m_portManager;
    
    // 通信配置
//    QString m_portName;
//    int m_baudRate;
//    int m_dataBits;
//    QString m_parity;
//    int m_stopBits;
//    QString m_flowControl;
    SerialConfig m_serialConfig;
//    int m_slaveId;

    // 任务列表
    QList<Task*> m_tasks; //所有的任务信息集合
    QMap<QString, Task*> m_taskMap;  //< 任务名称到任务对象的映射
};

#endif // DEVICECONFIG_H
