
#include <QDebug>
#include <QFile>
#include <QSettings>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QGuiApplication>
#include <QFileInfo>
#include <QJSEngine>
#include <QJSValue>
#include <QProcess>
#include <QDir>
#include <QTimer>
#include "inc/Common/systemdata.h"

Q_GLOBAL_STATIC(SystemData, global_systemData);

SystemData::SystemData(QObject *parent): QObject(parent) {


}

SystemData::~SystemData()
{
    this->SaveSystemConfig(m_systemSetPath);
}

SystemData *SystemData::globalSystemData()
{
    return global_systemData;
}

bool SystemData::LoadSystemConfig(const QString &filePath)
{
    qDebug()<<filePath;

    QMutexLocker locker(&m_mutex);

    // 检查文件路径是否为空
    if (filePath.isEmpty()) {
        qWarning() << "SystemData::LoadSystemConfig: File path is empty";
        return false;
    }

    // 检查文件是否存在
    if (!QFile::exists(filePath)) {
        qWarning() << "SystemData::LoadSystemConfig: File does not exist:" << filePath;
        // 可以在这里创建默认配置文件
        return false;
    }

    // 检查文件是否可读
    QFileInfo fileInfo(filePath);
    if (!fileInfo.isReadable()) {
        qCritical() << "SystemData::LoadSystemConfig: File is not readable:" << filePath;
        return false;
    }

    m_systemSetPath = filePath;
    QSettings settings(filePath, QSettings::IniFormat);

    // 系统设置信息配置
    settings.beginGroup("SystemSetting");
    m_systemConfig.language = settings.value("language", "zh_CN").toString();
    m_systemConfig.brightness = settings.value("brightness", 75).toInt();
    m_systemConfig.version = settings.value("version").toString();
    settings.endGroup();

    return true;
}

// 保存系统配置到文件
bool SystemData::SaveSystemConfig(const QString& filePath) {
    QMutexLocker locker(&m_mutex);

    QSettings settings(filePath, QSettings::IniFormat);

    // 系统设置配置
    settings.beginGroup("SystemSetting");
    settings.setValue("language", m_systemConfig.language);
    settings.setValue("brightness", m_systemConfig.brightness);
    settings.setValue("version", m_systemConfig.version);
    settings.endGroup();

    settings.sync();
    return settings.status() == QSettings::NoError;
}

// 更新系统配置
void SystemData::UpdateSystemConfig(const SystemSetting& config) {
    QMutexLocker locker(&m_mutex);
    m_systemConfig = config;
}

QString SystemData::currentLanguage()
{
    return m_systemConfig.language;
}

void SystemData::UpdateSystemSetting_language(QString language)
{
    QMutexLocker locker(&m_mutex);

    m_systemConfig.language = language;

    this->UpdateSysSettingConfigRow("language",language);
}

void SystemData::UpdateSystemSetting_brightness(int bright)
{
    QMutexLocker locker(&m_mutex);
    m_systemConfig.brightness = bright;

    this->UpdateSysSettingConfigRow("brightness",bright);
}

void SystemData::UpdateSystemSetting_version(QString version)
{
    QMutexLocker locker(&m_mutex);
    if(version.isEmpty()) return;

    m_systemConfig.version = version;

    this->UpdateSysSettingConfigRow("version",version);
}

void SystemData::UpdateSysSettingConfigRow(const QString &paraname, const QVariant &paravalue)
{
    // 修改配置文件
    QSettings settings(m_systemSetPath, QSettings::IniFormat);
    settings.beginGroup("SystemSetting");
    settings.setValue(paraname, paravalue);
    settings.endGroup();
    settings.sync();
}

bool SystemData::RestartApplication()
{
    QString appPath = QCoreApplication::applicationFilePath();
    QStringList args = QCoreApplication::arguments();
    args.removeFirst(); // remove app path

    qDebug() << "Restarting app:" << appPath << "with args:" << args;

    // 在退出当前实例前确保所有资源被正确释放
    // 可以添加一些清理操作
    qApp->processEvents(); // 处理剩余的事件

    if (!QProcess::startDetached(appPath, args, QDir::currentPath())) {
        qCritical() << "Failed to start detached process!";
        return false;
    }

    qDebug() << "Restart initiated. Quitting current instance...";

    // 延迟退出，确保新进程已经启动
    QTimer::singleShot(100, []() {
        QCoreApplication::quit();
    });

    return true;
}

QString SystemData::getCurrentTimeInTimezone()
{
    QDateTime utcTime = QDateTime::currentDateTimeUtc();

    // 手动添加8小时偏移（东八区）
    QDateTime beijingTime = utcTime.addSecs(8 * 3600);

    // 格式化输出
    return beijingTime.toString("yyyy-MM-dd hh:mm:ss");
}
