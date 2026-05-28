#include "rndismanager.h"

#include <QProcess>

RndisManager::RndisManager(QObject *parent)
    : QObject(parent)
    , m_interfaceName(QStringLiteral("usb0"))
    , m_deviceIp(QStringLiteral("192.168.0.2"))
{
}

bool RndisManager::initialize()
{
    emit logMessage(QStringLiteral("RNDIS已配置为系统服务，检查网络接口 %1 是否就绪").arg(m_interfaceName));
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
