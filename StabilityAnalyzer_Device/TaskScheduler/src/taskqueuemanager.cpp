#include "taskqueuemanager.h"
#include <QDebug>
#include <QMutexLocker>

TaskQueueManager::TaskQueueManager(QObject *parent)
    : QObject(parent)
    , m_isRunning(false)
    , m_isPaused(false)
    , m_schedulerTimer(new QTimer(this))
{
    qRegisterMetaType<TaskResult>("TaskResult");
    qRegisterMetaType<QVector<quint16>>("QVector<quint16>");

    m_schedulerTimer->setInterval(100);
    connect(m_schedulerTimer, &QTimer::timeout, this, [this]() {
        QMutexLocker locker(&m_queueMutex);
        for (auto it = m_portQueues.begin(); it != m_portQueues.end(); ++it)
            requestDispatch(it.value());
    });
}

int TaskQueueManager::highPriorityQueueSize() const
{
    QMutexLocker locker(&m_queueMutex);
    int total = 0;
    for (auto it = m_portQueues.constBegin(); it != m_portQueues.constEnd(); ++it)
        total += it.value()->highPriorityQueue.size();
    return total;
}

int TaskQueueManager::pollingQueueSize() const
{
    QMutexLocker locker(&m_queueMutex);
    int total = 0;
    for (auto it = m_portQueues.constBegin(); it != m_portQueues.constEnd(); ++it)
        total += it.value()->pollingQueue.size();
    return total;
}

void TaskQueueManager::startScheduler()
{
    if (m_isRunning) {
        qWarning() << "Task scheduler is already running";
        return;
    }

    m_isRunning = true;
    m_isPaused = false;

    m_schedulerTimer->start();

    qDebug() << "启动任务调度器";
    emit runningStatusChanged(true);

    QMutexLocker locker(&m_queueMutex);
    for (auto it = m_portQueues.begin(); it != m_portQueues.end(); ++it)
        requestDispatch(it.value());
}

void TaskQueueManager::stopScheduler()
{
    if (!m_isRunning) {
        return;
    }

    m_isRunning = false;
    m_isPaused = false;

    m_schedulerTimer->stop();

    QMutexLocker locker(&m_queueMutex);
    for (auto it = m_portQueues.begin(); it != m_portQueues.end(); ++it) {
        PortQueue *pq = it.value();
        if (pq->worker) {
            QMetaObject::invokeMethod(pq->worker, "stop", Qt::QueuedConnection);
        }
        if (pq->thread && pq->thread->isRunning()) {
            locker.unlock();
            pq->thread->quit();
            pq->thread->wait(5000);
            if (pq->thread->isRunning()) {
                pq->thread->terminate();
                pq->thread->wait();
            }
            locker.relock();
        }
    }

    clearAllTasks();

    qDebug() << "停止任务调度器";
    emit runningStatusChanged(false);
}

void TaskQueueManager::pauseScheduler()
{
    if (!m_isRunning || m_isPaused) {
        return;
    }

    m_isPaused = true;
    qDebug() << "暂停任务调度器";
}

void TaskQueueManager::resumeScheduler()
{
    if (!m_isRunning || !m_isPaused) {
        return;
    }

    m_isPaused = false;
    qDebug() << "恢复任务调度器";

    QMutexLocker locker(&m_queueMutex);
    for (auto it = m_portQueues.begin(); it != m_portQueues.end(); ++it)
        requestDispatch(it.value());
}

void TaskQueueManager::registerPort(const QString &portName)
{
    QMutexLocker locker(&m_queueMutex);
    if (m_portQueues.contains(portName)) {
        return;
    }

    PortQueue *pq = new PortQueue();
    pq->portName = portName;
    setupPortWorker(pq);
    m_portQueues.insert(portName, pq);

    qDebug() << "[TaskQueueManager] registered port:" << portName;
}

void TaskQueueManager::unregisterPort(const QString &portName)
{
    QMutexLocker locker(&m_queueMutex);
    if (!m_portQueues.contains(portName)) {
        return;
    }

    PortQueue *pq = m_portQueues.value(portName);
    teardownPortWorker(pq);
    m_portQueues.remove(portName);
    delete pq;

    qDebug() << "[TaskQueueManager] unregistered port:" << portName;
}

