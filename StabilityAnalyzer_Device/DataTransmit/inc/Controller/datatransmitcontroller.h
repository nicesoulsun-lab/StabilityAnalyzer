#ifndef DATATRANSMITCONTROLLER_H
#define DATATRANSMITCONTROLLER_H

#include "datatransmit_global.h"

#include <QObject>
#include <QVariantList>
#include <QVariantMap>

class ControlChannelServer;
class StatusChannelServer;
class StreamChannelServer;
class RndisManager;
class QTimer;
class QJsonObject;

/*
 * 文件功能：
 * DataTransmitController 是 Device 侧通信模块的统一入口。
 * 它负责执行 RNDIS 启动流程、启动三路 TCP 服务、处理基础握手命令，
 * 并把通信状态统一暴露给 Application / Sub 的控制层。
 */
class DATATRANSMIT_EXPORT DataTransmitController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(ConnectionState connectionState READ connectionState NOTIFY connectionStateChanged)
    Q_PROPERTY(QString connectionStateText READ connectionStateText NOTIFY connectionStateChanged)
    Q_PROPERTY(bool rndisReady READ isRndisReady NOTIFY rndisReadyChanged)
    Q_PROPERTY(bool controlListening READ isControlListening NOTIFY channelsStateChanged)
    Q_PROPERTY(bool statusListening READ isStatusListening NOTIFY channelsStateChanged)
    Q_PROPERTY(bool streamListening READ isStreamListening NOTIFY channelsStateChanged)
    Q_PROPERTY(bool controlClientConnected READ isControlClientConnected NOTIFY channelsStateChanged)
    Q_PROPERTY(bool statusClientConnected READ isStatusClientConnected NOTIFY channelsStateChanged)
    Q_PROPERTY(bool streamClientConnected READ isStreamClientConnected NOTIFY channelsStateChanged)
    Q_PROPERTY(QString scriptPath READ scriptPath WRITE setScriptPath NOTIFY scriptPathChanged)
    Q_PROPERTY(QString networkInterface READ networkInterface WRITE setNetworkInterface NOTIFY networkInterfaceChanged)
    Q_PROPERTY(QString deviceIp READ deviceIp WRITE setDeviceIp NOTIFY deviceIpChanged)
    Q_PROPERTY(QVariantList experimentChannels READ experimentChannels NOTIFY experimentChannelsChanged)
    Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged)
    Q_PROPERTY(bool autoRestartEnabled READ autoRestartEnabled WRITE setAutoRestartEnabled NOTIFY autoRestartEnabledChanged)

public:
    /*
     * 枚举功能：
     * 描述 Device 侧通信底座当前处于哪个阶段。
     */
    enum ConnectionState {
        Init,
        StartingRndis,
        WaitingNetworkReady,
        StartingControlServer,
        StartingStatusServer,
        StartingStreamServer,
        Online,
        Restarting
    };
    Q_ENUM(ConnectionState)

    /* 函数功能：创建 Device 侧通信控制器，并初始化各个通道对象。 */
    explicit DataTransmitController(QObject *parent = nullptr);
    ~DataTransmitController() override;

    ConnectionState connectionState() const;
    QString connectionStateText() const;

    bool isRndisReady() const;
    bool isControlListening() const;
    bool isStatusListening() const;
    bool isStreamListening() const;
    bool isControlClientConnected() const;
    bool isStatusClientConnected() const;
    bool isStreamClientConnected() const;

    QString scriptPath() const;
    void setScriptPath(const QString &scriptPath);

    QString networkInterface() const;
    void setNetworkInterface(const QString &interfaceName);

    QString deviceIp() const;
    void setDeviceIp(const QString &deviceIp);
    QVariantList experimentChannels() const;

    QString lastError() const;

    bool autoRestartEnabled() const;
    void setAutoRestartEnabled(bool enabled);

    /* 函数功能：启动 Device 侧通信服务。 */
    Q_INVOKABLE void startConnection();

    /* 函数功能：停止 Device 侧通信服务。 */
    Q_INVOKABLE void stopConnection();

    /* 函数功能：立即触发一次重新启动流程。 */
    Q_INVOKABLE void restartNow();

    /* 函数功能：向状态通道推送一条自定义状态消息。 */
    Q_INVOKABLE bool sendStatusMessage(const QVariantMap &payload);

    /* 函数功能：向实时数据通道推送一条自定义实时数据消息。 */
    Q_INVOKABLE bool sendStreamMessage(const QVariantMap &payload);

