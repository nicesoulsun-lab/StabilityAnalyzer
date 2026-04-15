#ifndef MODBUSRTUCLIENT_H
#define MODBUSRTUCLIENT_H

#include <QObject>
#include <QSerialPort>
#include <QThread>
#include <QMutex>
#include <QTimer>
#include <QVector>
#include <QFuture>
#include <QtConcurrent>
#include <QQueue>
#include <QDateTime>
#include "qmodbusrtuunit_global.h"
#include "ModbusConfig.h"

/**
 * @brief The ModbusRTUClient class
 * Modbus RTU 客户端通信类 - 重构版本
 * 提供完整的 Modbus RTU 协议实现，支持串口通信
 * 线程安全，支持多线程操作，异步操作
 */
class QMODBUSRTUUNIT_EXPORT ModbusRTUClient : public QObject
{
    Q_OBJECT

public:
    enum ConnectionState {
        Disconnected = 0,
        Connecting,
        Connected,
        Error
    };
    Q_ENUM(ConnectionState)

    enum ErrorCode {
        NoError = 0,
        ConnectionError,
        TimeoutError,
        ProtocolError,
        CRCError,
        InvalidParameter
    };
    Q_ENUM(ErrorCode)

    ModbusRTUClient(QObject *parent = nullptr);
    ~ModbusRTUClient();
    //清理资源
    void clearUpResource();
//    void start(QString portName);

    // 配置管理
    ModbusConfig* config() const;
    
    // 连接管理
//    bool connectToDevice(const QString &portName = QString());
//    void disconnectFromDevice();
    
    // 同步操作 - 阻塞式
    QVector<bool> readCoilsSync(int slaveAddress, int startAddress, int count, int timeout = -1);
    QVector<bool> readDiscreteInputsSync(int slaveAddress, int startAddress, int count, int timeout = -1);
    QVector<quint16> readHoldingRegistersSync(int slaveAddress, int startAddress, int count, int timeout = -1);
    QVector<quint16> readInputRegistersSync(int slaveAddress, int startAddress, int count, int timeout = -1);
    bool writeSingleCoilSync(int slaveAddress, int address, bool value, int timeout = -1);
    bool writeSingleRegisterSync(int slaveAddress, int address, quint16 value, int timeout = -1);
    bool writeMultipleCoilsSync(int slaveAddress, int startAddress, const QVector<bool> &values, int timeout = -1);
    bool writeMultipleRegistersSync(int slaveAddress, int startAddress, const QVector<quint16> &values, int timeout = -1);
    
    // 异步操作 - 信号槽方式
//    bool readCoils(int slaveAddress, int startAddress, int count);
//    bool readDiscreteInputs(int slaveAddress, int startAddress, int count);
//    bool readHoldingRegisters(int slaveAddress, int startAddress, int count);
//    bool readInputRegisters(int slaveAddress, int startAddress, int count);
//    bool writeSingleCoil(int slaveAddress, int address, bool value);
//    bool writeSingleRegister(int slaveAddress, int address, quint16 value);
//    bool writeMultipleCoils(int slaveAddress, int startAddress, const QVector<bool> &values);
//    bool writeMultipleRegisters(int slaveAddress, int startAddress, const QVector<quint16> &values);

    // 属性访问器
    ConnectionState connectionState() const;

public slots:
    void init();
    bool connectToDevice(const QString &portName = QString());
    void disconnectFromDevice();
    bool readCoils(int slaveAddress, int startAddress, int count);
    bool readDiscreteInputs(int slaveAddress, int startAddress, int count);
    bool readHoldingRegisters(int slaveAddress, int startAddress, int count);
    bool readInputRegisters(int slaveAddress, int startAddress, int count);
    bool writeSingleCoil(int slaveAddress, int address, bool value);
    bool writeSingleRegister(int slaveAddress, int address, quint16 value);
    bool writeMultipleCoils(int slaveAddress, int startAddress, const QVector<bool> &values);
    bool writeMultipleRegisters(int slaveAddress, int startAddress, const QVector<quint16> &values);

signals:
    void sig_Startinit();
    void connectionStateChanged(ConnectionState state);
    void errorOccurred(ErrorCode errorCode, const QString &errorString);
    
    // 线程间通信信号
    void requestConnect(const QString &portName);
    void requestDisconnect();
    
    // 数据接收信号
    void coilsRead(int slaveAddress, int startAddress, const QVector<bool> &values);
    void discreteInputsRead(int slaveAddress, int startAddress, const QVector<bool> &values);
    void holdingRegistersRead(int slaveAddress, int startAddress, const QVector<quint16> &values);
    void inputRegistersRead(int slaveAddress, int startAddress, const QVector<quint16> &values);
    void writeCompleted(int slaveAddress, int functionCode, int startAddress, int count);

private slots:
//    void startWork();
    void onReadyRead();
    void onErrorOccurred(QSerialPort::SerialPortError error);
    void handleTimeout();

private:
    // 请求结构体
    struct ModbusRequest {
        int slaveAddress;
        int functionCode;
        int startAddress;
        int count;
        QByteArray data;
        QDateTime timestamp;
        
        ModbusRequest(int slave, int func, int start, int cnt, const QByteArray &d = QByteArray())
            : slaveAddress(slave), functionCode(func), startAddress(start), count(cnt), data(d), timestamp(QDateTime::currentDateTime()) {}
    };
    
    // Modbus协议相关方法
    QByteArray buildRequestFrame(int slaveAddress, int functionCode, int startAddress, int count, const QByteArray &data = QByteArray());
    quint16 calculateCRC(const QByteArray &data);
    bool parseResponse(const QByteArray &response, int slaveAddress, int functionCode);
    bool sendRequest(const QByteArray &request, int slaveAddress, int functionCode);
    
    // 线程安全操作
    void setConnectionState(ConnectionState state);
    void setError(ErrorCode errorCode, const QString &errorString);
    
    // 异步操作处理
    bool processAsyncOperation(int slaveAddress, int functionCode, int startAddress, int count, const QByteArray &data = QByteArray());
    bool processQueuedRequest();
    void emitAsyncResult(int slaveAddress, int functionCode, int startAddress, int count, const QByteArray &response);

    //串口相关
    QSerialPort *m_serialPort;
    QString m_portName;
    mutable QMutex m_mutex;
    QTimer *m_timeoutTimer;
    ModbusConfig *m_config;
    
    // 配置参数
    ConnectionState m_connectionState;
    
    // 通信状态
    QByteArray m_receiveBuffer;
    bool m_waitingForResponse;
    int m_expectedSlaveAddress;
    int m_expectedFunctionCode;
    int m_expectedByteCount;
    
    // 异步操作状态
    QMutex m_asyncMutex;
    QWaitCondition m_asyncCondition;
    QByteArray m_asyncResponse;
    bool m_asyncResponseReceived;
    ErrorCode m_asyncError;
    
    // 请求队列
    QQueue<ModbusRequest> m_requestQueue;
    QMutex m_queueMutex;
};

#endif // MODBUSRTUCLIENT_H
