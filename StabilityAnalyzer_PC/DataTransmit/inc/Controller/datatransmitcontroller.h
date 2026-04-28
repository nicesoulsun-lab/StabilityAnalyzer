#ifndef DATATRANSMITCONTROLLER_H
#define DATATRANSMITCONTROLLER_H

#include <QObject>
#include <QVariantList>
#include <QVariantMap>
#include "datatransmit_global.h"

class QTimer;
class QProcess;
class TcpChannelClient;
class ControlChannelClient;
class StatusChannelClient;
class StreamChannelClient;

/*
 * 文件功能：
 * DataTransmitController 是 PC 侧 RNDIS + TCP 通信总控。
 * 它负责设备发现、网卡配置、三路通道建链、重连，以及向上层暴露统一状态和消息入口。
 */
class DATATRANSMIT_EXPORT DataTransmitController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(ConnectionState connectionState READ connectionState NOTIFY connectionStateChanged)
    Q_PROPERTY(QString connectionStateText READ connectionStateText NOTIFY connectionStateChanged)
    Q_PROPERTY(QString adapterName READ adapterName NOTIFY adapterNameChanged)
    Q_PROPERTY(QString adapterStatus READ adapterStatus NOTIFY adapterStatusChanged)
    Q_PROPERTY(QString deviceStatus READ deviceStatus NOTIFY deviceStatusChanged)
    Q_PROPERTY(bool controlConnected READ controlConnected NOTIFY channelStatusChanged)
    Q_PROPERTY(bool statusConnected READ statusConnected NOTIFY channelStatusChanged)
    Q_PROPERTY(bool streamConnected READ streamConnected NOTIFY channelStatusChanged)
    Q_PROPERTY(DeviceUiConnectionState deviceUiConnectionState READ deviceUiConnectionState NOTIFY deviceUiConnectionStateChanged)
    Q_PROPERTY(QString deviceUiConnectionStateText READ deviceUiConnectionStateText NOTIFY deviceUiConnectionStateChanged)
    Q_PROPERTY(QString deviceConnectionStateText READ deviceConnectionStateText NOTIFY deviceChannelInfoChanged)
    Q_PROPERTY(QString deviceNetworkInterface READ deviceNetworkInterface NOTIFY deviceChannelInfoChanged)
    Q_PROPERTY(QString deviceIpAddress READ deviceIpAddress NOTIFY deviceChannelInfoChanged)
    Q_PROPERTY(bool rndisReady READ rndisReady NOTIFY deviceChannelInfoChanged)
    Q_PROPERTY(bool controlListening READ controlListening NOTIFY deviceChannelInfoChanged)
    Q_PROPERTY(bool statusListening READ statusListening NOTIFY deviceChannelInfoChanged)
    Q_PROPERTY(bool streamListening READ streamListening NOTIFY deviceChannelInfoChanged)
    Q_PROPERTY(bool deviceControlClientConnected READ deviceControlClientConnected NOTIFY deviceChannelInfoChanged)
    Q_PROPERTY(bool deviceStatusClientConnected READ deviceStatusClientConnected NOTIFY deviceChannelInfoChanged)
    Q_PROPERTY(bool deviceStreamClientConnected READ deviceStreamClientConnected NOTIFY deviceChannelInfoChanged)
    Q_PROPERTY(QString lastHeartbeatTimestamp READ lastHeartbeatTimestamp NOTIFY deviceChannelInfoChanged)
    Q_PROPERTY(QVariantList experimentChannels READ experimentChannels NOTIFY experimentChannelsChanged)
    Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged)
    Q_PROPERTY(bool autoReconnectEnabled READ autoReconnectEnabled WRITE setAutoReconnectEnabled NOTIFY autoReconnectEnabledChanged)

public:
    enum ConnectionState {
        INIT,
        WAIT_DEVICE,
        WAIT_ADAPTER,
        CONFIGURE_IP,
        WAIT_DEVICE_READY,
        CONNECT_CONTROL,
        CONNECT_STATUS,
        CONNECT_STREAM,
        ONLINE,
        RECONNECTING
    };
    Q_ENUM(ConnectionState)

    enum DeviceUiConnectionState {
        // 面向界面展示的连接状态，与后台重连状态机解耦。
        DeviceDisconnected,
        DeviceConnecting,
        DeviceConnected
    };
    Q_ENUM(DeviceUiConnectionState)

    explicit DataTransmitController(QObject *parent = nullptr);
    ~DataTransmitController() override;

    ConnectionState connectionState() const;
    QString connectionStateText() const;
    QString adapterName() const;
    QString adapterStatus() const;
    QString deviceStatus() const;
    bool controlConnected() const;
    bool statusConnected() const;
    bool streamConnected() const;
    DeviceUiConnectionState deviceUiConnectionState() const;
    QString deviceUiConnectionStateText() const;
    QString deviceConnectionStateText() const;
    QString deviceNetworkInterface() const;
    QString deviceIpAddress() const;
    bool rndisReady() const;
    bool controlListening() const;
    bool statusListening() const;
    bool streamListening() const;
    bool deviceControlClientConnected() const;
    bool deviceStatusClientConnected() const;
    bool deviceStreamClientConnected() const;
    QString lastHeartbeatTimestamp() const;
    QVariantList experimentChannels() const;
    QString lastError() const;
    bool autoReconnectEnabled() const;

    Q_INVOKABLE void startConnection();
    Q_INVOKABLE void stopConnection();
    Q_INVOKABLE void retryNow();
    Q_INVOKABLE bool sendControlCommand(const QString &command, const QVariantMap &payload = QVariantMap());
    Q_INVOKABLE bool sendStatusMessage(const QVariantMap &payload);
    Q_INVOKABLE bool sendStreamMessage(const QVariantMap &payload);

