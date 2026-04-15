#ifndef CONNECTION_MANAGER_H
#define CONNECTION_MANAGER_H

#include "modbus_types.h"
#include <QObject>
#include <QSerialPort>
#include <QTimer>
#include <QElapsedTimer>
#include <QMutex>
#include "qmodbusrtuunit_global.h"
/**
 * @file connection_manager.h
 * @brief 串口连接管理类
 * 
 * ConnectionManager类，负责管理Modbus RTU的串口通信：
 * - 串口连接管理：打开/关闭串口，参数配置
 * - 数据收发：异步数据发送和接收
 * - 错误处理：连接错误、通信错误的检测和处理
 * - 重连机制：支持重连
 * - 通信统计：记录发送/接收字节数、错误次数等
 *
 * 优点：
 * - 线程安全：使用QMutex保护串口操作
 * - 异步通信：基于Qt信号槽的非阻塞通信
 * - 智能重连：支持多种重连策略，提高系统可靠性
 *
 */
class QMODBUSRTUUNIT_EXPORT ConnectionManager : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief 重连策略枚举
     * 定义不同的重连策略，用于控制连接断开后的重连行为
     */
    enum ReconnectPolicy {
        NoReconnect,           ///< 不重连
        ImmediateReconnect,    ///< 立即重连
        IncrementalReconnect,  ///< 递增重连（重连间隔逐渐增加）
        ExponentialBackoff     ///< 指数退避重连（重连间隔指数增长）
    };

    /**
     * @brief 连接配置结构体
     * 包含串口连接的所有配置参数
     */
    struct ConnectionConfig {
        SerialPortConfig serialConfig;      ///< 串口配置参数
        ReconnectPolicy reconnectPolicy;    ///< 重连策略
        int reconnectInterval;              ///< 重连间隔（毫秒）
        int maxReconnectAttempts;           ///< 最大重连尝试次数
        bool autoDetectPort;                ///< 是否自动检测串口
        bool keepAliveEnabled;              ///< 是否启用心跳包
        int keepAliveInterval;              ///< 心跳包间隔（毫秒）
        
        /**
         * @brief 默认构造函数
         * 使用默认配置：递增重连策略，5秒重连间隔，最大10次重连尝试
         * 启用心跳包，30秒心跳间隔
         */
        ConnectionConfig() {
            reconnectPolicy = IncrementalReconnect;
            reconnectInterval = 5000;
            maxReconnectAttempts = 100; //最大重连次数修改为3次，超过3次之后默认断开链接，同时应用层需要断开任务调度器
            autoDetectPort = false;
            keepAliveEnabled = false; //先默认不开启心跳包
            keepAliveInterval = 30000;
        }
    };

    /**
     * @brief 构造函数
     * @param parent 父对象指针
     */
    explicit ConnectionManager(QObject* parent = nullptr);
    
    /**
     * @brief 析构函数
     * 自动断开连接并清理资源
     */
    ~ConnectionManager();
    
    // 连接管理
    /**
     * @brief 连接到Modbus设备
     * @param config 连接配置参数
     * @return 连接是否成功
     */
    bool connect(const ConnectionConfig& config);
    
    /**
     * @brief 断开与Modbus设备的连接
     */
    void disconnect();
    
    /**
     * @brief 尝试重新连接
     * @return 重连是否成功
     */
    bool reconnect();
    
    /**
     * @brief 检查是否已连接到设备
     * @return 连接状态
     */
    bool isConnected() const;
    
    // 串口操作
    /**
     * @brief 发送数据到串口
     * @param data 要发送的数据字节数组
     * @return 发送是否成功
     * 
     * 线程安全的数据发送方法，使用互斥锁保护串口操作
     * 发送前会检查连接状态，如果未连接则返回false
     * 发送成功后会发射dataSent信号
     */
    bool sendData(const QByteArray& data);
    
    /**
     * @brief 从串口读取数据
     * @param timeout 读取超时时间（毫秒），默认100ms
     * @return 读取到的数据字节数组
     * 
     * 阻塞式读取方法，等待数据到达或超时
     * 如果超时时间内没有数据到达，返回空字节数组
     * 读取到数据后会发射dataReceived信号
     */
    QByteArray readData(int timeout = 100);
    
    /**
     * @brief 刷新串口缓冲区
     * 
     * 清空串口的发送和接收缓冲区
     * 用于在连接异常或数据错乱时恢复通信
     */
    void flush();
    
    /**
     * @brief 清空内部缓冲区
     * 
     * 清空内部的数据缓冲区，但不影响串口硬件缓冲区
     * 用于重新开始数据采集或处理
     */
    void clearBuffers();
    
    // 配置管理
    /**
     * @brief 设置连接配置
     * @param config 新的连接配置参数
     * 
     * 动态更新连接配置，如果当前已连接，会先断开连接再应用新配置
     * 配置更改后需要重新连接才能生效
     */
    void setConfig(const ConnectionConfig& config);
    
    /**
     * @brief 获取当前连接配置
     * @return 当前连接配置的常量引用
     */
    const ConnectionConfig& config() const { return m_config; }
    
    // 状态查询
    /**
     * @brief 获取当前连接状态
     * @return 当前连接状态枚举值
     */
    ConnectionState state() const { return m_state; }
    
    /**
     * @brief 获取重连尝试次数
     * @return 当前重连尝试次数
     */
    int reconnectAttempts() const { return m_reconnectAttempts; }
    
    /**
     * @brief 获取连接持续时间
     * @return 当前连接持续时间（毫秒）
     */
    qint64 connectionTime() const { return m_connectionTime; }
    
    /**
     * @brief 获取错误描述字符串
     * @return 最近一次错误的描述信息
     */
    QString errorString() const { return m_errorString; }
    
    // 自动检测
    /**
     * @brief 获取可用的串口列表
     * @return 系统中可用的串口名称列表
     * 
     * 扫描系统上所有可用的串口设备
     * 返回格式如：["COM1", "COM3", "COM5"]
     */
    QStringList availablePorts() const;
    
    /**
     * @brief 检测串口的最优设置
     * @param portName 要检测的串口名称
     * @param detectedConfig 检测到的配置参数输出
     * @return 检测是否成功
     * 
     * 自动检测串口的波特率、数据位、校验位和停止位
     * 通过发送测试数据并分析响应来识别最佳配置
     */
    bool detectOptimalSettings(const QString& portName, SerialPortConfig& detectedConfig);
    
    // 健康检查
    /**
     * @brief 执行连接健康检查
     * @return 健康检查是否通过
     * 
     * 发送测试数据包并验证响应，检查连接质量
     * 如果启用了心跳包，会使用心跳包进行健康检查
     */
    bool performHealthCheck();
    
    /**
     * @brief 计算信号质量
     * @return 信号质量评分（0.0-1.0）
     * 
     * 基于错误率计算虚拟信号质量：
     * - 1.0：无错误，信号质量优秀
     * - 0.5：中等错误率，信号质量一般
     * - 0.0：高错误率，信号质量差
     */
    float signalQuality() const;
    
