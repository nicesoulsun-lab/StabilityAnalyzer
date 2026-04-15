#include "request_queue.h"
#include <QDateTime>
#include <QDebug>
#include <algorithm>
#include <QRandomGenerator>

RequestQueue::RequestQueue(QObject* parent)
    : QObject(parent)
    , m_maxSize(1000)
    , m_enablePriority(true)
{
    m_statsTimer = new QTimer(this);
    connect(m_statsTimer, &QTimer::timeout, this, &RequestQueue::updateStats);
    m_statsTimer->start(1000); // 每秒更新一次统计
}

RequestQueue::~RequestQueue()
{
    clear();
}

bool RequestQueue::enqueue(QSharedPointer<ModbusRequest> request)
{
    if (!request) {
        qWarning() << "尝试入队空请求";
        return false;
    }
    
    // 使用更安全的锁策略，避免多线程死锁
    QWriteLocker locker(&m_lock);

    // 检查队列是否已满（直接计算大小，避免递归调用）
    if (m_maxSize > 0) {
        int totalSize = 0;
        for (const auto& queue : m_priorityQueues) {
            totalSize += queue.size();
        }

        if (totalSize >= m_maxSize) {
            qWarning() << "请求队列已满，丢弃请求:" << request->requestId();
            emit queueFull();
            return false;
        }
    }
    
    // 设置请求状态
    request->setStatus(RequestStatus::Queued);
    
    // 添加到相应优先级的队列
    RequestPriority priority = m_enablePriority ? request->priority() : RequestPriority::Normal;
    m_priorityQueues[priority].enqueue(request);
    
    // 添加到所有请求映射
    qint64 requestId = request->requestId();
    m_allRequests[requestId] = request;
    
    // 更新索引
    updateIndex(requestId, request, true);
    
    // 连接请求信号
    connect(request.data(), static_cast<void (ModbusRequest::*)(qint64, RequestStatus)>(&ModbusRequest::statusChanged),
            this, &RequestQueue::onRequestStatusChanged);
    connect(request.data(), static_cast<void (ModbusRequest::*)(qint64)>(&ModbusRequest::timeout),
            this, &RequestQueue::onRequestTimeout);
    
    // 更新统计
    m_stats.totalRequests++;
    m_stats.pendingRequests++;
    
    // 发射信号
    emit requestEnqueued(requestId);
    
    // 更新队列最大大小统计
    int totalSize = 0;
    for (const auto& queue : m_priorityQueues) {
        totalSize += queue.size();
    }
    
    if (totalSize > m_stats.maxQueueTime) {
        m_stats.maxQueueTime = totalSize;
    }
    
    return true;
}

QSharedPointer<ModbusRequest> RequestQueue::dequeue()
{
    QWriteLocker locker(&m_lock);
    
    if (isEmpty()) {
        return nullptr;
    }
    
    // 根据优先级获取请求
    QSharedPointer<ModbusRequest> request;
    
    if (m_enablePriority) {
        // 按优先级从高到低查找
        for (int prio = static_cast<int>(RequestPriority::System); 
             prio >= static_cast<int>(RequestPriority::Low); --prio) {
            RequestPriority priority = static_cast<RequestPriority>(prio);
            
            if (!m_priorityQueues[priority].isEmpty()) {
                request = m_priorityQueues[priority].dequeue();
                break;
            }
        }
    } else {
        // 不使用优先级，从默认队列获取
        if (!m_priorityQueues[RequestPriority::Normal].isEmpty()) {
            request = m_priorityQueues[RequestPriority::Normal].dequeue();
        }
    }
    
    if (request) {
        qint64 requestId = request->requestId();
        
        // 更新状态
        request->setStatus(RequestStatus::Sending);
        
        // 添加到处理中集合
        m_processingRequests.insert(requestId);
        
        // 更新统计
        m_stats.pendingRequests--;
        m_stats.processingRequests++;
        
        // 计算排队时间
        qint64 queueTime = QDateTime::currentMSecsSinceEpoch() - request->queueTime();
        m_stats.avgQueueTime = ((m_stats.avgQueueTime * (m_stats.totalRequests - 1)) + queueTime) / m_stats.totalRequests;
        
        if (queueTime > m_stats.maxQueueTime) {
            m_stats.maxQueueTime = queueTime;
        }
        
        // 发射信号
        emit requestDequeued(requestId);
        
        // 如果队列为空，发射信号
        if (isEmpty()) {
            emit queueEmpty();
        }
    }
    
    return request;
}