public slots:
    void setAutoReconnectEnabled(bool enabled);

signals:
    void connectionStateChanged();
    void adapterNameChanged();
    void adapterStatusChanged();
    void deviceStatusChanged();
    void channelStatusChanged();
    void deviceUiConnectionStateChanged();
    // 仅在设备真正不可用时触发，用于前端弹框；普通状态切换不使用它。
    void deviceUnavailablePromptRequested();
    void deviceChannelInfoChanged();
    void experimentChannelsChanged();
    void lastErrorChanged();
    void autoReconnectEnabledChanged();
    void rawMessageReceived(const QString &channel, const QVariantMap &message);
    void controlMessageReceived(const QVariantMap &message);
    void statusMessageReceived(const QVariantMap &message);
    void streamMessageReceived(const QVariantMap &message);
    void logMessage(const QString &message);

private:
    enum ProbeTask {
        NoProbeTask,
        DetectAdapterTask,
        ConfigureAdapterTask,
        PingDeviceTask
    };

    void ensureConnection();
    void connectChannel(TcpChannelClient *channel);
    bool sendJson(TcpChannelClient *channel, const QVariantMap &payload);
    void bindChannels();
    void handleChannelMessage(const QString &channelName, const QVariantMap &message);
    void handleChannelConnected(const QString &channelName);
    void handleChannelError(TcpChannelClient *channel, const QString &errorMessage);
    void handleChannelDisconnected();
    // 记录一次建链失败，达到阈值后 UI 显示未连接，但后台仍继续重连。
    void recordConnectFailure();
    // 成功连上全部通道后清空失败计数，为下一轮断开判断做准备。
    void resetConnectFailures();
    // 更新 UI 专用连接状态，并在合适时机触发断开提示信号。
    void updateDeviceUiConnectionState(DeviceUiConnectionState state);
    void setConnectionState(ConnectionState state);
    void setAdapterName(const QString &name);
    void setAdapterStatus(const QString &status);
    void setDeviceStatus(const QString &status);
    void setLastError(const QString &error);
    void updateDeviceChannelInfo(const QVariantMap &message);
    void updateExperimentChannels(const QVariantMap &message);
    void resetDeviceChannelInfo();
    void resetExperimentChannels();
    void scheduleRetry(int intervalMs);
    void startProbeTask(ProbeTask task,
                        const QString &program,
                        const QStringList &arguments,
                        const QString &errorContext);
    void finishProbeTask();
    void handleProbeFinished(int exitCode, int exitStatus);
    void handleProbeError(int processError);
    void handleAdapterLookupFinished(int exitCode, int exitStatus, const QString &stdOutput, const QString &stdError);
    void handleConfigureAdapterFinished(int exitCode, int exitStatus, const QString &stdOutput, const QString &stdError);
    void handlePingFinished(int exitCode, int exitStatus, const QString &stdOutput, const QString &stdError);
    bool ensureAdminPrivilege();
    QString findAdapterByVidPid() const;
    QString findRndisAdapter() const;
    bool isAdapterIpConfigured(const QString &adapterName) const;
    bool ensureAdapterConfigured();
    bool pingDevice() const;
    bool allChannelsConnected() const;
    static QString stateToText(ConnectionState state);

private:
    QTimer *m_retryTimer = nullptr;
    QProcess *m_probeProcess = nullptr;
    bool m_running = false;
    bool m_autoReconnectEnabled = true;
    ProbeTask m_probeTask = NoProbeTask;
    QString m_probeErrorContext;
    bool m_deviceReachabilityConfirmed = false;
    ConnectionState m_connectionState = INIT;
    // UI 展示状态：避免重连状态机一直显示“连接中”。
    DeviceUiConnectionState m_deviceUiConnectionState = DeviceDisconnected;
    // 连续建链失败计数，超过阈值后才把 UI 状态降为“未连接”。
    int m_connectFailureCount = 0;
    // 防止后台重连期间反复弹“设备断开”提示。
    bool m_deviceUnavailablePromptActive = false;
    QString m_adapterName;
    QString m_adapterStatus;
    QString m_deviceStatus;
    QString m_lastError;
    QString m_ipConfiguredAdapterName;
    QString m_deviceConnectionStateText;
    QString m_deviceNetworkInterface;
    QString m_deviceIpAddress;
    QString m_lastHeartbeatTimestamp;
    QVariantList m_experimentChannels;
    bool m_rndisReady = false;
    bool m_controlListening = false;
    bool m_statusListening = false;
    bool m_streamListening = false;
    bool m_deviceControlClientConnected = false;
    bool m_deviceStatusClientConnected = false;
    bool m_deviceStreamClientConnected = false;
    ControlChannelClient *m_controlChannel = nullptr;
    StatusChannelClient *m_statusChannel = nullptr;
    StreamChannelClient *m_streamChannel = nullptr;
};

#endif // DATATRANSMITCONTROLLER_H
