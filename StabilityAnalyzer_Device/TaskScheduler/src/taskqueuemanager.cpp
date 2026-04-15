#include "taskqueuemanager.h"
//#include "task.h"
#include <QDebug>
#include <QMutexLocker>

/**
 * @brief TaskQueueManager 构造函数
 * @param parent 父对象指针
 */
TaskQueueManager::TaskQueueManager(QObject *parent)
    : QObject(parent)
    , m_isRunning(false)
    , m_isPaused(false)
    , m_schedulerTimer(new QTimer(this))
    , m_executionThread(new QThread(this))
    , m_executionWorker(nullptr)
    , m_workerBusy(false)
{
    qRegisterMetaType<TaskResult>("TaskResult");
    qRegisterMetaType<QVector<quint16>>("QVector<quint16>");

    // 设置调度定时器，暂时先设置500ms吧
    m_schedulerTimer->setInterval(100); // 每2秒检查一次队列
    connect(m_schedulerTimer, &QTimer::timeout, this, &TaskQueueManager::processNextTask);
    
    // 初始化执行线程
    setupExecutionThread();
}

/**
 * @brief 设置任务执行线程
 */
void TaskQueueManager::setupExecutionThread()
{
    // 创建工作对象
    m_executionWorker = new TaskExecutionWorker();
    
    // 将工作对象移动到子线程
    m_executionWorker->moveToThread(m_executionThread);
    
    // 连接工作线程的任务完成信号
    connect(m_executionWorker, &TaskExecutionWorker::taskCompleted,
            this, &TaskQueueManager::onWorkerTaskCompleted, Qt::QueuedConnection);
    
    // 连接工作线程的错误信号
    connect(m_executionWorker, &TaskExecutionWorker::taskError,
            this, [this](const QString &error) {
                qWarning() << "Task execution error:" << error;
            }, Qt::QueuedConnection);
    
    // 启动执行线程
    m_executionThread->start();
    
    qDebug() << "Task execution thread started";
}

/**
 * @brief 获取高优先级队列大小
 * @return 队列中的任务数量
 */
int TaskQueueManager::highPriorityQueueSize() const
{
    QMutexLocker locker(&m_queueMutex);
    return m_highPriorityQueue.size();
}

/**
 * @brief 获取轮询队列大小
 * @return 队列中的任务数量
 */
int TaskQueueManager::pollingQueueSize() const
{
    QMutexLocker locker(&m_queueMutex);
    return m_pollingQueue.size();
}

/**
 * @brief 启动任务调度器
 */
void TaskQueueManager::startScheduler()
{
    if (m_isRunning) {
        qWarning() << "Task scheduler is already running";
        return;
    }
    
    m_isRunning = true;
    m_isPaused = false;
    
    // 启动调度定时器
    m_schedulerTimer->start();
    
    qDebug() << "启动任务调度器";
    emit runningStatusChanged(true);
}

/**
 * @brief 停止任务调度器
 */
void TaskQueueManager::stopScheduler()
{
    if (!m_isRunning) {
        return;
    }
    
    m_isRunning = false;
    m_isPaused = false;
    
    // 停止调度定时器
    m_schedulerTimer->stop();
    
    // 停止工作线程
    if (m_executionWorker) {
        QMetaObject::invokeMethod(m_executionWorker, "stop", Qt::QueuedConnection);
    }
    
    if (m_executionThread && m_executionThread->isRunning()) {
        m_executionThread->quit();
        m_executionThread->wait(5000); // 等待5秒
        if (m_executionThread->isRunning()) {
            m_executionThread->terminate();
            m_executionThread->wait();
        }
    }
    
    // 清空队列
    clearAllTasks();
    
    qDebug() << "停止任务调度器";
    emit runningStatusChanged(false);
}

/**
 * @brief 暂停任务调度器
 */
void TaskQueueManager::pauseScheduler()
{
    if (!m_isRunning || m_isPaused) {
        return;
    }
    
    m_isPaused = true;
    
    qDebug() << "暂停任务调度器";
}

/**
 * @brief 恢复任务调度器
 */
void TaskQueueManager::resumeScheduler()
{
    if (!m_isRunning || !m_isPaused) {
        return;
    }
    
    m_isPaused = false;
    
    qDebug() << "恢复任务调度器";
}