QSharedPointer<ModbusRequest> RequestQueue::peek() const
{
    QReadLocker locker(&m_lock);
    
    if (isEmpty()) {
        return nullptr;
    }
    
    if (m_enablePriority) {
        // 按优先级从高到低查找
        for (int prio = static_cast<int>(RequestPriority::System); 
             prio >= static_cast<int>(RequestPriority::Low); --prio) {
            RequestPriority priority = static_cast<RequestPriority>(prio);
            
            if (!m_priorityQueues[priority].isEmpty()) {
                return m_priorityQueues[priority].head();
            }
        }
    } else {
        if (!m_priorityQueues[RequestPriority::Normal].isEmpty()) {
            return m_priorityQueues[RequestPriority::Normal].head();
        }
    }
    
    return nullptr;
}

bool RequestQueue::cancelRequest(qint64 requestId)
{
    QWriteLocker locker(&m_lock);
    
    QSharedPointer<ModbusRequest> request = m_allRequests.value(requestId);
    if (!request) {
        qWarning() << "取消请求失败: 未找到请求" << requestId;
        return false;
    }
    
    // 从优先级队列中移除
    RequestPriority priority = request->priority();
    QQueue<QSharedPointer<ModbusRequest>>& queue = m_priorityQueues[priority];
    
    for (int i = 0; i < queue.size(); ++i) {
        if (queue[i]->requestId() == requestId) {
            queue.removeAt(i);
            
            // 从所有请求映射中移除
            m_allRequests.remove(requestId);
            
            // 更新索引
            updateIndex(requestId, request, false);
            
            // 从处理中集合移除（如果在处理中）
            m_processingRequests.remove(requestId);
            
            // 更新请求状态
            request->setStatus(RequestStatus::Cancelled);
            
            // 更新统计
            if (request->status() == RequestStatus::Queued) {
                m_stats.pendingRequests--;
            } else if (request->status() == RequestStatus::Sending || 
                      request->status() == RequestStatus::WaitingResponse) {
                m_stats.processingRequests--;
            }
            m_stats.cancelledRequests++;
            
            // 发射信号
            emit requestCancelled(requestId);
            
            // 如果队列为空，发射信号
            if (isEmpty()) {
                emit queueEmpty();
            }
            
            return true;
        }
    }
    
    qWarning() << "取消请求失败: 请求不在队列中" << requestId;
    return false;
}

bool RequestQueue::removeRequest(qint64 requestId)
{
    // 移除请求但不取消它（用于已完成或失败的请求）
    QWriteLocker locker(&m_lock);
    
    QSharedPointer<ModbusRequest> request = m_allRequests.value(requestId);
    if (!request) {
        return false;
    }
    
    // 从所有请求映射中移除
    m_allRequests.remove(requestId);
    
    // 更新索引
    updateIndex(requestId, request, false);
    
    // 从处理中集合移除
    m_processingRequests.remove(requestId);
    
    // 更新统计
    if (request->status() == RequestStatus::Queued) {
        m_stats.pendingRequests--;
    } else if (request->status() == RequestStatus::Sending || 
              request->status() == RequestStatus::WaitingResponse) {
        m_stats.processingRequests--;
    }
    
    return true;
}

void RequestQueue::clear()
{
    QWriteLocker locker(&m_lock);
    
    // 取消所有待处理请求
    for (auto& queue : m_priorityQueues) {
        for (const auto& request : queue) {
            if (request->status() == RequestStatus::Queued ||
                request->status() == RequestStatus::Sending ||
                request->status() == RequestStatus::WaitingResponse) {
                request->setStatus(RequestStatus::Cancelled);
            }
        }
        queue.clear();
    }
    
    // 清空所有映射和索引
    m_allRequests.clear();
    m_tagIndex.clear();
    m_slaveIndex.clear();
    m_processingRequests.clear();
    
    // 重置统计
    resetStats();
    
    emit queueEmpty();
}

void RequestQueue::clearByPriority(RequestPriority priority)
{
    QWriteLocker locker(&m_lock);
    
    QQueue<QSharedPointer<ModbusRequest>>& queue = m_priorityQueues[priority];
    
    for (const auto& request : queue) {
        if (request->status() == RequestStatus::Queued) {
            request->setStatus(RequestStatus::Cancelled);
            emit requestCancelled(request->requestId());
        }
        
        // 从所有请求映射中移除
        m_allRequests.remove(request->requestId());
        
        // 更新索引
        updateIndex(request->requestId(), request, false);
    }
    
    queue.clear();
    
    // 更新统计
    m_stats.pendingRequests = size();
    
    // 如果队列为空，发射信号
    if (isEmpty()) {
        emit queueEmpty();
    }
}

