#include "rndismanager.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QProcess>

RndisManager::RndisManager(QObject *parent)
    : QObject(parent)
    , m_scriptPath(buildDefaultScriptPath())
    , m_interfaceName(QStringLiteral("usb0"))
    , m_deviceIp(QStringLiteral("192.168.0.2"))
{
}

bool RndisManager::initialize()
{
    if (m_scriptPath.isEmpty() || !QFileInfo::exists(m_scriptPath)) {
        setLastError(QStringLiteral("RNDIS script not found: %1").arg(m_scriptPath));
        setReady(false);
        return false;
    }

    emit logMessage(QStringLiteral("Executing RNDIS script: %1").arg(m_scriptPath));

    QProcess process;
    process.start(QStringLiteral("sh"), QStringList() << m_scriptPath);
    if (!process.waitForStarted(3000)) {
        setLastError(QStringLiteral("Failed to start RNDIS script process"));
        setReady(false);
        return false;
    }

    if (!process.waitForFinished(30000)) {
        process.kill();
        process.waitForFinished(3000);
        setLastError(QStringLiteral("RNDIS script execution timed out"));
        setReady(false);
        return false;
    }

    const QString stdOutput = QString::fromUtf8(process.readAllStandardOutput()).trimmed();
    const QString stdError = QString::fromUtf8(process.readAllStandardError()).trimmed();
    if (!stdOutput.isEmpty()) {
        emit logMessage(stdOutput);
    }
    if (!stdError.isEmpty()) {
        emit logMessage(stdError);
    }

    if (process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0) {
        setLastError(QStringLiteral("RNDIS script exited with code %1").arg(process.exitCode()));
        setReady(false);
        return false;
    }

    return refreshNetworkState();
}

bool RndisManager::refreshNetworkState()
{
    QProcess process;
    process.start(QStringLiteral("ip"), QStringList() << QStringLiteral("-4") << QStringLiteral("addr")
                                                      << QStringLiteral("show") << QStringLiteral("dev")
                                                      << m_interfaceName);
    if (!process.waitForStarted(3000)) {
        setLastError(QStringLiteral("Failed to start ip command"));
        setReady(false);
        return false;
    }

    if (!process.waitForFinished(5000)) {
        process.kill();
        process.waitForFinished(1000);
        setLastError(QStringLiteral("ip command timed out"));
        setReady(false);
        return false;
    }

    const QString output = QString::fromUtf8(process.readAllStandardOutput());
    const QString errorOutput = QString::fromUtf8(process.readAllStandardError()).trimmed();
    const bool ready = (process.exitCode() == 0) && output.contains(m_deviceIp);

    if (!ready) {
        const QString errorText = errorOutput.isEmpty()
                ? QStringLiteral("Interface %1 does not have IP %2").arg(m_interfaceName, m_deviceIp)
                : errorOutput;
        setLastError(errorText);
        setReady(false);
        return false;
    }

    setLastError(QString());
    setReady(true);
    emit logMessage(QStringLiteral("RNDIS ready on %1 with IP %2").arg(m_interfaceName, m_deviceIp));
    return true;
}

QString RndisManager::scriptPath() const
{
    return m_scriptPath;
}

void RndisManager::setScriptPath(const QString &scriptPath)
{
    if (m_scriptPath == scriptPath) {
        return;
    }

    m_scriptPath = scriptPath;
    emit scriptPathChanged();
}

QString RndisManager::interfaceName() const
{
    return m_interfaceName;
}

void RndisManager::setInterfaceName(const QString &interfaceName)
{
    if (m_interfaceName == interfaceName) {
        return;
    }

    m_interfaceName = interfaceName;
    emit interfaceNameChanged();
}

QString RndisManager::deviceIp() const
{
    return m_deviceIp;
}

void RndisManager::setDeviceIp(const QString &deviceIp)
{
    if (m_deviceIp == deviceIp) {
        return;
    }

    m_deviceIp = deviceIp;
    emit deviceIpChanged();
}

bool RndisManager::isReady() const
{
    return m_ready;
}

QString RndisManager::lastError() const
{
    return m_lastError;
}

QString RndisManager::buildDefaultScriptPath() const
{
    const QDir appDir(QCoreApplication::applicationDirPath());
    const QString runtimePath = appDir.filePath(QStringLiteral("sh/rndis_start.sh"));
    if (QFileInfo::exists(runtimePath)) {
        return runtimePath;
    }

    const QString sourceLikePath = appDir.filePath(QStringLiteral("../sh/rndis_start.sh"));
    if (QFileInfo::exists(sourceLikePath)) {
        return QDir::cleanPath(sourceLikePath);
    }

    return runtimePath;
}

void RndisManager::setLastError(const QString &errorText)
{
    if (m_lastError == errorText) {
        return;
    }

    m_lastError = errorText;
    emit lastErrorChanged();
}

void RndisManager::setReady(bool ready)
{
    if (m_ready == ready) {
        return;
    }

    m_ready = ready;
    emit readyChanged();
}
