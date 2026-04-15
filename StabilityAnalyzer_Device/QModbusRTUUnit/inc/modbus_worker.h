#ifndef MODBUS_WORKER_H
#define MODBUS_WORKER_H

#include "modbus_types.h"
#include "modbus_request.h"
#include "modbus_response.h"
#include "connection_manager.h"
#include <QObject>
#include <QThread>
#include <QQueue>
#include <QMutex>
#include <QTimer>
#include <QMap>
#include "qmodbusrtuunit_global.h"

/**
 * @file modbus_worker.h
 * @brief Modbus工作线程类，在子线程
 * 
 * ModbusWorker类，负责处理所有Modbus RTU通信的核心业务逻辑。
 * 主要功能：
 * - 请求队列管理：串行处理Modbus请求，避免并发冲突
 * - 协议处理：构建Modbus RTU帧，解析响应数据
 * - 错误处理：超时、CRC校验失败、异常响应等
 * - 通信统计：记录发送/接收字节数、错误次数等
 * 
 * 优点：
 * - 线程安全：使用QMutex保护共享资源
 * - 帧边界检测：智能检测完整Modbus RTU帧，避免多数据重复和数据不完整的问题
 * - 状态管理：通过m_isProcessing控制请求处理流程 
 */

/**
 * @brief Modbus工作线程，处理所有Modbus通信
 */
class QMODBUSRTUUNIT_EXPORT ModbusWorker : public QObject
{
    Q_OBJECT

public:
    explicit ModbusWorker(QObject* parent = nullptr);
    ~ModbusWorker();
    
    /**
     * @brief 初始化工作线程
     */
    bool initialize(const SerialPortConfig& serialConfig);
    
    /**
     * @brief 关闭工作线程
     */
    Q_INVOKABLE void shutdown();
    
    /**
     * @brief 添加请求到队列
     */
    
    /**
     * @brief 取消指定请求
     */
    void cancelRequest(qint64 requestId);
    
    /**
     * @brief 清空请求队列
     */
    void clearQueue();
    
    /**
     * @brief 检查是否已连接
     */
    bool isConnected() const;
    
    /**
     * @brief 计算Modbus CRC校验码
     */
    quint16 calculateCRC(const QByteArray& data);

signals:
    /**
     * @brief 请求完成信号
     */
    void requestCompleted(qint64 requestId, const ModbusResult& result);
    
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
    //连接信号
    void connected();

public slots:
    void enqueueRequest(ModbusRequestPtr request);
    /**
     * @brief 连接设备并返回结果（辅助槽函数）
     */
    void doConnectDevice(bool* result);

    /**
     * @brief 连接设备
     */
    void connectDevice();
    
    /**
     * @brief 断开设备连接
     */
    void disconnectDevice();
    
    /**
     * @brief 处理请求队列
     */
    void processQueue();
    
    /**
     * @brief 初始化工作线程（辅助槽函数）
     */
    void doInitialize(const SerialPortConfig& serialConfig, bool* result);
    
    /**
     * @brief 检查连接状态（辅助槽函数）
     */
    void doGetIsConnected(bool* result);

private slots:
    /**
     * @brief 处理串口数据接收
     */
    void onDataReceived(const QByteArray& data);
    
    /**
     * @brief 处理连接错误
     */
    void onConnectionError(const QString& error);

    //断开连接
    void onDisconnected();
    //连接成功槽函数
    void onConnected();

private:
    /**
     * @brief 发送Modbus请求
     */
    bool sendRequest(QSharedPointer<ModbusRequest> request);
    
    /**
     * @brief 处理Modbus响应
     */
    void processResponse(const QByteArray& data);
    
    /**
     * @brief 验证Modbus CRC
     */
    bool validateModbusCRC(const QByteArray& data);
    
    /**
     * @brief 创建响应对象
     */
    QSharedPointer<ModbusResponse> createResponse(ModbusFunctionCode functionCode);
    
    /**
     * @brief 处理异常响应
     */
    void handleExceptionResponse(quint8 slaveId, quint8 functionCode, quint8 exceptionCode);
    
    /**
     * @brief 检测并处理完整的Modbus RTU帧
     */
    bool detectAndProcessCompleteFrame();
    
    /**
     * @brief 根据功能码获取预期的帧长度
     */
    int getExpectedFrameLength(quint8 functionCode, const QByteArray& buffer, int startPos);
    
    /**
     * @brief 更新通信统计
     */
    void updateCommStats(bool success);

private:
    ConnectionManager* m_connectionManager;
    // 注：ModbusWorker 自身不管理线程，由 ModbusClient 将其 moveToThread，此处不保留 m_workerThread
    // QThread* m_workerThread;
    
    // 请求队列
    QQueue<QSharedPointer<ModbusRequest>> m_requestQueue;
    mutable QMutex m_queueMutex;
    
    // 当前处理的请求
    QSharedPointer<ModbusRequest> m_currentRequest;
    QByteArray m_responseBuffer;
    
    // 定时器
    QTimer* m_processTimer;
    QTimer* m_responseTimer;
    
    // 通信统计
    struct CommStats {
        quint64 totalRequests = 0;
        quint64 successfulRequests = 0;
        quint64 failedRequests = 0;
        quint64 timeoutErrors = 0;
        quint64 crcErrors = 0;
        quint64 frameErrors = 0;
    };
    
    CommStats m_commStats;
    
    // 配置
    SerialPortConfig m_serialConfig; // 串口配置
    int m_requestTimeout = 3000; // 请求超时时间（毫秒）
    int m_retryCount = 3;        // 重试次数
    
    bool m_isConnected = false;
    bool m_isProcessing = false; //是否正在处理一个请求，确保同一时间只处理一个modbus请求
};

#endif // MODBUS_WORKER_H