public slots:
    void updateExperimentChannelStatus(int channel, const QVariantMap &status);
    void sendCommandResult(const QString &command,
                           const QString &requestId,
                           bool success,
                           const QString &message = QString(),
                           const QVariantMap &extraPayload = QVariantMap());

signals:
    void connectionStateChanged();
    void rndisReadyChanged();
    void channelsStateChanged();
    void scriptPathChanged();
    void networkInterfaceChanged();
    void deviceIpChanged();
    void experimentChannelsChanged();
    void lastErrorChanged();
    void autoRestartEnabledChanged();
    void logMessage(const QString &message);
    void controlMessageReceived(const QVariantMap &message);
    void statusMessageReceived(const QVariantMap &message);
    void streamMessageReceived(const QVariantMap &message);
    void startExperimentRequested(int channel, int creatorId, const QVariantMap &params, const QString &requestId);
    void stopExperimentRequested(int channel, const QString &requestId);
    // 导入链路：PC 先拉取设备已有实验列表，再按实验 ID 拉取实验详情和原始数据。
    void listImportExperimentsRequested(const QString &requestId);
    void exportExperimentRequested(int experimentId, const QString &requestId);
    void exportExperimentScanRequested(int experimentId, int scanId, int offset, int limit, const QString &requestId);
    // 导入完成后回写设备端状态，避免同一条记录被重复导入。
    void markExperimentImportedRequested(int experimentId, int status, const QString &requestId);

private slots:
    /* 函数功能：执行启动状态机下一步。 */
    void performStartupStep();

    /* 函数功能：定时推送基础心跳和通道状态。 */
    void sendHeartbeat();

    /* 函数功能：处理控制通道收到的基础握手命令。 */
    void handleControlMessage(const QJsonObject &message);

    /* 函数功能：任一通道的连接状态变化后，统一刷新上层属性。 */
    void handleChannelStateChanged();

    /* 函数功能：通道或 RNDIS 报错后，统一进入重启流程。 */
    void handleTransportError(const QString &message);

private:
    /* 函数功能：设置当前状态并向上层发出通知。 */
    void setConnectionState(ConnectionState state);

    /* 函数功能：设置错误文本并向上层发出通知。 */
    void setLastError(const QString &errorText);

    /* 函数功能：安排稍后执行下一次重启。 */
    void scheduleRestart(int delayMs);

    /* 函数功能：停止全部通道监听，但不清理配置。 */
    void stopServers();

    /* 函数功能：组装一条标准状态快照，用于状态通道心跳和握手响应。 */
    QJsonObject buildStatusSnapshot() const;
    bool pushStatusSnapshot(const QString &messageType);

    /* 函数功能：组装一条标准设备信息响应。 */
    QJsonObject buildDeviceInfo() const;
    QJsonObject buildCommandResult(const QString &command,
                                   const QString &requestId,
                                   bool success,
                                   const QString &message = QString()) const;

    /* 属性功能：当前通信状态机状态。 */
    ConnectionState m_connectionState = Init;

    /* 属性功能：是否允许在故障后自动重新启动通信服务。 */
    bool m_autoRestartEnabled = true;

    /* 属性功能：最近一次错误文本。 */
    QString m_lastError;
    QVariantList m_experimentChannels;

    /* 属性功能：RNDIS 启动和网卡状态管理对象。 */
    RndisManager *m_rndisManager = nullptr;

    /* 属性功能：9000 控制通道服务端。 */
    ControlChannelServer *m_controlServer = nullptr;

    /* 属性功能：9001 状态通道服务端。 */
    StatusChannelServer *m_statusServer = nullptr;

    /* 属性功能：9002 实时数据通道服务端。 */
    StreamChannelServer *m_streamServer = nullptr;

    /* 属性功能：启动状态机驱动定时器。 */
    QTimer *m_startupTimer = nullptr;

    /* 属性功能：周期性状态心跳发送定时器。 */
    QTimer *m_heartbeatTimer = nullptr;

    /* 属性功能：记录当前控制器是否处于主动停止状态。 */
    bool m_manuallyStopped = true;
};

#endif // DATATRANSMITCONTROLLER_H