signals:
    /**
     * @brief 连接成功信号
     * 当成功连接到Modbus设备时发射
     */
    void connected();
    
    /**
     * @brief 连接断开信号
     * 当与Modbus设备的连接断开时发射
     */
    void disconnected();
    
    /**
     * @brief 连接错误信号
     * @param error 错误描述信息
     * 当连接过程中发生错误时发射
     */
    void connectionError(const QString& error);
    
    /**
     * @brief 状态改变信号
     * @param state 新的连接状态
     * 当连接状态发生变化时发射
     */
    void stateChanged(ConnectionState state);
    
    /**
     * @brief 数据发送信号
     * @param data 发送的数据
     * 当数据成功发送到串口时发射
     */
    void dataSent(const QByteArray& data);
    
    /**
     * @brief 数据接收信号
     * @param data 接收到的数据
     * 当从串口接收到数据时发射
     */
    void dataReceived(const QByteArray& data);
    
    /**
     * @brief 重连尝试信号
     * @param attempt 当前重连尝试次数
     * @param maxAttempts 最大重连尝试次数
     * 当开始重连尝试时发射
     */
    void reconnectAttempt(int attempt, int maxAttempts);
    
    /**
     * @brief 心跳包接收信号
     * 当接收到心跳包响应时发射
     */
    void keepAliveReceived();

