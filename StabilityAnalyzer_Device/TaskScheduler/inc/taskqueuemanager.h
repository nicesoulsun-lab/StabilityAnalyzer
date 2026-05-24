#ifndef TASKQUEUEMANAGER_H
#define TASKQUEUEMANAGER_H

#include <QObject>
#include <QQueue>
#include <QStack>
#include <QTimer>
#include <QMap>
#include <QHash>
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

    void registerPort(const QString &portName);
    void unregisterPort(const QString &portName);

    void addHighPriorityTask(const QString &portName, const QString &deviceId, Task *task);
    void addPollingTask(const QString &portName, const QString &deviceId, Task *task);
    void initializeDeviceTasks(const QString &portName, const QString &deviceId, const QList<Task*> &tasks);
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
        QString portName;
        Task *task;

        QueuedTask() : deviceId(""), portName(""), task(nullptr) {}
        QueuedTask(const QString &devId, const QString &pName, Task *t)
            : deviceId(devId), portName(pName), task(t) {}
    };

    struct PortQueue {
        QString portName;
        QThread *thread = nullptr;
        TaskExecutionWorker *worker = nullptr;
        QQueue<QueuedTask> highPriorityQueue;
        QQueue<QueuedTask> pollingQueue;
        bool workerBusy = false;
        bool dispatchPending = false;
    };

    void setupPortWorker(PortQueue *pq);
    void teardownPortWorker(PortQueue *pq);
    void requestDispatch(PortQueue *pq);
    void processNextTask(PortQueue *pq);
    bool executeTask(PortQueue *pq, const QueuedTask &queuedTask);
    void updateQueueStatus();

    bool m_isRunning;
    bool m_isPaused;

    QTimer *m_schedulerTimer;

    QHash<QString, PortQueue*> m_portQueues;

    mutable QMutex m_queueMutex;

    QMap<QString, QueuedTask> m_taskMap;
};

#endif // TASKQUEUEMANAGER_H
