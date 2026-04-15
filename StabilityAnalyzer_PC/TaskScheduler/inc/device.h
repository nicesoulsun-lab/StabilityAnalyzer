#ifndef DEVICE_H
#define DEVICE_H

#include <QObject>
#include <QString>
#include <QList>
#include <QJsonObject>
#include "task.h"
#include "taskscheduler_global.h"
/**
 * @brief 设备类
 * 
 * 表示一个Modbus设备，包含设备的配置信息和任务列表，负责：
 * - 设备属性管理（名称、描述、制造商等）
 * - 串口通信配置
 * - 任务列表管理
 * - 任务执行和状态跟踪
 * - 数据转换和缩放
 */
class TASKSCHEDULER_EXPORT Device : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString deviceId READ deviceId CONSTANT)
    Q_PROPERTY(QString deviceName READ deviceName CONSTANT)
    Q_PROPERTY(QString portName READ portName CONSTANT)
//    Q_PROPERTY(int slaveAddress READ slaveAddress CONSTANT)
    Q_PROPERTY(bool isConnected READ isConnected NOTIFY connectionStatusChanged)

public:
//    explicit Device(const QJsonObject &config, QObject *portManager = nullptr, QObject *parent = nullptr);
    explicit Device(const QString &deviceId, QObject *portManager = nullptr, QObject *parent = nullptr);

    // 属性访问器
    QString deviceId() const { return m_deviceId; }
    QString deviceName() const { return m_deviceName; }
    QString portName() const { return m_portName; }
//    int slaveAddress() const { return m_slaveAddress; }
    bool isConnected() const { return m_isConnected; }
    
    // 任务管理
    void addTask(Task *task);
    void addTasks(const QList<Task*> &tasks);
    void removeTask(const QString &taskId);
    QList<Task*> tasks() const { return m_tasks; }
    Task* findTask(const QString &taskId) const;
    
    // 初始化任务（启动时执行）
    void executeInitTasks();
    
    // 用户触发任务
    void executeUserTask(const QString &taskId);
    
    // 连接管理
    void connectToDevice();
    void disconnectFromDevice();
    
    // 获取Modbus客户端实例（用于任务执行）
    QObject* modbusClient() const { return m_modbusClient; }
    void setModbusClient(QObject *client) { m_modbusClient = client; }

signals:
    void connectionStatusChanged(bool connected);
    void taskCompleted(TaskResult res, QVector<quint16>data);
    void errorOccurred(const QString &error);

private slots:
//    void onTaskCompleted(bool success, const QVariant &result);
    void onTaskCompleted(TaskResult res, QVector<quint16>data);

private:
    QString m_deviceId;
    QString m_deviceName;
    QString m_portName;
//    int m_slaveAddress;
    bool m_isConnected;
    
    QObject *m_portManager;
    
    // Modbus客户端（由PortManager设置）
    QObject *m_modbusClient;
    
    // 任务列表
    QList<Task*> m_tasks;
    QHash<QString, Task*> m_taskMap;
};

#endif // DEVICE_H