/**
 * @brief 添加高优先级任务（FIFO）
 * @param deviceId 设备ID
 * @param task 任务对象
 */
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
}

/**
 * @brief 添加轮询任务（LIFO）
 * @param deviceId 设备ID
 * @param task 任务对象
 */
void TaskQueueManager::addPollingTask(const QString &deviceId, Task *task)
{
    if (!task || !m_isRunning) {
        return;
    }
    
    QMutexLocker locker(&m_queueMutex);

    // 去重,如果队列中已存在相同设备+任务名的任务，则不再重复入队
    for (const QueuedTask &qt : m_pollingQueue) {
        if (qt.deviceId == deviceId && qt.task && qt.task->taskName() == task->taskName()) {
            qDebug() << "轮询任务已在队列中，跳过入队 - Device:" << deviceId
                     << "任务名:" << task->taskName();
            return;
        }
    }

    QString taskMapKey = deviceId + "_" + task->taskName();
    QueuedTask queuedTask(deviceId, task);
    m_pollingQueue.enqueue(queuedTask);
    m_taskMap.insert(taskMapKey, queuedTask);
    
    qDebug() << "添加轮询任务 - Device:" << deviceId
             << "任务名:" << task->taskName()
             << "队列size:" << m_pollingQueue.size();
    
    updateQueueStatus();
}

/**
 * @brief 初始化设备任务
 * @param deviceId 设备ID
 * @param tasks 任务列表
 */
void TaskQueueManager::initializeDeviceTasks(const QString &deviceId, const QList<Task*> &tasks)
{
    if (!m_isRunning) {
        return;
    }
    
    QMutexLocker locker(&m_queueMutex);
    
    for (Task *task : tasks) {
        if (task) {
            // 检查任务类型，INIT_TASK配置的任务被添加到轮询队列
            if (task->taskType() == TaskType::INIT_TASK) {

                QString taskMapKey = deviceId + "_" + task->taskName();
                QueuedTask queuedTask(deviceId, task);
                m_pollingQueue.enqueue(queuedTask);
                m_taskMap.insert(taskMapKey, queuedTask);
                
                qDebug() << "设备初始化任务- Device:" << deviceId
                         << "任务名称:" << task->taskName()
                         << "任务指针:" << task
                         << "设备指针:" << task->device()
                         << "任务类型:" << (task->taskType() == TaskType::INIT_TASK ? "INIT_TASK" : "USER_TASK")
                         << "轮询间隔:" << task->interval();
            } else {
                // 用户任务（USER_TASK），不被添加到初始化队列
                qWarning() << "设备初始化跳过用户任务- Device:" << deviceId
                           << "任务名称:" << task->taskName()
                           << "任务类型:" << "USER_TASK"
                           << "轮询间隔:" << task->interval()
                           << "- 此任务只能在用户触发时执行";
            }
        }
    }
    
    updateQueueStatus();
}

/**
 * @brief 移除任务
 * @param taskName 任务名称
 */
void TaskQueueManager::removeTask(const QString &taskName)
{
    QMutexLocker locker(&m_queueMutex);
    
    // 从高优先级队列中移除
    for (auto it = m_highPriorityQueue.begin(); it != m_highPriorityQueue.end(); ++it) {
        if (it->task && it->task->taskName() == taskName) {
            m_highPriorityQueue.erase(it);
            break;
        }
    }
    
    // 从轮询队列中移除
    for (auto it = m_pollingQueue.begin(); it != m_pollingQueue.end(); ++it) {
        if (it->task && it->task->taskName() == taskName) {
            m_pollingQueue.erase(it);
            break;
        }
    }
    
    updateQueueStatus();
}

/**
 * @brief 清空所有任务
 */
void TaskQueueManager::clearAllTasks()
{
    QMutexLocker locker(&m_queueMutex);
    
    m_highPriorityQueue.clear();
    m_pollingQueue.clear();
    m_taskMap.clear();
    m_workerBusy = false;
    
    updateQueueStatus();
}

/**
 * @brief 获取高优先级任务列表
 * @return 任务ID列表
 */
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

/**
 * @brief 获取轮询任务列表
 * @return 任务ID列表
 */
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

/**
 * @brief 处理下一个任务
 */