void TaskQueueManager::addHighPriorityTask(const QString &portName, const QString &deviceId, Task *task)
{
    if (!task || !m_isRunning) {
        return;
    }

    QMutexLocker locker(&m_queueMutex);
    PortQueue *pq = m_portQueues.value(portName, nullptr);
    if (!pq) {
        qWarning() << "[TaskQueueManager] port not registered:" << portName;
        return;
    }

    QString taskMapKey = deviceId + "_" + task->taskName();
    QueuedTask queuedTask(deviceId, portName, task);
    pq->highPriorityQueue.enqueue(queuedTask);
    m_taskMap.insert(taskMapKey, queuedTask);

    updateQueueStatus();
    requestDispatch(pq);
}

void TaskQueueManager::addPollingTask(const QString &portName, const QString &deviceId, Task *task)
{
    if (!task || !m_isRunning) {
        return;
    }

    QMutexLocker locker(&m_queueMutex);
    PortQueue *pq = m_portQueues.value(portName, nullptr);
    if (!pq) {
        qWarning() << "[TaskQueueManager] port not registered:" << portName;
        return;
    }

    for (const QueuedTask &qt : pq->pollingQueue) {
        if (qt.deviceId == deviceId && qt.task && qt.task->taskName() == task->taskName()) {
            return;
        }
    }

    QString taskMapKey = deviceId + "_" + task->taskName();
    QueuedTask queuedTask(deviceId, portName, task);
    pq->pollingQueue.enqueue(queuedTask);
    m_taskMap.insert(taskMapKey, queuedTask);

    updateQueueStatus();
    requestDispatch(pq);
}

void TaskQueueManager::initializeDeviceTasks(const QString &portName, const QString &deviceId, const QList<Task*> &tasks)
{
    if (!m_isRunning) {
        return;
    }

    QMutexLocker locker(&m_queueMutex);
    PortQueue *pq = m_portQueues.value(portName, nullptr);
    if (!pq) {
        qWarning() << "[TaskQueueManager] port not registered:" << portName;
        return;
    }

    for (Task *task : tasks) {
        if (task) {
            if (task->taskType() == TaskType::INIT_TASK) {
                QString taskMapKey = deviceId + "_" + task->taskName();
                QueuedTask queuedTask(deviceId, portName, task);
                pq->pollingQueue.enqueue(queuedTask);
                m_taskMap.insert(taskMapKey, queuedTask);

                qDebug() << "设备初始化任务- Device:" << deviceId
                         << "任务名称:" << task->taskName()
                         << "Port:" << portName
                         << "轮询间隔:" << task->interval();
            } else {
                qWarning() << "设备初始化跳过用户任务- Device:" << deviceId
                           << "任务名称:" << task->taskName()
                           << "任务类型:" << "USER_TASK";
            }
        }
    }

    updateQueueStatus();
    requestDispatch(pq);
}

void TaskQueueManager::removeTask(const QString &taskName)
{
    QMutexLocker locker(&m_queueMutex);

    for (auto it = m_portQueues.begin(); it != m_portQueues.end(); ++it) {
        PortQueue *pq = it.value();

        for (auto qit = pq->highPriorityQueue.begin(); qit != pq->highPriorityQueue.end(); ++qit) {
            if (qit->task && qit->task->taskName() == taskName) {
                pq->highPriorityQueue.erase(qit);
                break;
            }
        }

        for (auto qit = pq->pollingQueue.begin(); qit != pq->pollingQueue.end(); ++qit) {
            if (qit->task && qit->task->taskName() == taskName) {
                pq->pollingQueue.erase(qit);
                break;
            }
        }
    }

    updateQueueStatus();
}

void TaskQueueManager::clearAllTasks()
{
    for (auto it = m_portQueues.begin(); it != m_portQueues.end(); ++it) {
        PortQueue *pq = it.value();
        pq->highPriorityQueue.clear();
        pq->pollingQueue.clear();
        pq->workerBusy = false;
        pq->dispatchPending = false;
    }
    m_taskMap.clear();

    updateQueueStatus();
}

QList<QString> TaskQueueManager::getHighPriorityTaskList() const
{
    QMutexLocker locker(&m_queueMutex);

    QList<QString> taskList;
    for (auto it = m_portQueues.constBegin(); it != m_portQueues.constEnd(); ++it) {
        for (const QueuedTask &task : it.value()->highPriorityQueue) {
            if (task.task) {
                taskList.append(task.task->taskName());
            }
        }
    }
    return taskList;
}

QList<QString> TaskQueueManager::getPollingTaskList() const
{
    QMutexLocker locker(&m_queueMutex);

    QList<QString> taskList;
    for (auto it = m_portQueues.constBegin(); it != m_portQueues.constEnd(); ++it) {
        for (const QueuedTask &task : it.value()->pollingQueue) {
            if (task.task) {
                taskList.append(task.task->taskName());
            }
        }
    }
    return taskList;
}

