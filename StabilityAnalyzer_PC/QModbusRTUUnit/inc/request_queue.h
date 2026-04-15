#ifndef REQUEST_QUEUE_H
#define REQUEST_QUEUE_H

#include "modbus_request.h"
#include <QObject>
#include <QQueue>
#include <QMap>
#include <QMutex>
#include <QReadWriteLock>
#include <QTimer>
#include <atomic>
#include <QSet>

class RequestQueue : public QObject
{
    Q_OBJECT

public:
    struct QueueStats {
        int totalRequests;
        int pendingRequests;
        int processingRequests;
        int completedRequests;
        int failedRequests;
        int cancelledRequests;
        qint64 avgQueueTime;
        qint64 maxQueueTime;
        
        QueueStats() {
            reset();
        }
        
        void reset() {
            totalRequests = 0;
            pendingRequests = 0;
            processingRequests = 0;
            completedRequests = 0;
            failedRequests = 0;
            cancelledRequests = 0;
            avgQueueTime = 0;
            maxQueueTime = 0;
        }
    };

    explicit RequestQueue(QObject* parent = nullptr);
    ~RequestQueue();
    
    // 队列操作
    bool enqueue(QSharedPointer<ModbusRequest> request);
    QSharedPointer<ModbusRequest> dequeue();
    QSharedPointer<ModbusRequest> peek() const;
    
    // 请求管理
    bool cancelRequest(qint64 requestId);
    bool removeRequest(qint64 requestId);
    void clear();
    void clearByPriority(RequestPriority priority);
    
    // 查找请求
    QSharedPointer<ModbusRequest> findRequest(qint64 requestId) const;
    QList<QSharedPointer<ModbusRequest>> findRequestsByTag(const QString& tag) const;
    QList<QSharedPointer<ModbusRequest>> findRequestsBySlave(int slaveId) const;
    
    // 队列状态
    bool isEmpty() const;
    int size() const;
    int size(RequestPriority priority) const;
    int pendingCount() const;
    int processingCount() const;
    
    // 配置
    void setMaxSize(int maxSize);
    int maxSize() const { return m_maxSize; }
    void setEnablePriority(bool enable);
    bool priorityEnabled() const { return m_enablePriority; }
    
    // 统计信息
    QueueStats stats() const;
    void resetStats();
    
    // 批量操作
    QList<qint64> batchEnqueue(const QList<QSharedPointer<ModbusRequest>>& requests);
    int batchCancel(const QList<qint64>& requestIds);
    
    // 请求过滤
    QList<QSharedPointer<ModbusRequest>> filterRequests(
        std::function<bool(const QSharedPointer<ModbusRequest>&)> predicate) const;
    
signals:
    void requestEnqueued(qint64 requestId);
    void requestDequeued(qint64 requestId);
    void requestCancelled(qint64 requestId);
    void requestTimeout(qint64 requestId);
    void queueEmpty();
    void queueFull();
    void statsUpdated(const RequestQueue::QueueStats& stats);
    
private slots:
    void onRequestStatusChanged(qint64 requestId, RequestStatus status);
    void onRequestTimeout(qint64 requestId);
    void updateStats();
    
private:
    // 多优先级队列
    QMap<RequestPriority, QQueue<QSharedPointer<ModbusRequest>>> m_priorityQueues;
    
    // 请求映射
    QMap<qint64, QSharedPointer<ModbusRequest>> m_allRequests;
    QMap<QString, QList<qint64>> m_tagIndex;
    QMap<int, QList<qint64>> m_slaveIndex;
    
    // 处理中的请求
    QSet<qint64> m_processingRequests;
    
    // 同步机制
    mutable QReadWriteLock m_lock;
    
    // 配置
    int m_maxSize;
    bool m_enablePriority;
    
    // 统计
    QueueStats m_stats;
    QTimer* m_statsTimer;
    
    // 辅助方法
    RequestPriority getNextPriority() const;
    void updateIndex(qint64 requestId, const QSharedPointer<ModbusRequest>& request, bool add);
    void cleanupTimeoutRequests();
};
#endif