private slots:
    /**
     * @brief 串口数据接收槽函数
     * 当串口有数据可读时被调用，处理接收到的数据
     */
    void onReadyRead();
    
    /**
     * @brief 串口错误处理槽函数
     * @param error 串口错误类型
     * 当串口发生错误时被调用，处理错误并尝试恢复
     */
    void onError(QSerialPort::SerialPortError error);
    
    /**
     * @brief 重连超时槽函数
     * 当重连定时器超时时被调用，执行重连操作
     */
    void onReconnectTimeout();
    
    /**
     * @brief 心跳包超时槽函数
     * 当心跳包定时器超时时被调用，发送心跳包并检查响应
     */
    void onKeepAliveTimeout();
    
    /**
     * @brief 心跳包响应超时槽函数
     * 当心跳包响应超时时被调用，处理心跳包无响应的情况
     */
    void onHeartbeatResponseTimeout();
    
private:
    // CRC计算
    /**
     * @brief 计算CRC校验码
     * @param data 要计算CRC的数据
     * @return CRC校验码
     * 
     * 使用Modbus RTU标准的CRC-16算法（多项式0xA001）
     * 也有crc32，crc8，这个具体使用哪个，实际开发的时候可以和需求询问下
     * 算法步骤：
     * 1. 初始化CRC为0xFFFF
     * 2. 对每个字节进行异或操作
     * 3. 对每个位进行移位和异或操作
     */
    quint16 calculateCRC(const QByteArray& data);
    
    /**
     * @brief 验证CRC校验码
     * @param data 包含CRC的数据帧
     * @return CRC验证是否通过
     * 
     * 验证接收到的Modbus数据帧的CRC校验码是否正确
     * 数据帧的最后2个字节应为CRC校验码
     */
    bool verifyCRC(const QByteArray& data);
    
    // 重连策略
    /**
     * @brief 计算重连延迟时间
     * @return 重连延迟时间（毫秒）
     * 
     * 根据当前重连策略和尝试次数计算重连延迟：
     * - ImmediateReconnect：固定延迟
     * - IncrementalReconnect：延迟时间递增
     * - ExponentialBackoff：延迟时间指数增长
     */
    int calculateReconnectDelay();
    
    // 数据验证
    /**
     * @brief 验证Modbus数据帧
     * @param data 要验证的数据帧
     * @return 数据帧是否有效
     * 
     * 验证Modbus RTU数据帧的完整性和格式：
     * 1. 检查数据帧长度
     * 2. 验证CRC校验码
     * 3. 检查从站地址和功能码
     */
    bool validateModbusFrame(const QByteArray& data);
    
    /**
     * @brief 检测是否为心跳包响应
     * @param data 接收到的数据帧
     * @return 是否为心跳包响应
     * 
     * 检测接收到的数据是否为心跳包查询的响应
     * 心跳包查询：从站1，功能码0x03，读取保持寄存器0，数量1
     * 响应格式：从站地址，功能码，字节数，寄存器值，CRC
     */
    bool isHeartbeatResponse(const QByteArray& data);
    
    // 成员变量
    QSerialPort* m_serialPort;        ///< 串口对象指针，负责底层串口通信
    ConnectionConfig m_config;        ///< 当前连接配置参数
    ConnectionState m_state;          ///< 当前连接状态
    
    // 定时器
    QTimer* m_reconnectTimer;         ///< 重连定时器，控制重连间隔
    QTimer* m_keepAliveTimer;         ///< 心跳包定时器，定期发送心跳包
    QTimer* m_heartbeatResponseTimer; ///< 心跳包响应超时定时器
    QElapsedTimer m_connectionTimer;  ///< 连接持续时间计时器
    
    // 状态跟踪
    int m_reconnectAttempts;          ///< 当前重连尝试次数
    int m_heartbeatTimeoutCount;      ///< 心跳包超时次数
    int m_maxHeartbeatTimeouts;       ///< 最大心跳包超时次数（默认3次）
    bool m_heartbeatAcknowledged;     ///< 心跳包是否已确认
    QString m_errorString;            ///< 最近一次错误的描述信息
    qint64 m_connectionTime;          ///< 连接建立的时间戳（毫秒）
    
    // 统计数据
    qint64 m_bytesSent;               ///< 已发送字节总数
    qint64 m_bytesReceived;           ///< 已接收字节总数
    qint64 m_sendErrors;              ///< 发送错误次数
    qint64 m_receiveErrors;           ///< 接收错误次数
    
    // 缓冲区
    QByteArray m_receiveBuffer;       ///< 接收数据缓冲区
    QByteArray m_sendBuffer;          ///< 发送数据缓冲区
    
    // 同步
    QMutex m_serialMutex;             ///< 串口操作互斥锁，保证线程安全
};
#endif
