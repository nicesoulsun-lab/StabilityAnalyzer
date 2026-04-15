#ifndef MODBUSRTUCLIENT_H
#define MODBUSRTUCLIENT_H

#include <QObject>
#include <QSerialPort>
#include <QTimer>
#include <QMutex>
#include <QQueue>
#include <QFuture>
#include <QtConcurrent/QtConcurrent>
#include "modbus_types.h"
#include "qmodbusrtuunit_global.h"
/**
 * @brief 异步操作结果结构体
 * 用于存储异步Modbus操作的结果信息
 */
struct AsyncResult {
    bool success;               ///< 操作是否成功
    ModbusException exception;  ///< 异常代码（如果操作失败）
    QVector<quint16> data;      ///< 读取的数据（仅适用于读取操作）
    QString errorString;        ///< 错误描述信息
};

/**
 * @brief Modbus RTU客户端类
 * 提供同步和异步的Modbus RTU通信功能，支持所有标准Modbus功能码
 * 该类封装了串口通信、CRC校验、超时重试等底层细节
 */
class QMODBUSRTUUNIT_EXPORT ModbusRtuClient : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param parent 父对象指针
     */
    explicit ModbusRtuClient(QObject *parent = nullptr);
    
    /**
     * @brief 析构函数
     * 自动断开连接并清理资源
     */
    ~ModbusRtuClient();

    /**
     * @brief 串口配置结构体
     * 用于配置串口通信参数
     */
    struct SerialConfig {
        QString portName;                       ///< 串口名称（如"COM1"）
        qint32 baudRate;                        ///< 波特率（如9600, 115200）
        QSerialPort::DataBits dataBits;         ///< 数据位（5,6,7,8）
        QSerialPort::Parity parity;             ///< 校验位（无校验、奇校验、偶校验）
        QSerialPort::StopBits stopBits;         ///< 停止位（1, 1.5, 2）
        QSerialPort::FlowControl flowControl;  ///< 流控制（无、硬件、软件）

        /**
         * @brief 默认构造函数
         * 使用COM1、9600波特率、8数据位、无校验、1停止位、无流控制的默认配置
         */
        SerialConfig() :
            portName("COM1"),
            baudRate(9600),
            dataBits(QSerialPort::Data8),
            parity(QSerialPort::NoParity),
            stopBits(QSerialPort::OneStop),
            flowControl(QSerialPort::NoFlowControl) {}
    };

    // 连接管理
    /**
     * @brief 连接到Modbus设备
     * @param config 串口配置参数
     * @return 连接是否成功
     */
    bool connect(const SerialConfig &config);
    
    /**
     * @brief 断开与Modbus设备的连接
     */
    void disconnect();
    
    /**
     * @brief 检查是否已连接到设备
     * @return 连接状态
     */
    bool isConnected() const;

    // 同步操作 (阻塞调用)
    /**
     * @brief 读取保持寄存器（功能码0x03）
     * @param slaveId 从站地址（1-247）
     * @param startAddr 起始寄存器地址
     * @param quantity 要读取的寄存器数量（1-125）
     * @param exception 异常信息输出参数（可选）
     * @return 读取的寄存器值数组
     */
    QVector<quint16> readHoldingRegisters(int slaveId, int startAddr, int quantity,
                                          ModbusException *exception = nullptr);
    
    /**
     * @brief 读取输入寄存器（功能码0x04）
     * @param slaveId 从站地址（1-247）
     * @param startAddr 起始寄存器地址
     * @param quantity 要读取的寄存器数量（1-125）
     * @param exception 异常信息输出参数（可选）
     * @return 读取的寄存器值数组
     */
    QVector<quint16> readInputRegisters(int slaveId, int startAddr, int quantity,
                                        ModbusException *exception = nullptr);
    
    /**
     * @brief 读取线圈（功能码0x01）
     * @param slaveId 从站地址（1-247）
     * @param startAddr 起始线圈地址
     * @param quantity 要读取的线圈数量（1-2000）
     * @param exception 异常信息输出参数（可选）
     * @return 读取的线圈状态数组
     */
    QVector<bool> readCoils(int slaveId, int startAddr, int quantity,
                           ModbusException *exception = nullptr);
    
    /**
     * @brief 读取离散输入（功能码0x02）
     * @param slaveId 从站地址（1-247）
     * @param startAddr 起始离散输入地址
     * @param quantity 要读取的离散输入数量（1-2000）
     * @param exception 异常信息输出参数（可选）
     * @return 读取的离散输入状态数组
     */
    QVector<bool> readDiscreteInputs(int slaveId, int startAddr, int quantity,
                                     ModbusException *exception = nullptr);

    /**
     * @brief 写入单个寄存器（功能码0x06）
     * @param slaveId 从站地址（1-247）
     * @param addr 寄存器地址
     * @param value 要写入的值
     * @param exception 异常信息输出参数（可选）
     * @return 写入是否成功
     */
    bool writeSingleRegister(int slaveId, int addr, quint16 value,
                             ModbusException *exception = nullptr);
    
    /**
     * @brief 写入多个寄存器（功能码0x10）
     * @param slaveId 从站地址（1-247）
     * @param startAddr 起始寄存器地址
     * @param values 要写入的值数组
     * @param exception 异常信息输出参数（可选）
     * @return 写入是否成功
     */
    bool writeMultipleRegisters(int slaveId, int startAddr, const QVector<quint16> &values,
                                ModbusException *exception = nullptr);
    
    /**
     * @brief 写入单个线圈（功能码0x05）
     * @param slaveId 从站地址（1-247）
     * @param addr 线圈地址
     * @param value 要写入的状态（true=ON, false=OFF）
     * @param exception 异常信息输出参数（可选）
     * @return 写入是否成功
     */
    bool writeSingleCoil(int slaveId, int addr, bool value,
                         ModbusException *exception = nullptr);
    
    /**
     * @brief 写入多个线圈（功能码0x0F）
     * @param slaveId 从站地址（1-247）
     * @param startAddr 起始线圈地址
     * @param values 要写入的状态数组
     * @param exception 异常信息输出参数（可选）
     * @return 写入是否成功
     */
    bool writeMultipleCoils(int slaveId, int startAddr, const QVector<bool> &values,
                            ModbusException *exception = nullptr);

    // 异步操作 (非阻塞调用)
    /**
     * @brief 异步读取保持寄存器（功能码0x03）
     * @param slaveId 从站地址（1-247）
     * @param startAddr 起始寄存器地址
     * @param quantity 要读取的寄存器数量（1-125）
     * @return 异步操作结果Future对象
     */
    QFuture<AsyncResult> readHoldingRegistersAsync(int slaveId, int startAddr, int quantity);
    
    /**
     * @brief 异步读取输入寄存器（功能码0x04）
     * @param slaveId 从站地址（1-247）
     * @param startAddr 起始寄存器地址
     * @param quantity 要读取的寄存器数量（1-125）
     * @return 异步操作结果Future对象
     */
    QFuture<AsyncResult> readInputRegistersAsync(int slaveId, int startAddr, int quantity);
    
    /**
     * @brief 异步读取线圈（功能码0x01）
     * @param slaveId 从站地址（1-247）
     * @param startAddr 起始线圈地址
     * @param quantity 要读取的线圈数量（1-2000）
     * @return 异步操作结果Future对象
     */
    QFuture<AsyncResult> readCoilsAsync(int slaveId, int startAddr, int quantity);
    
    /**
     * @brief 异步读取离散输入（功能码0x02）
     * @param slaveId 从站地址（1-247）
     * @param startAddr 起始离散输入地址
     * @param quantity 要读取的离散输入数量（1-2000）
     * @return 异步操作结果Future对象
     */
    QFuture<AsyncResult> readDiscreteInputsAsync(int slaveId, int startAddr, int quantity);

    /**
     * @brief 异步写入单个寄存器（功能码0x06）
     * @param slaveId 从站地址（1-247）
     * @param addr 寄存器地址
     * @param value 要写入的值
     * @return 异步操作结果Future对象
     */
    QFuture<AsyncResult> writeSingleRegisterAsync(int slaveId, int addr, quint16 value);
    
    /**
     * @brief 异步写入多个寄存器（功能码0x10）
     * @param slaveId 从站地址（1-247）
     * @param startAddr 起始寄存器地址
     * @param values 要写入的值数组
     * @return 异步操作结果Future对象
     */
    QFuture<AsyncResult> writeMultipleRegistersAsync(int slaveId, int startAddr,
                                                     const QVector<quint16> &values);
    
    /**
     * @brief 异步写入单个线圈（功能码0x05）
     * @param slaveId 从站地址（1-247）
     * @param addr 线圈地址
     * @param value 要写入的状态（true=ON, false=OFF）
     * @return 异步操作结果Future对象
     */
    QFuture<AsyncResult> writeSingleCoilAsync(int slaveId, int addr, bool value);
    
    /**
     * @brief 异步写入多个线圈（功能码0x0F）
     * @param slaveId 从站地址（1-247）
     * @param startAddr 起始线圈地址
     * @param values 要写入的状态数组
     * @return 异步操作结果Future对象
     */
    QFuture<AsyncResult> writeMultipleCoilsAsync(int slaveId, int startAddr,
                                                 const QVector<bool> &values);

    // 配置
    /**
     * @brief 设置响应超时时间
     * @param milliseconds 超时时间（毫秒）
     */
    void setResponseTimeout(int milliseconds);
    
    /**
     * @brief 设置帧间延迟时间
     * @param milliseconds 延迟时间（毫秒）
     */
    void setInterFrameDelay(int milliseconds);
    
    /**
     * @brief 设置重试次数
     * @param count 重试次数
     */
    void setRetryCount(int count);

