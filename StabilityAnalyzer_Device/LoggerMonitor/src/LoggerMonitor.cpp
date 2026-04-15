#include "LoggerMonitor.h"
#include "LoggerMonitorWidget.h"
#include <QTextCursor>
#include <QTextBlock>
#include <QDebug>
#include <QThread>
#include "logmanager.h"

LoggerMonitor::LoggerMonitor(QObject *parent)
    : QObject{parent}
{
    qRegisterMetaType<QTextCursor>("QTextCursor");
    qRegisterMetaType<QTextBlock>("QTextBlock");
    QThread *thread = new QThread();
    this->moveToThread(thread);
    thread->start();
    connect(this, &LoggerMonitor::sig_init, this, [this](){
        m_timer = new QTimer;
        //qDebug() << "logger timer thread " << QThread::currentThread();
        connect(m_timer, &QTimer::timeout, [this](){
            QMutexLocker lock(&m_mutex);
            if(!m_loggerCache.isEmpty())
                emit sig_loggermonitor_appendMsg(m_loggerCache.takeLast());
            //qDebug() << "logger timer thread ++" << QThread::currentThread();

        });
    });
}


LoggerMonitor *LoggerMonitor::instance()
{
    static LoggerMonitor* l = nullptr;
    if(l == nullptr){
        l = new LoggerMonitor;
    }
    return l;
}

//初始化，添加信号连接什么的
void LoggerMonitor::init()
{

    emit sig_init();

    connect(this, &LoggerMonitor::sig_loggermonitor_show, [](){
        LoggerMonitorWidget::monitorWidget()->showNormal();
    });
    connect(LoggerMonitorWidget::monitorWidget(), &LoggerMonitorWidget::sig_close,this, [this](){
        QMutexLocker lock(&m_mutex);
        m_timer->stop();
    }, Qt::QueuedConnection);
    connect(LoggerMonitorWidget::monitorWidget(), &LoggerMonitorWidget::sig_show, this, [this](){
        QMutexLocker lock(&m_mutex);
        //qDebug() << "timer start thread ++++" << QThread::currentThread();
        m_timer->start(20);
    }, Qt::QueuedConnection);
    connect(this, &LoggerMonitor::sig_loggermonitor_appendMsg, LoggerMonitorWidget::monitorWidget(), &LoggerMonitorWidget::slot_monitorWidget_appendMsg);
}

//添加日志
void LoggerMonitor::appendLogger(const QString &logger, const QString &module, const int subjectId, const QString &time, const LoggerMonitor_Type &type)
{
    QMutexLocker lock(&m_mutex);

    if(LoggerMonitorWidget::monitorWidget()->isHidden()){
        m_timer->stop();
        return;
    }
    //日志超过数量清空
    if(m_loggerCache.size() > 5000){
        m_loggerCache.clear();
        LOG_WARNING() << tr("监控数据长度超出限制, 自动清空!!!!!!!!!!!!!!!");
    }
    QString t = tr("消息");
    switch (type) {
    case LM_TYPE_INFO:
        break;
    case LM_TYPE_WARINING:
        t = tr("警告");
        break;
    case LM_TYPE_ERROR:
        t = tr("错误");
        break;
    default:
        break;
    }
    QString msg = tr("<span style=\"color: rgb(50, 80, 115);\">[%1</span> <span style=\"color: rgb(50, 113, 174)\">%2 %3]</span> "
                     "<span style=\"color: rgb(73, 148, 196)\">%4</span>")
            .arg(time)
            .arg(module)
            .arg(t)
            .arg(logger);
    m_loggerCache.append(msg);
    LOG_INFO()<<"当前日志数据为："<<m_loggerCache.size();
}

//展示出来日志监控的界面
void LoggerMonitor::showMonitor()
{
    emit sig_loggermonitor_show();
}
