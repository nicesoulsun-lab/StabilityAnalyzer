#ifndef TASKEXECUTIONWORKER_H
#define TASKEXECUTIONWORKER_H

#include <QObject>
#include <QThread>
#include <QMutex>
#include <QWaitCondition>
#include "taskscheduler_global.h"
#include "task.h"

class Task;

/**
 * @brief 任务执行工作类
 * 
 * 负责在子线程中执行任务，避免阻塞UI线程
 */
class TASKSCHEDULER_EXPORT TaskExecutionWorker : public QObject
{
    Q_OBJECT

public:
    explicit TaskExecutionWorker(QObject *parent = nullptr);
    ~TaskExecutionWorker();

public slots:
    /**
     * @brief 执行任务
     */
    void executeTask(Task *task);
//    QVector<quint16> executeTask(Task *task);

    /**
     * @brief 停止工作线程
     */
    void stop();

signals:
    /**
     * @brief 任务执行完成信号
     */
    void taskCompleted(TaskResult res, QVector<quint16> data);
    
    /**
     * @brief 任务执行错误信号
     */
    void taskError(const QString &error);

private slots:
    /**
     * @brief 处理任务完成信号
     */
    void onTaskCompleted(TaskResult res, QVector<quint16> data);

private:
    bool m_stopRequested;
};

#endif // TASKEXECUTIONWORKER_H