QSharedPointer<ModbusRequest> RequestQueue::findRequest(qint64 requestId) const
{
    QReadLocker locker(&m_lock);
    return m_allRequests.value(requestId);
}

QList<QSharedPointer<ModbusRequest>> RequestQueue::findRequestsByTag(const QString& tag) const
{
    QReadLocker locker(&m_lock);
    
    QList<QSharedPointer<ModbusRequest>> result;
    QList<qint64> requestIds = m_tagIndex.value(tag);
    
    for (qint64 requestId : requestIds) {
        auto request = m_allRequests.value(requestId);
        if (request) {
            result.append(request);
        }
    }
    
    return result;
}

QList<QSharedPointer<ModbusRequest>> RequestQueue::findRequestsBySlave(int slaveId) const
{
    QReadLocker locker(&m_lock);
    
    QList<QSharedPointer<ModbusRequest>> result;
    QList<qint64> requestIds = m_slaveIndex.value(slaveId);
    
    for (qint64 requestId : requestIds) {
        auto request = m_allRequests.value(requestId);
        if (request) {
            result.append(request);
        }
    }
    
    return result;
}

bool RequestQueue::isEmpty() const
{
    QReadLocker locker(&m_lock);
    
    for (const auto& queue : m_priorityQueues) {
        if (!queue.isEmpty()) {
            return false;
        }
    }
    
    return true;
}

int RequestQueue::size() const
{
    QReadLocker locker(&m_lock);
    
    int total = 0;
    for (const auto& queue : m_priorityQueues) {
        total += queue.size();
    }
    
    return total;
}

int RequestQueue::size(RequestPriority priority) const
{
    QReadLocker locker(&m_lock);
    return m_priorityQueues[priority].size();
}

int RequestQueue::pendingCount() const
{
    QReadLocker locker(&m_lock);
    return m_stats.pendingRequests;
}

int RequestQueue::processingCount() const
{
    QReadLocker locker(&m_lock);
    return m_stats.processingRequests;
}

void RequestQueue::setMaxSize(int maxSize)
{
    QWriteLocker locker(&m_lock);
    m_maxSize = maxSize;
}

void RequestQueue::setEnablePriority(bool enable)
{
    QWriteLocker locker(&m_lock);
    m_enablePriority = enable;
}

RequestQueue::QueueStats RequestQueue::stats() const
{
    QReadLocker locker(&m_lock);
    return m_stats;
}

void RequestQueue::resetStats()
{
    QWriteLocker locker(&m_lock);
    m_stats.reset();
    
    // 直接计算大小，避免调用size()方法
    int totalSize = 0;
    for (const auto& queue : m_priorityQueues) {
        totalSize += queue.size();
    }
    m_stats.pendingRequests = totalSize;
}

QList<qint64> RequestQueue::batchEnqueue(const QList<QSharedPointer<ModbusRequest>>& requests)
{
    QList<qint64> successfulIds;
    
    for (const auto& request : requests) {
        if (enqueue(request)) {
            successfulIds.append(request->requestId());
        }
    }
    
    return successfulIds;
}

int RequestQueue::batchCancel(const QList<qint64>& requestIds)
{
    int cancelledCount = 0;
    
    for (qint64 requestId : requestIds) {
        if (cancelRequest(requestId)) {
            cancelledCount++;
        }
    }
    
    return cancelledCount;
}

QList<QSharedPointer<ModbusRequest>> RequestQueue::filterRequests(
    std::function<bool(const QSharedPointer<ModbusRequest>&)> predicate) const
{
    QReadLocker locker(&m_lock);
    
    QList<QSharedPointer<ModbusRequest>> result;
    
    for (const auto& queue : m_priorityQueues) {
        for (const auto& request : queue) {
            if (predicate(request)) {
                result.append(request);
            }
        }
    }
    
    return result;
}

void RequestQueue::onRequestStatusChanged(qint64 requestId, RequestStatus status)
{
    QWriteLocker locker(&m_lock);
    
    auto request = m_allRequests.value(requestId);
    if (!request) {
        return;
    }
    
    switch (status) {
    case RequestStatus::Completed:
    case RequestStatus::Failed:
    case RequestStatus::Cancelled:
    case RequestStatus::Timeout:
        // 从处理中集合移除
        m_processingRequests.remove(requestId);
        
        // 更新统计
        m_stats.processingRequests--;
        
        if (status == RequestStatus::Completed) {
            m_stats.completedRequests++;
        } else if (status == RequestStatus::Failed) {
            m_stats.failedRequests++;
        } else if (status == RequestStatus::Cancelled) {
            m_stats.cancelledRequests++;
        }
        break;
        
    default:
        break;
    }
}

