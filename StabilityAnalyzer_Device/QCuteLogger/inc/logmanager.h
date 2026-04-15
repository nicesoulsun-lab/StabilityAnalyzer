#ifndef LOGMANAGER_H
#define LOGMANAGER_H

#include <Logger.h>
#include <QtCore>

class ConsoleAppender;
class RollingFileAppender;
#include "CuteLogger_global.h"

// 导出
class CUTELOGGERSHARED_EXPORT LogManager
{
public:
    explicit LogManager();
    ~LogManager();
    void  InitLog(const QString &filePath = QCoreApplication::applicationDirPath() + "/log", const QString &fileName=qApp->applicationName(), bool isDebug = true);
    QString getLogFilePath();

private:
    QString m_format;
    QString m_logPath;
    ConsoleAppender* m_consoleAppender; //控制台控制
    RollingFileAppender* m_rollingFileAppender; //文件滚动控制

private:
    LogManager(const LogManager &);
    LogManager & operator = (const LogManager &);
    void initConsoleAppender();
    void initRollingFileAppender();
    void setLogFilePath(const QString &filePath, const QString &fileName);
};

#endif // LOGMANAGER_H