void TaskQueueManager::processNextTask()
{
    if (m_isPaused) {
        return;
    }

    if(m_highPriorityQueue.size()<=0 && m_pollingQueue.size()<=0){
        return;
    }
    
    QMutexLocker locker(&m_queueMutex);
    QueuedTask nextTask("", nullptr);
    
    // 优先处理高优先级队列
    if (!m_highPriorityQueue.isEmpty()) {
        nextTask = m_highPriorityQueue.dequeue();
    } 
    // 然后处理轮询队列
    else if (!m_pollingQueue.isEmpty()) {
        nextTask = m_pollingQueue.dequeue();
    }
    
    locker.unlock();
    
    if (nextTask.task) {
        // 同步/异步任务统一直接投递执行，等待逻辑由调用方 waitForTaskCompletion 负责
        // 避免在定时器回调（主线程）中嵌套 QEventLoop 导致事件循环重入问题
        executeTask(nextTask);
    }

    qDebug()<<"执行下一条指令";
    updateQueueStatus();
}

/**
 * @brief 执行任务
 * @param queuedTask 队列任务
 */
void TaskQueueManager::executeTask(const QueuedTask &queuedTask)
{
    if (!queuedTask.task) {
        return;
    }
    
    QString deviceId = queuedTask.deviceId;
    QString taskId = queuedTask.task->taskName();
    QString uniqueTaskId = queuedTask.task->taskId();
    
    // qDebug() << "Executing task - Device:" << deviceId
    //          << "Task:" << taskId
    //          << "Task ID:" << uniqueTaskId;
    
    // 检查任务是否有有效的设备对象
    if (!queuedTask.task->device()) {
        qWarning() << "Task has no valid device object - Device:" << deviceId 
                   << "Task:" << taskId 
                   << "Task ID:" << uniqueTaskId;
        return;
    }
    
    // 发出任务开始信号
    emit taskStarted(deviceId, taskId);
    
    // 在工作线程中执行任务
    QMetaObject::invokeMethod(m_executionWorker, "executeTask", 
                              Qt::QueuedConnection,
                              Q_ARG(Task*, queuedTask.task));
}

/**
 * @brief 处理工作线程的任务完成信号
 */
void TaskQueueManager::onWorkerTaskCompleted(TaskResult res, QVector<quint16> data)
{
//    qDebug() << "Task completed in worker thread - Device:" << res.deviceId
//             << "Task:" << res.taskName
//             << "Task ID:" << res.taskId
//             << "Success:" << !res.isException
//             << "Data size:" << data.size();
    
    // 发出任务完成信号
    emit taskCompleted(res, data);
    
    // 更新队列状态
    updateQueueStatus();
}

/**
 * @brief 处理任务完成信号
 * @param success 执行是否成功
 * @param result 执行结果
 * task传递过来的信号
 */
void TaskQueueManager::onTaskCompleted(TaskResult res, QVector<quint16>data)
{
    // 发出任务完成信号
    emit taskCompleted(res, data);
    
    // 更新队列状态
    updateQueueStatus();
}

/**
 * @brief 更新队列状态
 */
void TaskQueueManager::updateQueueStatus()
{
    int highPriorityCount = m_highPriorityQueue.size();
    int pollingCount = m_pollingQueue.size();
    
    emit queueStatusChanged(highPriorityCount, pollingCount);
}

/**
 * @brief 等待同步任务完成
 * @param task 需要等待的任务
 */
void TaskQueueManager::waitForTaskCompletion(Task *task)
{
    if (!task || !task->isSync()) {
        return;
    }
    
    QEventLoop eventLoop;
    QTimer timeoutTimer;
    
    // 设置超时时间（10秒）
    timeoutTimer.setSingleShot(true);
    timeoutTimer.start(10000);
    
    // 连接任务完成信号
    QObject::connect(task, &Task::taskCompleted, &eventLoop, &QEventLoop::quit);
    
    // 连接超时信号
    QObject::connect(&timeoutTimer, &QTimer::timeout, &eventLoop, &QEventLoop::quit);
    
    // 阻塞并等待任务完成或超时
    eventLoop.exec();
    
    if (timeoutTimer.isActive()) {
        // 任务正常完成
        timeoutTimer.stop();
        // qDebug() << "Sync task completed successfully:" << task->taskName() << "Task ID:" << task->taskName();
    } else {
        // 任务超时
        qWarning() << "Sync task timeout:" << task->taskName() << "Task ID:" << task->taskName();
        
        // 取消同步任务
        task->cancel();
    }
}
