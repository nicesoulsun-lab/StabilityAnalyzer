#ifndef MODBUS_TYPES_H
#define MODBUS_TYPES_H

#include <QObject>
#include <QDateTime>
#include <QVector>
#include <QMap>
#include <QSerialPort>
#include <QtGlobal>
#include <cstdint>
#include <QMetaType>

/**
 * @brief Modbus异常代码定义 (符合Modbus标准)
 */
enum ModbusException {
    NoException                       = 0x00,
    IllegalFunction                   = 0x01,
    IllegalDataAddress                = 0x02,
    IllegalDataValue                  = 0x03,
    ServerDeviceFailure               = 0x04,
    Acknowledge                       = 0x05,
    ServerDeviceBusy                  = 0x06,
    NegativeAcknowledge               = 0x07,
    MemoryParityError                 = 0x08,
    GatewayPathUnavailable            = 0x0A,
    GatewayTargetDeviceFailedToRespond = 0x0B,

    // 扩展错误码
    TimeoutError                      = 0x80,
    CRCCheckError                     = 0x81,
    FrameError                        = 0x82,
    PortError                         = 0x83,
    BufferOverflowError               = 0x84,
    RequestQueueFull                  = 0x85,
    ConnectionError                   = 0x86,
    InvalidResponse                   = 0x87,
    SendError                         = 0x88
};

/**
 * @brief Modbus功能码定义
 */
enum ModbusFunctionCode {
    ReadCoils                         = 0x01,
    ReadDiscreteInputs                = 0x02,
    ReadHoldingRegisters              = 0x03,
    ReadInputRegisters                = 0x04,
    WriteSingleCoil                   = 0x05,
    WriteSingleRegister               = 0x06,
    WriteMultipleCoils                = 0x0F,
    WriteMultipleRegisters            = 0x10,

    // 扩展功能码
    MaskWriteRegister                 = 0x16,
    ReadWriteMultipleRegisters        = 0x17,
    ReadFIFOQueue                     = 0x18
};

/**
 * @brief 请求优先级定义
 */
enum RequestPriority : int {
    Low        = 0,      // 低优先级：后台数据采集、日志记录等
    Normal     = 1,      // 正常优先级：用户操作、常规数据读取
    High       = 2,      // 高优先级：实时数据显示、控制命令
    Critical   = 3,      // 紧急优先级：报警处理、安全相关
    System     = 4       // 系统优先级：配置读取、心跳检测
};

/**
 * @brief 请求状态定义
 */
enum class RequestStatus {
    Pending,           // 等待处理
    Queued,            // 已入队
    Sending,           // 正在发送
    WaitingResponse,   // 等待响应
    Processing,        // 处理中
    Completed,         // 已完成
    Failed,            // 失败
    Cancelled,         // 已取消
    Timeout,           // 超时
    Retrying           // 重试中
};

/**
 * @brief 连接状态定义
 */
enum class ConnectionState {
    Disconnected,      // 未连接
    Connecting,        // 连接中
    Connected,         // 已连接
    Disconnecting,     // 断开中
    Error              // 错误状态
};

/**
 * @brief Modbus数据值类型
 */
struct ModbusValue {
    quint16 value;           // 原始值
    qreal scaledValue;       // 缩放后的值
    QDateTime timestamp;     // 时间戳
    bool isValid;            // 是否有效

    ModbusValue() : value(0), scaledValue(0.0), isValid(false) {}
    ModbusValue(quint16 v) : value(v), scaledValue(v), isValid(true) {}
};

/**
 * @brief 寄存器定义
 */
struct RegisterInfo {
    int slaveId;            // 从站地址
    int address;            // 寄存器地址
    QString name;           // 寄存器名称
    QString description;    // 描述
    QString unit;           // 单位
    qreal scaleFactor;      // 缩放因子
    qreal offset;           // 偏移量
    int decimalPlaces;      // 小数位数
    bool isSigned;          // 是否为有符号数

    RegisterInfo() : slaveId(1), address(0), scaleFactor(1.0),
                     offset(0.0), decimalPlaces(0), isSigned(false) {}
};

