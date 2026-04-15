#include "taskexecutionworker.h"
#include "task.h"
#include <QDebug>

TaskExecutionWorker::TaskExecutionWorker(QObject *parent)
    : QObject(parent)
    , m_stopRequested(false)
{
    qRegisterMetaType<TaskResult>("TaskResult");
    qRegisterMetaType<QVector<quint16>>("QVector<quint16>");
    qRegisterMetaType<Task*>("Task*");

    qDebug() << "TaskExecutionWorker created in thread:" << QThread::currentThread();
}

TaskExecutionWorker::~TaskExecutionWorker()
{
    stop();
    qDebug() << "TaskExecutionWorker destroyed";
}

void TaskExecutionWorker::executeTask(Task *task)
{
    if (!task) {
        qWarning() << "Cannot execute null task";
        return;
    }
    
//    qDebug() << "Executing task in worker thread:" << task->taskName()
//             << "Device:" << task->deviceId()
//             << "Task ID:" << task->taskId()
//             << "Thread:" << QThread::currentThread()
//             << "TaskObjectThread:" << task->thread();
    
    // 断开之前的连接（如果有），然后重新连接，确保每个任务有独立的连接
    disconnect(task, &Task::taskCompleted, this, &TaskExecutionWorker::onTaskCompleted);
    
    // 连接任务完成信号，使用DirectConnection确保数据正确传递
    connect(task, &Task::taskCompleted, 
            this, &TaskExecutionWorker::onTaskCompleted, 
            Qt::DirectConnection);
    
    // 执行任务
    const QVector<quint16> result = task->execute();
    Q_UNUSED(result);
}

void TaskExecutionWorker::stop()
{
    m_stopRequested = true;
}

void TaskExecutionWorker::onTaskCompleted(TaskResult res, QVector<quint16> data)
{
    // 断开信号连接
    Task *task = qobject_cast<Task*>(sender());
    if (task) {
        disconnect(task, &Task::taskCompleted, 
                   this, &TaskExecutionWorker::onTaskCompleted);
    }
    
    // 发出任务完成信号
    emit taskCompleted(res, data);
}
