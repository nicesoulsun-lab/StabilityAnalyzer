#ifndef LOGGERMONITOR_H
#define LOGGERMONITOR_H

#include <QObject>
#include <QDateTime>
#include <QMutex>
#include <QMutexLocker>
#include <QList>
#include "LoggerMonitor_global.h"
#include <QTimer>
#include <QVector>

#define LM_MODULE_DataReceiver          "数据接收"
#define LM_MODULE_DataParser            "数据解析"
#define LM_MODULE_TimeSpaceSync         "时空同步"

enum LoggerMonitor_Type
{
    LM_TYPE_INFO,
    LM_TYPE_WARINING,
    LM_TYPE_ERROR
};


class LOGGERMONITOR_EXPORT LoggerMonitor : public QObject
{
    Q_OBJECT
private:
    explicit LoggerMonitor(QObject *parent = nullptr);
    LoggerMonitor(const LoggerMonitor& log);
    LoggerMonitor& operator=(const LoggerMonitor&);
public:
    static LoggerMonitor* instance();

    /// 初始化 在主线程中(需要创建UI窗口)
    void init();

    /**
     * @brief appendLogger 添加一条显示日志
     * @param logger 日志内容
     * @param subjectId 专业id
     * @param module 模块
     * @param time 时间
     * @param type 消息内容
     */
    void appendLogger(const QString& logger, const QString& module, const int subjectId = 0,
                      const QString& time = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz"),
                      const LoggerMonitor_Type& type = LM_TYPE_INFO);
    /// 显示界面
    void showMonitor();
signals:
    void sig_loggermonitor_show();
    void sig_loggermonitor_appendMsg(QString msg);
    void sig_init();
private:
    // 日志列表
    //QList<QString> m_loggerList;
    QMutex m_mutex;
    QVector<QString> m_loggerCache;
    QTimer* m_timer = nullptr;
};
#define LOGGERMONITOR LoggerMonitor::instance()
#endif // LOGGERMONITOR_H
