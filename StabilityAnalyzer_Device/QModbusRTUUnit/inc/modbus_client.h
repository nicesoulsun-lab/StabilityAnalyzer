#ifndef MODBUS_CLIENT_H
#define MODBUS_CLIENT_H

#include "modbus_types.h"
#include "modbus_request.h"
#include "modbus_response.h"
#include "modbus_worker.h"
#include "qmodbusrtuunit_global.h"
#include <QObject>
#include <QThread>
#include <QMap>
#include <QEventLoop>
#include <QTimer>
#include <atomic>

/**
 * @file modbus_client.h
 * @brief Modbus RTU客户端主接口类
 * 
 * ModbusClient类，作为整个Modbus RTU通信系统的入口点。
 * 采用三层架构设计：
 * - 接口层：ModbusClient - 外部调用的API接口
 * - 业务逻辑层：ModbusWorker - 在子线程里面，主要负责处理Modbus协议和请求队列
 * - 通信层：ConnectionManager - 子线程管理串口通信
 * 
 * 支持同步和异步两种操作模式：
 * - 同步模式：阻塞调用，等待响应返回
 * - 异步模式：非阻塞调用，通过信号通知结果
 * 
 */

class QMODBUSRTUUNIT_EXPORT ModbusClient : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(ModbusClient)

public:
    /**
     * @brief 客户端配置结构体
     * 
     * 包含Modbus客户端运行所需的所有配置参数
     */
    struct ClientConfig {
        SerialPortConfig serialConfig;  ///< 串口通信配置（端口、波特率等）
        int maxQueueSize = 100;         ///< 最大请求队列大小，防止内存溢出
        int requestTimeout = 3000;      ///< 请求超时时间（毫秒）
        int retryCount = 3;             ///< 失败重试次数
        
        ClientConfig() = default;       ///< 默认构造函数
    };

    explicit ModbusClient(QObject* parent = nullptr);
    ~ModbusClient();
    
    /**
     * @brief 初始化客户端
     */
    bool initialize(const ClientConfig& config);
    
    /**
     * @brief 关闭客户端
     */
    void shutdown();
    
    /**
     * @brief 连接设备
     */
    bool connect();
    
    /**
     * @brief 断开设备连接
     */
    void disconnect();
    
    /**
     * @brief 检查是否已连接
     */
    bool isConnected() const;
    
    // 同步操作接口
    /**
     * @brief 读取保持寄存器（同步）
     */
    QVector<quint16> readHoldingRegisters(int slaveId, int startAddr, int quantity);
    
    /**
     * @brief 读取输入寄存器（同步）
     */
    QVector<quint16> readInputRegisters(int slaveId, int startAddr, int quantity);
    
    /**
     * @brief 读取线圈状态（同步）
     */
    QVector<quint16> readCoils(int slaveId, int startAddr, int quantity);
    
    /**
     * @brief 读取离散输入（同步）
     */
    QVector<quint16> readDiscreteInputs(int slaveId, int startAddr, int quantity);
    
    /**
     * @brief 写入单个线圈（同步）
     */
    bool writeSingleCoil(int slaveId, int address, const QVector<quint16>& values);
    
    /**
     * @brief 写入单个寄存器（同步）
     */
    bool writeSingleRegister(int slaveId, int address, const QVector<quint16>& values);
    
    /**
     * @brief 写入多个线圈（同步）
     */
    bool writeMultipleCoils(int slaveId, int startAddr, const QVector<quint16>& values);
    
    /**
     * @brief 写入多个寄存器（同步）
     */
    bool writeMultipleRegisters(int slaveId, int startAddr, const QVector<quint16>& values);
    
    // 异步操作接口
    
    /**
     * @brief 异步读取保持寄存器
     */
    void readHoldingRegistersAsync(int slaveId, int startAddr, int quantity, const QString& tag = QString());
    
    /**
     * @brief 异步读取输入寄存器
     */
    void readInputRegistersAsync(int slaveId, int startAddr, int quantity, const QString& tag = QString());
    
    /**
     * @brief 异步读取线圈状态
     */
    void readCoilsAsync(int slaveId, int startAddr, int quantity, const QString& tag = QString());
    
    /**
     * @brief 异步读取离散输入
     */
    void readDiscreteInputsAsync(int slaveId, int startAddr, int quantity, const QString& tag = QString());
    
    /**
     * @brief 异步写入单个线圈
     */
    void writeSingleCoilAsync(int slaveId, int address, const QVector<quint16>& values, const QString& tag = QString());
    
    /**
     * @brief 异步写入单个寄存器
     */
    void writeSingleRegisterAsync(int slaveId, int address, const QVector<quint16>& values, const QString& tag = QString());
    
    /**
     * @brief 异步写入多个线圈
     */
    void writeMultipleCoilsAsync(int slaveId, int startAddr, const QVector<quint16>& values, const QString& tag = QString());
    
    /**
     * @brief 异步写入多个寄存器
     */
    void writeMultipleRegistersAsync(int slaveId, int startAddr, const QVector<quint16>& values, const QString& tag = QString());
    
    /**
     * @brief 取消指定请求
     */
    void cancelRequest(qint64 requestId);
    
    /**
     * @brief 清空请求队列
     */
    void clearQueue();

signals:
    /**
     * @brief 异步请求完成信号
     */
    void requestCompleted(const QString& tag, const ModbusResult& result);
    
    /**
     * @brief 连接状态变化信号
     */
    void connectionStatusChanged(bool connected);
    
    /**
     * @brief 通信错误信号
     */
    void communicationError(const QString& error);

    //断开连接信号
    void disconnected();
    void connected();
private slots:
    /**
     * @brief 处理工作线程的请求完成信号
     */
    void onWorkerRequestCompleted(qint64 requestId, const ModbusResult& result);
    
    /**
     * @brief 处理工作线程的连接状态变化
     */
    void onWorkerConnectionStatusChanged(bool connected);
    
    /**
     * @brief 处理工作线程的通信错误
     */
    void onWorkerCommunicationError(const QString& error);

    //断开连接
    void onDisconnected();
    void onConnected();
private:
    /**
     * @brief 生成唯一的请求ID
     */
    qint64 generateRequestId();
    
    /**
     * @brief 创建请求对象
     */
    QSharedPointer<ModbusRequest> createRequest(ModbusFunctionCode functionCode, int slaveId, 
                                               int startAddr, int quantity, const QByteArray& data = QByteArray());
    
    /**
     * @brief 执行同步操作
     */
    ModbusResult executeSyncOperation(QSharedPointer<ModbusRequest> request);
    
    /**
     * @brief 执行异步操作
     */
    void executeAsyncOperation(QSharedPointer<ModbusRequest> request, const QString& tag);

private:
    ModbusWorker* m_worker;
    QThread* m_workerThread;
    
    // 请求映射表（用于异步操作）
    QMap<qint64, QString> m_requestTagMap;
    
    // 同步操作相关
    QMap<qint64, QEventLoop*> m_syncEventLoops;
    QMap<qint64, ModbusResult> m_syncResults;
    mutable QMutex m_syncMutex;
    
    // 请求ID计数器
    qint64 m_requestIdCounter = 0;
    mutable QMutex m_idMutex;
    
    bool m_isInitialized = false;

    // 连接状态缓存（原子变量，避免跨线程 BlockingQueuedConnection 查询造成的性能损耗和潜在死锁）
    mutable std::atomic<bool> m_cachedConnected{false};
};

#endif // MODBUS_CLIENT_H
