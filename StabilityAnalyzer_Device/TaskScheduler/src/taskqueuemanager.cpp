#include "taskqueuemanager.h"
#include <QDebug>
#include <QMutexLocker>

TaskQueueManager::TaskQueueManager(QObject *parent)
    : QObject(parent)
    , m_isRunning(false)
    , m_isPaused(false)
    , m_workerBusy(false)
    , m_executionThread(new QThread(this))
    , m_executionWorker(nullptr)
    , m_schedulerTimer(new QTimer(this))
{
    qRegisterMetaType<TaskResult>("TaskResult");
    qRegisterMetaType<QVector<quint16>>("QVector<quint16>");

    m_executionWorker = new TaskExecutionWorker();
    m_executionWorker->moveToThread(m_executionThread);

    connect(m_executionWorker, &TaskExecutionWorker::taskCompleted,
            this, &TaskQueueManager::onWorkerTaskCompleted);

    connect(m_executionWorker, &TaskExecutionWorker::taskError,
            this, [](const QString &error) {
        qWarning() << "Task execution error:" << error;
    });

    m_executionThread->start();

    m_schedulerTimer->setInterval(100);
    connect(m_schedulerTimer, &QTimer::timeout, this, [this]() {
        QMutexLocker locker(&m_queueMutex);
        processNextTask();
    });
}

int TaskQueueManager::highPriorityQueueSize() const
{
    QMutexLocker locker(&m_queueMutex);
    return m_highPriorityQueue.size();
}

int TaskQueueManager::pollingQueueSize() const
{
    QMutexLocker locker(&m_queueMutex);
    return m_pollingQueue.size();
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
    processNextTask();
}

void TaskQueueManager::stopScheduler()
{
    if (!m_isRunning) {
        return;
    }

    m_isRunning = false;
    m_isPaused = false;

    m_schedulerTimer->stop();

    if (m_executionWorker) {
        QMetaObject::invokeMethod(m_executionWorker, "stop", Qt::QueuedConnection);
    }

    if (m_executionThread->isRunning()) {
        m_executionThread->quit();
        m_executionThread->wait(5000);
        if (m_executionThread->isRunning()) {
            m_executionThread->terminate();
            m_executionThread->wait();
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
    processNextTask();
}

void TaskQueueManager::addHighPriorityTask(const QString &deviceId, Task *task)
{
    if (!task || !m_isRunning) {
        return;
    }

    QMutexLocker locker(&m_queueMutex);
    QString taskMapKey = deviceId + "_" + task->taskName();
    QueuedTask queuedTask(deviceId, task);
    m_highPriorityQueue.enqueue(queuedTask);
    m_taskMap.insert(taskMapKey, queuedTask);

    updateQueueStatus();
    processNextTask();
}

void TaskQueueManager::addPollingTask(const QString &deviceId, Task *task)
{
    if (!task || !m_isRunning) {
        return;
    }

    QMutexLocker locker(&m_queueMutex);

    for (const QueuedTask &qt : m_pollingQueue) {
        if (qt.deviceId == deviceId && qt.task && qt.task->taskName() == task->taskName()) {
            return;
        }
    }

    QString taskMapKey = deviceId + "_" + task->taskName();
    QueuedTask queuedTask(deviceId, task);
    m_pollingQueue.enqueue(queuedTask);
    m_taskMap.insert(taskMapKey, queuedTask);

    updateQueueStatus();
    processNextTask();
}

void TaskQueueManager::initializeDeviceTasks(const QString &deviceId, const QList<Task*> &tasks)
{
    if (!m_isRunning) {
        return;
    }

    QMutexLocker locker(&m_queueMutex);

    for (Task *task : tasks) {
        if (task) {
            if (task->taskType() == TaskType::INIT_TASK) {
                QString taskMapKey = deviceId + "_" + task->taskName();
                QueuedTask queuedTask(deviceId, task);
                m_pollingQueue.enqueue(queuedTask);
                m_taskMap.insert(taskMapKey, queuedTask);

                qDebug() << "设备初始化任务- Device:" << deviceId
                         << "任务名称:" << task->taskName()
                         << "轮询间隔:" << task->interval();
            } else {
                qWarning() << "设备初始化跳过用户任务- Device:" << deviceId
                           << "任务名称:" << task->taskName()
                           << "任务类型:" << "USER_TASK";
            }
        }
    }

    updateQueueStatus();
    processNextTask();
}

void TaskQueueManager::removeTask(const QString &taskName)
{
    QMutexLocker locker(&m_queueMutex);

    for (auto it = m_highPriorityQueue.begin(); it != m_highPriorityQueue.end(); ++it) {
        if (it->task && it->task->taskName() == taskName) {
            m_highPriorityQueue.erase(it);
            break;
        }
    }

    for (auto it = m_pollingQueue.begin(); it != m_pollingQueue.end(); ++it) {
        if (it->task && it->task->taskName() == taskName) {
            m_pollingQueue.erase(it);
            break;
        }
    }

    updateQueueStatus();
}

void TaskQueueManager::clearAllTasks()
{
    m_highPriorityQueue.clear();
    m_pollingQueue.clear();
    m_workerBusy = false;
    m_taskMap.clear();

    updateQueueStatus();
}

QList<QString> TaskQueueManager::getHighPriorityTaskList() const
{
    QMutexLocker locker(&m_queueMutex);

    QList<QString> taskList;
    for (const QueuedTask &task : m_highPriorityQueue) {
        if (task.task) {
            taskList.append(task.task->taskName());
        }
    }
    return taskList;
}

QList<QString> TaskQueueManager::getPollingTaskList() const
{
    QMutexLocker locker(&m_queueMutex);

    QList<QString> taskList;
    for (const QueuedTask &task : m_pollingQueue) {
        if (task.task) {
            taskList.append(task.task->taskName());
        }
    }
    return taskList;
}

void TaskQueueManager::processNextTask()
{
    if (!m_isRunning || m_isPaused || m_workerBusy) {
        return;
    }

    QueuedTask nextTask("", nullptr);

    if (!m_highPriorityQueue.isEmpty()) {
        nextTask = m_highPriorityQueue.dequeue();
    } else if (!m_pollingQueue.isEmpty()) {
        nextTask = m_pollingQueue.dequeue();
    } else {
        return;
    }

    if (!nextTask.task) {
        return;
    }

    QString deviceId = nextTask.deviceId;
    QString taskId = nextTask.task->taskName();

    if (!nextTask.task->device()) {
        qWarning() << "Task has no valid device object - Device:" << deviceId
                   << "Task:" << taskId;
        processNextTask();
        return;
    }

    m_workerBusy = true;

    emit taskStarted(deviceId, taskId);

    const bool invokeOk = QMetaObject::invokeMethod(m_executionWorker, "executeTask",
                                                    Qt::QueuedConnection,
                                                    Q_ARG(Task*, nextTask.task));
    if (!invokeOk) {
        qWarning() << "Failed to dispatch task to execution worker - Device:" << deviceId
                   << "Task:" << taskId;
        m_workerBusy = false;
        processNextTask();
        return;
    }

    updateQueueStatus();
}

void TaskQueueManager::onWorkerTaskCompleted(TaskResult res, QVector<quint16> data)
{
    QMutexLocker locker(&m_queueMutex);
    m_workerBusy = false;

    emit taskCompleted(res, data);

    updateQueueStatus();
    processNextTask();
}

void TaskQueueManager::updateQueueStatus()
{
    emit queueStatusChanged(m_highPriorityQueue.size(), m_pollingQueue.size());
}