void RequestQueue::onRequestTimeout(qint64 requestId)
{
    // 超时请求处理
    auto request = findRequest(requestId);
    if (request && (request->status() == RequestStatus::Sending || 
                   request->status() == RequestStatus::WaitingResponse)) {
        request->setStatus(RequestStatus::Timeout);
        emit requestTimeout(requestId);
    }
}

void RequestQueue::updateStats()
{
    QueueStats currentStats = stats();
    emit statsUpdated(currentStats);
}

void RequestQueue::updateIndex(qint64 requestId, const QSharedPointer<ModbusRequest>& request, bool add)
{
    QString tag = request->tag();
    int slaveId = request->slaveId();
    
    if (add) {
        if (!tag.isEmpty()) {
            m_tagIndex[tag].append(requestId);
        }
        if (slaveId > 0) {
            m_slaveIndex[slaveId].append(requestId);
        }
    } else {
        if (!tag.isEmpty()) {
            m_tagIndex[tag].removeAll(requestId);
            if (m_tagIndex[tag].isEmpty()) {
                m_tagIndex.remove(tag);
            }
        }
        if (slaveId > 0) {
            m_slaveIndex[slaveId].removeAll(requestId);
            if (m_slaveIndex[slaveId].isEmpty()) {
                m_slaveIndex.remove(slaveId);
            }
        }
    }
}

void RequestQueue::cleanupTimeoutRequests()
{
    QWriteLocker locker(&m_lock);
    
    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
    int timeoutThreshold = 30000; // 30秒超时
    
    for (auto it = m_allRequests.begin(); it != m_allRequests.end();) {
        auto request = it.value();
        
        if (request->status() == RequestStatus::Queued) {
            qint64 queueTime = currentTime - request->queueTime();
            if (queueTime > timeoutThreshold) {
                // 移除超时的请求
                qint64 requestId = request->requestId();
                
                // 从优先级队列中移除
                RequestPriority priority = request->priority();
                QQueue<QSharedPointer<ModbusRequest>>& queue = m_priorityQueues[priority];
                
                for (int i = 0; i < queue.size(); ++i) {
                    if (queue[i]->requestId() == requestId) {
                        queue.removeAt(i);
                        break;
                    }
                }
                
                // 更新索引
                updateIndex(requestId, request, false);
                
                // 从处理中集合移除
                m_processingRequests.remove(requestId);
                
                // 设置请求状态
                request->setStatus(RequestStatus::Timeout);
                
                // 更新统计
                m_stats.pendingRequests--;
                m_stats.failedRequests++;
                
                // 发射信号
                emit requestTimeout(requestId);
                
                it = m_allRequests.erase(it);
                continue;
            }
        }
        ++it;
    }
}

RequestPriority RequestQueue::getNextPriority() const
{
    // 简单的优先级调度算法
    static QMap<RequestPriority, int> priorityWeights = {
        {RequestPriority::System, 10},
        {RequestPriority::Critical, 8},
        {RequestPriority::High, 6},
        {RequestPriority::Normal, 4},
        {RequestPriority::Low, 2}
    };
    
    // 加权随机选择
    int totalWeight = 0;
    QVector<RequestPriority> availablePriorities;
    QVector<int> weights;
    
    for (int prio = static_cast<int>(RequestPriority::Low); 
         prio <= static_cast<int>(RequestPriority::System); ++prio) {
        RequestPriority priority = static_cast<RequestPriority>(prio);
        
        if (!m_priorityQueues[priority].isEmpty()) {
            int weight = priorityWeights[priority];
            totalWeight += weight;
            availablePriorities.append(priority);
            weights.append(weight);
        }
    }
    
    if (availablePriorities.isEmpty()) {
        return RequestPriority::Normal;
    }
    
    // 随机选择
    int random = QRandomGenerator::global()->bounded(totalWeight);
    int accumulated = 0;
    
    for (int i = 0; i < availablePriorities.size(); ++i) {
        accumulated += weights[i];
        if (random < accumulated) {
            return availablePriorities[i];
        }
    }
    
    return availablePriorities.last();
}
