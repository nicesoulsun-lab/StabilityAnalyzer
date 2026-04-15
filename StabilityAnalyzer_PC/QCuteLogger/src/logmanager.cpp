#include "logmanager.h"
#include <ConsoleAppender.h>
#include <RollingFileAppender.h>

LogManager::LogManager()
{
    m_format = "%{time}{yy-MM-dd,HH:mm:ss.zzz}[%{type:-7}][%{file:-25} %{line}] %{message}\n";
}

//初始化控制台输出
void LogManager::initConsoleAppender(){
    m_consoleAppender = new ConsoleAppender;
    m_consoleAppender->setFormat(m_format);
    cuteLogger->registerAppender(m_consoleAppender);
}

//初始化文件输出
void LogManager::initRollingFileAppender(){
    m_rollingFileAppender = new RollingFileAppender(m_logPath);
    m_rollingFileAppender->setFormat(m_format);
    m_rollingFileAppender->setLogFilesLimit(5); //限制日志文件的数量
    m_rollingFileAppender->setDatePattern(RollingFileAppender::DailyRollover); //日志滚动策略，这个是按照天滚动
    cuteLogger->registerAppender(m_rollingFileAppender);
}

void LogManager:: InitLog(const QString &filePath, const QString &fileName, bool isDebug){
    setLogFilePath(filePath, fileName);
    if(isDebug)
    {
        this->initConsoleAppender();
        this->initRollingFileAppender();
    }
    else
    {
        this->initConsoleAppender();
    }
}

QString LogManager::getLogFilePath(){
    return m_logPath;
}

void LogManager::setLogFilePath(const QString &filePath, const QString &fileName)
{
    QDir tempDir;
    if (!tempDir.exists(filePath)){
        tempDir.mkpath(filePath);
    }
    m_logPath = filePath + QString("/%1.log").arg(fileName);
    //qDebug()<<"log file -- "<<m_logPath;
}

LogManager::~LogManager()
{
}