/**
 * @brief Modbus请求结果
 */
struct ModbusResult {
    bool success;                    // 是否成功
    ModbusException exception;       // 异常代码
//    QVector<ModbusValue> values;     // 读取的值
    QVector<quint16> values;     // 读取的值
    QByteArray rawResponse;          // 原始响应数据
    qint64 requestId;                // 请求ID
    qint64 responseTime;             // 响应时间(ms)
    QString errorString;             // 错误描述

    // 统计信息
    int retryCount;                  // 重试次数
    qint64 queueTime;                // 排队时间(ms)
    qint64 processTime;              // 处理时间(ms)

    ModbusResult() : success(false), exception(ModbusException::NoException),
                    requestId(-1), responseTime(0), retryCount(0),
                    queueTime(0), processTime(0) {}
};
Q_DECLARE_METATYPE(ModbusResult)

/**
 * @brief 通信统计信息
 */
struct CommunicationStats {
    // 请求统计
    qint64 totalRequests;
    qint64 successfulRequests;
    qint64 failedRequests;
    qint64 timeoutErrors;
    qint64 crcErrors;
    qint64 frameErrors;

    // 响应时间统计
    qint64 minResponseTime;
    qint64 maxResponseTime;
    qint64 avgResponseTime;
    qint64 totalResponseTime;

    // 队列统计
    qint64 maxQueueSize;
    qint64 currentQueueSize;
    qint64 queueOverflows;

    // 连接统计
    qint64 connectionAttempts;
    qint64 disconnectionCount;
    qint64 totalConnectedTime;

    CommunicationStats() {
        reset();
    }

    void reset() {
        totalRequests = 0;
        successfulRequests = 0;
        failedRequests = 0;
        timeoutErrors = 0;
        crcErrors = 0;
        frameErrors = 0;

        minResponseTime = (std::numeric_limits<qint64>::max)();
        maxResponseTime = 0;
        avgResponseTime = 0;
        totalResponseTime = 0;

        maxQueueSize = 0;
        currentQueueSize = 0;
        queueOverflows = 0;

        connectionAttempts = 0;
        disconnectionCount = 0;
        totalConnectedTime = 0;
    }
};

/**
 * @brief 串口配置
 */
struct SerialPortConfig {
    QString portName;                        // 串口名称
    qint32 baudRate;                         // 波特率
    QSerialPort::DataBits dataBits;          // 数据位
    QSerialPort::Parity parity;              // 校验位
    QSerialPort::StopBits stopBits;          // 停止位
    QSerialPort::FlowControl flowControl;    // 流控制
    int interFrameDelay;                     // 帧间延迟(ms)
    int responseTimeout;                     // 响应超时(ms)
    int maxRetries;                          // 最大重试次数

    SerialPortConfig() {
        portName = "COM1";
        baudRate = 9600;
        dataBits = QSerialPort::Data8;
        parity = QSerialPort::NoParity;
        stopBits = QSerialPort::OneStop;
        flowControl = QSerialPort::NoFlowControl;
        interFrameDelay = 5;
        responseTimeout = 1000;
        maxRetries = 3;
    }

    bool operator==(const SerialPortConfig& other) const {
        return portName == other.portName &&
               baudRate == other.baudRate &&
               dataBits == other.dataBits &&
               parity == other.parity &&
               stopBits == other.stopBits &&
               flowControl == other.flowControl;
    }

    bool operator!=(const SerialPortConfig& other) const {
        return portName != other.portName ||
               baudRate != other.baudRate ||
               dataBits != other.dataBits ||
               parity != other.parity ||
               stopBits != other.stopBits ||
               flowControl != other.flowControl ||
               interFrameDelay != other.interFrameDelay ||
               responseTimeout != other.responseTimeout ||
               maxRetries != other.maxRetries;
    }
};

//// 显式模板实例化声明，解决QFutureWatcher虚拟表链接错误
//extern template class QFutureWatcher<ModbusResult>;

#endif // MODBUS_TYPES_H