private slots:
    /**
     * @brief 串口数据接收槽函数
     * 当串口有数据可读时被调用，处理接收到的Modbus数据帧
     */
    void onReadyRead();

signals:
    /**
     * @brief 连接成功信号
     * 当成功连接到Modbus设备时发出
     */
    void connected();
    
    /**
     * @brief 断开连接信号
     * 当与Modbus设备断开连接时发出
     */
    void disconnected();
    
    /**
     * @brief 错误发生信号
     * @param error 错误描述信息
     */
    void errorOccurred(const QString &error);
    
    /**
     * @brief 响应接收信号
     * @param slaveId 从站地址
     * @param functionCode 功能码
     */
    void responseReceived(int slaveId, ModbusFunctionCode functionCode);
    
    /**
     * @brief 请求发送信号
     * @param slaveId 从站地址
     * @param functionCode 功能码
     */
    void requestSent(int slaveId, ModbusFunctionCode functionCode);

private:
    // 串口和定时器
    QSerialPort *m_serialPort;        ///< 串口对象指针
    QTimer *m_responseTimer;          ///< 响应超时定时器
    QTimer *m_interFrameTimer;        ///< 帧间延迟定时器

    // 配置
    int m_responseTimeout;            ///< 响应超时时间（毫秒）
    int m_interFrameDelay;            ///< 帧间延迟时间（毫秒）
    int m_retryCount;                 ///< 重试次数

    // 状态和锁
    bool m_isConnected;               ///< 连接状态标志
    QMutex m_serialMutex;             ///< 串口操作互斥锁
    QByteArray m_receiveBuffer;       ///< 接收数据缓冲区

    // CRC计算
    /**
     * @brief 计算CRC校验码
     * @param data 要计算CRC的数据
     * @return CRC校验码
     */
    quint16 calculateCRC(const QByteArray &data);
    
    /**
     * @brief 验证CRC校验码
     * @param data 包含CRC的数据帧
     * @return CRC验证是否通过
     */
    bool verifyCRC(const QByteArray &data);

    // 底层通信
    /**
     * @brief 发送Modbus请求
     * @param slaveId 从站地址
     * @param functionCode 功能码
     * @param data 请求数据
     * @param exception 异常信息输出参数（可选）
     * @return 响应数据
     */
    QByteArray sendRequest(int slaveId, ModbusFunctionCode functionCode,
                          const QByteArray &data, ModbusException *exception = nullptr);
    
    /**
     * @brief 构建Modbus请求帧
     * @param slaveId 从站地址
     * @param functionCode 功能码
     * @param data 请求数据
     * @return 完整的Modbus请求帧（包含CRC）
     */
    QByteArray buildRequest(int slaveId, ModbusFunctionCode functionCode,
                           const QByteArray &data);
    
    /**
     * @brief 读取Modbus响应
     * @param slaveId 从站地址
     * @param functionCode 功能码
     * @param exception 异常信息输出参数（可选）
     * @return 响应数据
     */
    QByteArray readResponse(int slaveId, ModbusFunctionCode functionCode,
                           ModbusException *exception = nullptr);

    // 数据转换
    /**
     * @brief 线圈状态数组转换为字节数组
     * @param coils 线圈状态数组
     * @return 转换后的字节数组
     */
    QByteArray coilsToBytes(const QVector<bool> &coils);
    
    /**
     * @brief 字节数组转换为线圈状态数组
     * @param bytes 字节数组
     * @param count 线圈数量
     * @return 转换后的线圈状态数组
     */
    QVector<bool> bytesToCoils(const QByteArray &bytes, int count);
    
    /**
     * @brief 寄存器数组转换为字节数组
     * @param registers 寄存器值数组
     * @return 转换后的字节数组
     */
    QByteArray registersToBytes(const QVector<quint16> &registers);
    
    /**
     * @brief 字节数组转换为寄存器数组
     * @param bytes 字节数组
     * @return 转换后的寄存器值数组
     */
    QVector<quint16> bytesToRegisters(const QByteArray &bytes);

    // 异步操作执行器
    /**
     * @brief 执行异步读取操作
     * @param slaveId 从站地址
     * @param functionCode 功能码
     * @param startAddr 起始地址
     * @param quantity 数量
     * @return 异步操作结果
     */
    AsyncResult executeAsyncRead(int slaveId, ModbusFunctionCode functionCode,
                                int startAddr, int quantity);
    
    /**
     * @brief 执行异步写入操作
     * @param slaveId 从站地址
     * @param functionCode 功能码
     * @param startAddr 起始地址
     * @param data 写入数据
     * @return 异步操作结果
     */
    AsyncResult executeAsyncWrite(int slaveId, ModbusFunctionCode functionCode,
                                 int startAddr, const QByteArray &data);
};

#endif // MODBUSRTUCLIENT_H