void TaskQueueManager::setupPortWorker(PortQueue *pq)
{
    pq->worker = new TaskExecutionWorker();
    pq->thread = new QThread(this);
    pq->worker->moveToThread(pq->thread);

    connect(pq->worker, &TaskExecutionWorker::taskCompleted,
            this, &TaskQueueManager::onWorkerTaskCompleted, Qt::QueuedConnection);

    connect(pq->worker, &TaskExecutionWorker::taskError,
            this, [this](const QString &error) {
        qWarning() << "Task execution error:" << error;
    }, Qt::QueuedConnection);

    pq->thread->start();

    qDebug() << "[TaskQueueManager] worker thread started for port:" << pq->portName;
}

void TaskQueueManager::teardownPortWorker(PortQueue *pq)
{
    if (pq->worker) {
        QMetaObject::invokeMethod(pq->worker, "stop", Qt::QueuedConnection);
    }
    if (pq->thread && pq->thread->isRunning()) {
        pq->thread->quit();
        pq->thread->wait(5000);
        if (pq->thread->isRunning()) {
            pq->thread->terminate();
            pq->thread->wait();
        }
    }
}

void TaskQueueManager::requestDispatch(PortQueue *pq)
{
    if (!m_isRunning || m_isPaused || pq->workerBusy || pq->dispatchPending) {
        return;
    }

    if (pq->highPriorityQueue.isEmpty() && pq->pollingQueue.isEmpty()) {
        return;
    }

    pq->dispatchPending = true;

    QMetaObject::invokeMethod(this, [this, pq]() {
        processNextTask(pq);
    }, Qt::QueuedConnection);
}

void TaskQueueManager::processNextTask(PortQueue *pq)
{
    QueuedTask nextTask("", nullptr, nullptr);

    {
        QMutexLocker locker(&m_queueMutex);
        pq->dispatchPending = false;

        if (!m_isRunning || m_isPaused || pq->workerBusy) {
            return;
        }

        if (!pq->highPriorityQueue.isEmpty()) {
            nextTask = pq->highPriorityQueue.dequeue();
        } else if (!pq->pollingQueue.isEmpty()) {
            nextTask = pq->pollingQueue.dequeue();
        } else {
            return;
        }

        pq->workerBusy = true;
    }

    if (!executeTask(pq, nextTask)) {
        QMutexLocker locker(&m_queueMutex);
        pq->workerBusy = false;
        locker.unlock();
        updateQueueStatus();
        requestDispatch(pq);
        return;
    }

    updateQueueStatus();
}

bool TaskQueueManager::executeTask(PortQueue *pq, const QueuedTask &queuedTask)
{
    if (!queuedTask.task) {
        return false;
    }

    QString deviceId = queuedTask.deviceId;
    QString taskId = queuedTask.task->taskName();

    if (!queuedTask.task->device()) {
        qWarning() << "Task has no valid device object - Device:" << deviceId
                   << "Task:" << taskId;
        return false;
    }

    emit taskStarted(deviceId, taskId);

    const bool invokeOk = QMetaObject::invokeMethod(pq->worker, "executeTask",
                                                    Qt::QueuedConnection,
                                                    Q_ARG(Task*, queuedTask.task));
    if (!invokeOk) {
        qWarning() << "Failed to dispatch task to execution worker - Device:" << deviceId
                   << "Task:" << taskId
                   << "Port:" << pq->portName;
        return false;
    }

    return true;
}

void TaskQueueManager::onWorkerTaskCompleted(TaskResult res, QVector<quint16> data)
{
    TaskExecutionWorker *worker = qobject_cast<TaskExecutionWorker*>(sender());
    if (!worker) {
        return;
    }

    PortQueue *completedPq = nullptr;
    {
        QMutexLocker locker(&m_queueMutex);
        for (auto it = m_portQueues.begin(); it != m_portQueues.end(); ++it) {
            if (it.value()->worker == worker) {
                completedPq = it.value();
                completedPq->workerBusy = false;
                break;
            }
        }
    }

    emit taskCompleted(res, data);

    updateQueueStatus();

    if (completedPq) {
        QMutexLocker locker(&m_queueMutex);
        requestDispatch(completedPq);
    }
}

void TaskQueueManager::updateQueueStatus()
{
    int highPriorityCount = 0;
    int pollingCount = 0;

    for (auto it = m_portQueues.constBegin(); it != m_portQueues.constEnd(); ++it) {
        highPriorityCount += it.value()->highPriorityQueue.size();
        pollingCount += it.value()->pollingQueue.size();
    }

    emit queueStatusChanged(highPriorityCount, pollingCount);
}
