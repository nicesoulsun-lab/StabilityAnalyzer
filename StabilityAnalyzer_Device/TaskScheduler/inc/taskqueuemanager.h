#ifndef TASKQUEUEMANAGER_H
#define TASKQUEUEMANAGER_H

#include <QObject>
#include <QQueue>
#include <QStack>
#include <QTimer>
#include <QMap>
#include <QString>
#include <QMutex>
#include <QThread>
#include "taskscheduler_global.h"
#include "task.h"
#include "taskexecutionworker.h"
class Task;

/**
 * @brief 任务队列管理器
 * 
 * 实现双队列任务调度系统：
 * - 高优先级队列（FIFO）：用于应用层触发的流程性任务，这个任务是强顺序的，也就是如队列的时候是1，2，3，那执行的时候也必须按照1，2，3的顺序执行，先进先出
 * - 轮询队列（FIFO）：用于初始化轮询任务和定时任务，先进先出，配合去重机制保证同一任务不重复积压
 * - 高优先级队列优先于轮询队列执行，如果高优先级队列的任务都执行完执行才执行轮询队列的任务
 * - 任务状态跟踪和结果处理，一个请求任务执行完之后执行队列的下一个请求任务
 */
class TASKSCHEDULER_EXPORT TaskQueueManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool isRunning READ isRunning NOTIFY runningStatusChanged)
    Q_PROPERTY(int highPriorityQueueSize READ highPriorityQueueSize NOTIFY queueStatusChanged)
    Q_PROPERTY(int pollingQueueSize READ pollingQueueSize NOTIFY queueStatusChanged)

public:
    explicit TaskQueueManager(QObject *parent = nullptr);
    
    // 队列状态访问器
    bool isRunning() const { return m_isRunning; }
    int highPriorityQueueSize() const;
    int pollingQueueSize() const;
    
    // 队列管理
    void startScheduler();
    void stopScheduler();
    void pauseScheduler();
    void resumeScheduler();
    
    // 任务管理
    void addHighPriorityTask(const QString &deviceId, Task *task);
    void addPollingTask(const QString &deviceId, Task *task);
    void initializeDeviceTasks(const QString &deviceId, const QList<Task*> &tasks);
    void removeTask(const QString &taskName);
    void clearAllTasks();
    
    // 队列状态查询
    QList<QString> getHighPriorityTaskList() const;
    QList<QString> getPollingTaskList() const;

signals:
    void taskStarted(const QString &deviceId, const QString &taskName);
    void taskCompleted(TaskResult res, QVector<quint16>data);
    void queueStatusChanged(int highPriorityCount, int pollingCount);
    void runningStatusChanged(bool running);
    void errorOccurred(const QString &error);

private slots:
    void processNextTask();
//    void onTaskCompleted(bool success, const QVariant &result);
    void onTaskCompleted(TaskResult res, QVector<quint16>data);
    void onWorkerTaskCompleted(TaskResult res, QVector<quint16>data);

private:
    struct QueuedTask {
        QString deviceId;
        Task *task;
        
        QueuedTask() : deviceId(""), task(nullptr) {}
        QueuedTask(const QString &devId, Task *t) : deviceId(devId), task(t) {}
    };
    
    void setupExecutionThread();
    
    void executeTask(const QueuedTask &queuedTask);
    void updateQueueStatus();
    void waitForTaskCompletion(Task *task);
    
    bool m_isRunning;
    bool m_isPaused;
    
    // 双队列系统
    QQueue<QueuedTask> m_highPriorityQueue;  //< 高优先级队列（FIFO）
    QQueue<QueuedTask> m_pollingQueue;       //< 轮询队列（FIFO）
    
    // 任务调度定时器
    QTimer *m_schedulerTimer;
    
    // 任务执行线程
    QThread *m_executionThread;
    TaskExecutionWorker *m_executionWorker;
    bool m_workerBusy;
    
    // 线程安全
    mutable QMutex m_queueMutex;
    
    // 任务映射
    QMap<QString, QueuedTask> m_taskMap;  //< 任务ID到队列任务的映射
};

#endif // TASKQUEUEMANAGER_H
