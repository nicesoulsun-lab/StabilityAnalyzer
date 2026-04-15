#ifndef MODBUS_REQUEST_H
#define MODBUS_REQUEST_H

#include "modbus_types.h"
#include <QObject>
#include <QTimer>
#include <QElapsedTimer>
#include <QSharedPointer>
#include <atomic>
#include "qmodbusrtuunit_global.h"
/**
 * @file modbus_request.h
 * @brief Modbus请求类定义
 * 
 * Modbus请求相关的类，采用工厂模式和建造者模式设计：
 * - ModbusRequest: 抽象基类，定义所有请求的通用接口
 * - Builder: 建造者类，提供API构建请求对象
 * - 具体请求类：ReadHoldingRegistersRequest, WriteSingleRegisterRequest等
 * 
 * 优点：
 * - 类型安全：每种功能码对应特定的请求类
 * - 可扩展：易于添加新的Modbus功能码支持
 * - 线程安全：使用QSharedPointer进行内存管理
 */

class QMODBUSRTUUNIT_EXPORT ModbusRequest : public QObject
{
    Q_OBJECT

public:
    // 请求构造器模式
    class Builder {
    public:
        Builder(int slaveId, ModbusFunctionCode functionCode)
            : m_slaveId(slaveId)
            , m_functionCode(functionCode)
            , m_priority(RequestPriority::Normal)
            , m_timeout(1000)
            , m_maxRetries(3)
            , m_useCache(false)
            , m_cacheValidity(5000)
        {}
        
        Builder& setStartAddress(int addr) { m_startAddr = addr; return *this; }
        Builder& setQuantity(int qty) { m_quantity = qty; return *this; }
        Builder& setData(const QVector<quint16>& data) { m_data = data; return *this; }
        Builder& setPriority(RequestPriority prio) { m_priority = prio; return *this; }
        Builder& setTimeout(int ms) { m_timeout = ms; return *this; }
        Builder& setMaxRetries(int retries) { m_maxRetries = retries; return *this; }
        Builder& useCache(bool use, int validityMs = 5000) { 
            m_useCache = use; 
            m_cacheValidity = validityMs; 
            return *this; 
        }
        Builder& setTag(const QString& tag) { m_tag = tag; return *this; }
        
        QSharedPointer<ModbusRequest> build();
        
        void setUseCache(bool newUseCache);

    private:
        int m_slaveId;
        ModbusFunctionCode m_functionCode;
        int m_startAddr;
        int m_quantity;
        QVector<quint16> m_data;
        RequestPriority m_priority;
        int m_timeout;
        int m_maxRetries;
        bool m_useCache;
        int m_cacheValidity;
        QString m_tag;
    };

    explicit ModbusRequest(int slaveId, ModbusFunctionCode functionCode, 
                          RequestPriority priority = RequestPriority::Normal,
                          QObject* parent = nullptr);
    virtual ~ModbusRequest();

    // 请求信息
    qint64 requestId() const { return m_requestId; }
    int slaveId() const { return m_slaveId; }
    ModbusFunctionCode functionCode() const { return m_functionCode; }
    RequestPriority priority() const { return m_priority; }
    RequestStatus status() const { return m_status; }
    const QString& tag() const { return m_tag; }
    
    // 时间信息
    qint64 creationTime() const { return m_creationTime; }
    qint64 queueTime() const { return m_queueTime; }
    qint64 startTime() const { return m_startTime; }
    qint64 completionTime() const { return m_completionTime; }
    qint64 responseTime() const { return m_responseTime; }
    
    // 配置
    int timeout() const { return m_timeout; }
    int maxRetries() const { return m_maxRetries; }
    int retryCount() const { return m_retryCount; }
    bool useCache() const { return m_useCache; }
    int cacheValidity() const { return m_cacheValidity; }
    
    // 结果
    const ModbusResult& result() const { return m_result; }
    bool hasResult() const { return m_status == RequestStatus::Completed || 
                                  m_status == RequestStatus::Failed; }
    
    // 状态管理
    void setStatus(RequestStatus status);
    void setResult(const ModbusResult& result);
    void incrementRetryCount() { m_retryCount++; }
    bool canRetry() const { return m_retryCount < m_maxRetries; }
    
    // 请求构建
    virtual QByteArray buildRequestData() = 0;
    virtual ModbusResult parseResponse(const QByteArray& response) = 0;
    virtual QString description() const = 0;
    
    // 取消请求
    void cancel();
    bool isCancelled() const { return m_status == RequestStatus::Cancelled; }
    
    // 超时处理
    void startTimeoutTimer();
    void stopTimeoutTimer();
    bool isTimeout() const { return m_timeoutTimer && m_timeoutTimer->isActive(); }
    
