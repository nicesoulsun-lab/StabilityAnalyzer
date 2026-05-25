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

class TASKSCHEDULER_EXPORT TaskQueueManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool isRunning READ isRunning NOTIFY runningStatusChanged)
    Q_PROPERTY(int highPriorityQueueSize READ highPriorityQueueSize NOTIFY queueStatusChanged)
    Q_PROPERTY(int pollingQueueSize READ pollingQueueSize NOTIFY queueStatusChanged)

public:
    explicit TaskQueueManager(QObject *parent = nullptr);

    bool isRunning() const { return m_isRunning; }
    int highPriorityQueueSize() const;
    int pollingQueueSize() const;

    void startScheduler();
    void stopScheduler();
    void pauseScheduler();
    void resumeScheduler();

    void addHighPriorityTask(const QString &deviceId, Task *task);
    void addPollingTask(const QString &deviceId, Task *task);
    void initializeDeviceTasks(const QString &deviceId, const QList<Task*> &tasks);
    void removeTask(const QString &taskName);
    void clearAllTasks();

    QList<QString> getHighPriorityTaskList() const;
    QList<QString> getPollingTaskList() const;

signals:
    void taskStarted(const QString &deviceId, const QString &taskName);
    void taskCompleted(TaskResult res, QVector<quint16>data);
    void queueStatusChanged(int highPriorityCount, int pollingCount);
    void runningStatusChanged(bool running);
    void errorOccurred(const QString &error);

private slots:
    void onWorkerTaskCompleted(TaskResult res, QVector<quint16>data);

private:
    struct QueuedTask {
        QString deviceId;
        Task *task;

        QueuedTask() : deviceId(""), task(nullptr) {}
        QueuedTask(const QString &devId, Task *t)
            : deviceId(devId), task(t) {}
    };

    bool m_isRunning;
    bool m_isPaused;
    bool m_workerBusy;

    QThread *m_executionThread;
    TaskExecutionWorker *m_executionWorker;

    QTimer *m_schedulerTimer;

    QQueue<QueuedTask> m_highPriorityQueue;
    QQueue<QueuedTask> m_pollingQueue;

    mutable QMutex m_queueMutex;

    QMap<QString, QueuedTask> m_taskMap;

    void processNextTask();
    void updateQueueStatus();
};

#endif // TASKQUEUEMANAGER_H
