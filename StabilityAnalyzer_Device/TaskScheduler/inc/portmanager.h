#ifndef PORTMANAGER_H
#define PORTMANAGER_H

#include <QObject>
#include <QString>
#include <QHash>
#include <QList>
#include <QSerialPort>
#include "device.h"
#include "../../QModbusRTUUnit/inc/modbus_client.h"
#include "struct.h"
#include "taskscheduler_global.h"

/**
 * @brief 串口管理器类
 *
 * 负责管理串口连接和设备通信，包括：
 * - 串口连接和断开管理
 * - 设备注册和查找
 * - 任务队列管理
 * - 通信状态监控
 * - 错误处理和重连机制
 */
class TASKSCHEDULER_EXPORT PortManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int portCount READ portCount NOTIFY portCountChanged)

public:
    explicit PortManager(QObject *parent = nullptr);
    ~PortManager();

    // 属性访问器
    int portCount() const { return m_ports.size(); }

    // 端口管理,这个地方暂时传递json，如果传递结构体的话会出现多层嵌套头文件的问题
//    bool addPort(const QString &portName, const QJsonObject &portConfig);
    bool addPort(const SerialConfig &portConfig);
    bool removePort(const QString &portName);
    bool isPortConnected(const QString &portName) const;

    // 设备管理
    bool addDeviceToPort(const QString &portName, Device *device);
    bool removeDeviceFromPort(const QString &portName, const QString &deviceId);
    QList<Device*> getDevicesOnPort(const QString &portName) const;

    // 连接管理
    bool connectPort(const QString &portName);
    bool disconnectPort(const QString &portName);

    // 任务执行
    void executeInitTasks();
    void executeUserTask(const QString &portName, const QString &deviceId, const QString &taskId);

    // 查找功能
    Device* findDevice(const QString &deviceId) const;
    QObject* getModbusClient(const QString &portName) const;

signals:
    void portCountChanged(int count);
    void portConnectionChanged(const QString &portName, bool connected);
//    void deviceTaskCompleted(const QString &deviceId, const QString &taskId, bool success, const QVariant &result);
    void deviceTaskCompleted(TaskResult res, QVector<quint16>data);
    void errorOccurred(const QString &error);

private slots:
//    void onDeviceTaskCompleted(const QString &taskId, bool success, const QVariant &result);
    void onDeviceTaskCompleted(TaskResult res, QVector<quint16>data);
    //监听modbusclient断开连接信号
    void onModbusClientDisconnected();
    //监听modbusclient断开连接信号
    void onModbusClientConnected();

private:
    struct PortInfo {
        QString portName;
        QSerialPort::BaudRate baudRate;
        QSerialPort::DataBits dataBits;
        QSerialPort::Parity parity;
        QSerialPort::StopBits stopBits;
        QSerialPort::FlowControl flowControl;
        bool isConnected;
        QObject *modbusClient;
        QList<Device*> devices;
    };

    //创建mosbusclient客户端
    QObject* createModbusClient(const QString &portName, const PortInfo &portInfo);
    //销毁
    void destroyModbusClient(const QString &portName);

    QHash<QString, PortInfo> m_ports; //串口名-串口信息字典
    QHash<QString, Device*> m_deviceMap; // 设备ID到设备的映射
};

#endif // PORTMANAGER_H