    // 静态方法
    static QString exceptionToString(ModbusException exception);
    static QString functionCodeToString(ModbusFunctionCode functionCode);
    static QString priorityToString(RequestPriority priority);
    
    void setTimeout(int newTimeout);

    void setUseCache(bool newUseCache);

signals:
    void statusChanged(qint64 requestId, RequestStatus status);
    void completed(qint64 requestId, const ModbusResult& result);
    void failed(qint64 requestId, const QString& error);
    void cancelled(qint64 requestId);
    void timeout(qint64 requestId);
    void retrying(qint64 requestId, int retryCount);
    
private slots:
    void onTimeout();

private:
    static std::atomic<qint64> s_nextRequestId;
    
    qint64 m_requestId;
    int m_slaveId;
    ModbusFunctionCode m_functionCode;
    RequestPriority m_priority;
    RequestStatus m_status;
    ModbusResult m_result;
    
    // 时间戳
    qint64 m_creationTime;
    qint64 m_queueTime;
    qint64 m_startTime;
    qint64 m_completionTime;
    qint64 m_responseTime;
    
    // 配置
    QString m_tag;
    int m_timeout;
    int m_maxRetries;
    int m_retryCount;
    bool m_useCache;
    int m_cacheValidity;
    
    // 定时器
    QTimer* m_timeoutTimer;
};

// 具体请求类型实现
class ReadHoldingRegistersRequest : public ModbusRequest
{
    Q_OBJECT
public:
    ReadHoldingRegistersRequest(int slaveId, int startAddr, int quantity,
                               RequestPriority priority = RequestPriority::Normal);
    
    QByteArray buildRequestData() override;
    ModbusResult parseResponse(const QByteArray& response) override;
    QString description() const override;
    
    int quantity() const;

private:
    int m_startAddr;
    int m_quantity;
};

class WriteMultipleRegistersRequest : public ModbusRequest {
public:
    WriteMultipleRegistersRequest(int slaveId, int startAddr, 
                                 const QVector<quint16>& values,
                                 RequestPriority priority = RequestPriority::Normal);
    
    QByteArray buildRequestData() override;
    ModbusResult parseResponse(const QByteArray& response) override;
    QString description() const override;
    
private:
    int m_startAddr;
    QVector<quint16> m_values;
};

class ReadCoilsRequest : public ModbusRequest {
public:
    ReadCoilsRequest(int slaveId, int startAddr, int quantity,
                    RequestPriority priority = RequestPriority::Normal);
    
    QByteArray buildRequestData() override;
    ModbusResult parseResponse(const QByteArray& response) override;
    QString description() const override;
        
private:
    int m_startAddr;
    int m_quantity;
};

class WriteSingleCoilRequest : public ModbusRequest {
public:
    WriteSingleCoilRequest(int slaveId, int addr, bool value,
                          RequestPriority priority = RequestPriority::Normal);
    
    QByteArray buildRequestData() override;
    ModbusResult parseResponse(const QByteArray& response) override;
    QString description() const override;
    
private:
    int m_addr;
    bool m_value;
};

class ReadInputRegistersRequest : public ModbusRequest {
public:
    ReadInputRegistersRequest(int slaveId, int startAddr, int quantity,
                             RequestPriority priority = RequestPriority::Normal);
    
    QByteArray buildRequestData() override;
    ModbusResult parseResponse(const QByteArray& response) override;
    QString description() const override;
    
    int quantity() const;

private:
    int m_startAddr;
    int m_quantity;
};

class WriteMultipleCoilsRequest : public ModbusRequest {
public:
    WriteMultipleCoilsRequest(int slaveId, int startAddr, 
                             const QVector<bool>& values,
                             RequestPriority priority = RequestPriority::Normal);
    
    QByteArray buildRequestData() override;
    ModbusResult parseResponse(const QByteArray& response) override;
    QString description() const override;
    
private:
    int m_startAddr;
    QVector<bool> m_values;
};

class ReadDiscreteInputsRequest : public ModbusRequest {
public:
    ReadDiscreteInputsRequest(int slaveId, int startAddr, int quantity,
                             RequestPriority priority = RequestPriority::Normal);
    
    QByteArray buildRequestData() override;
    ModbusResult parseResponse(const QByteArray& response) override;
    QString description() const override;
    
    int quantity() const;

private:
    int m_startAddr;
    int m_quantity;
};

class WriteSingleRegisterRequest : public ModbusRequest {
public:
    WriteSingleRegisterRequest(int slaveId, int addr, quint16 value,
                              RequestPriority priority = RequestPriority::Normal);
    
    QByteArray buildRequestData() override;
    ModbusResult parseResponse(const QByteArray& response) override;
    QString description() const override;
    
private:
    int m_addr;
    quint16 m_value;
};

#endif // MODBUS_REQUEST_H
