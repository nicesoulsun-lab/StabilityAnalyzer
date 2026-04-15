#ifndef MODBUSWORKER_H
#define MODBUSWORKER_H

#include <QObject>
#include <QSerialPort>
#include <QMutex>
#include <QTimer>
#include <QByteArray>
#include "qmodbusrtuunit_global.h"

class QMODBUSRTUUNIT_EXPORT ModbusWorker : public QObject
{
    Q_OBJECT

public:
    explicit ModbusWorker(QObject *parent = nullptr);
    ~ModbusWorker();

public slots:
    // 连接管理
    bool connectToDevice(const QString &portName, int baudRate, int dataBits, int parity, int stopBits);
    void disconnectFromDevice();
    
    // 数据发送
    bool sendRequest(const QByteArray &request);
    
    // 配置管理
    void setResponseTimeout(int timeout);
    
    // 工作线程控制
    void start();
    void stop();

signals:
    void dataReceived(const QByteArray &data);
    void errorOccurred(const QString &error);
    void connectionStateChanged(bool connected);

private slots:
    void onReadyRead();
    void onErrorOccurred(QSerialPort::SerialPortError error);
    void handleTimeout();

private:
    QSerialPort *m_serialPort;
    QTimer *m_timeoutTimer;
    QMutex m_mutex;
    QByteArray m_receiveBuffer;
    bool m_waitingForResponse;
    int m_responseTimeout;
};

#endif // MODBUSWORKER_H
