#ifndef RNDISMANAGER_H
#define RNDISMANAGER_H

#include "datatransmit_global.h"

#include <QObject>

/*
 * 文件功能：
 * RndisManager 负责 Device 侧 RNDIS 网卡就绪检查。
 * RNDIS 已由系统服务管理，此处仅做接口名和 IP 状态确认。
 */
class DATATRANSMIT_EXPORT RndisManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString interfaceName READ interfaceName WRITE setInterfaceName NOTIFY interfaceNameChanged)
    Q_PROPERTY(QString deviceIp READ deviceIp WRITE setDeviceIp NOTIFY deviceIpChanged)
    Q_PROPERTY(bool ready READ isReady NOTIFY readyChanged)
    Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged)

public:
    /* 函数功能：创建一个 RNDIS 管理器。 */
    explicit RndisManager(QObject *parent = nullptr);

    /*
     * 函数功能：
     * 检查 RNDIS 网卡是否就绪。
     * @return 状态检查是否成功。
     */
    Q_INVOKABLE bool initialize();

    /*
     * 函数功能：
     * 刷新当前网卡/IP 就绪状态。
     * @return 当前 RNDIS 网卡是否已达到可通信状态。
     */
    Q_INVOKABLE bool refreshNetworkState();

    QString interfaceName() const;
    void setInterfaceName(const QString &interfaceName);

    QString deviceIp() const;
    void setDeviceIp(const QString &deviceIp);

    bool isReady() const;
    QString lastError() const;

signals:
    void interfaceNameChanged();
    void deviceIpChanged();
    void readyChanged();
    void lastErrorChanged();
    void logMessage(const QString &message);

private:
    /* 函数功能：更新错误文本并发出通知。 */
    void setLastError(const QString &errorText);

    /* 函数功能：更新 ready 状态并发出通知。 */
    void setReady(bool ready);

    /* 属性功能：期望出现的 RNDIS 接口名，默认 usb0。 */
    QString m_interfaceName;

    /* 属性功能：期望配置到设备侧的固定 IP。 */
    QString m_deviceIp;

    /* 属性功能：当前 RNDIS 网络是否已就绪。 */
    bool m_ready = false;

    /* 属性功能：最近一次状态检查的错误文本。 */
    QString m_lastError;
};

#endif // RNDISMANAGER_H
