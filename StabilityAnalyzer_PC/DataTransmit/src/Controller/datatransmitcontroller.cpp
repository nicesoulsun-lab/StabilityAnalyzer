#include "Controller/datatransmitcontroller.h"

#include "Channel/controlchannelclient.h"
#include "Channel/statuschannelclient.h"
#include "Channel/streamchannelclient.h"

#include <QDebug>
#include <QJsonArray>
#include <QJsonObject>
#include <QNetworkAddressEntry>
#include <QNetworkInterface>
#include <QProcess>
#include <QRegularExpression>
#include <QTimer>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

namespace {

const char *kPcIp = "192.168.0.1";
const char *kDeviceIp = "192.168.0.2";
const char *kMask = "255.255.255.0";
const char *kDeviceVid = "1D6B";
const char *kDevicePid = "0104";
// UI 侧不在第一次失败时立刻显示未连接，给设备枚举和 TCP 监听留一点恢复时间。
const int kConnectFailureThreshold = 3;

QString normalize(const QString &text)
{
    return text.trimmed().toLower();
}

bool containsRndisKeyword(const QString &text)
{
    const QString value = normalize(text);
    return value.contains("rndis")
        || value.contains("remote ndis")
        || value.contains("usb ethernet")
        || value.contains("usb network")
        || value.contains("gadget")
        || value.contains("cdc ecm");
}

}

DataTransmitController::DataTransmitController(QObject *parent)
    : QObject(parent)
    , m_retryTimer(new QTimer(this))
    , m_controlChannel(new ControlChannelClient(this))
    , m_statusChannel(new StatusChannelClient(this))
    , m_streamChannel(new StreamChannelClient(this))
{
    m_retryTimer->setSingleShot(true);
    connect(m_retryTimer, &QTimer::timeout, this, [this]() { ensureConnection(); });
    bindChannels();

    for (int channel = 0; channel < 4; ++channel) {
        QVariantMap initialStatus;
        initialStatus.insert(QStringLiteral("channel"), channel);
        initialStatus.insert(QStringLiteral("running"), false);
        initialStatus.insert(QStringLiteral("hasSample"), false);
        initialStatus.insert(QStringLiteral("isCovered"), false);
        initialStatus.insert(QStringLiteral("remainingSeconds"), 0);
        m_experimentChannels.append(initialStatus);
    }
}

DataTransmitController::~DataTransmitController() = default;

DataTransmitController::ConnectionState DataTransmitController::connectionState() const
{
    return m_connectionState;
}

QString DataTransmitController::connectionStateText() const
{
    return stateToText(m_connectionState);
}

QString DataTransmitController::adapterName() const
{
    return m_adapterName;
}

QString DataTransmitController::adapterStatus() const
{
    return m_adapterStatus;
}

QString DataTransmitController::deviceStatus() const
{
    return m_deviceStatus;
}

bool DataTransmitController::controlConnected() const
{
    return m_controlChannel->isConnected();
}

bool DataTransmitController::statusConnected() const
{
    return m_statusChannel->isConnected();
}

bool DataTransmitController::streamConnected() const
{
    return m_streamChannel->isConnected();
}

DataTransmitController::DeviceUiConnectionState DataTransmitController::deviceUiConnectionState() const
{
    return m_deviceUiConnectionState;
}

QString DataTransmitController::deviceUiConnectionStateText() const
{
    switch (m_deviceUiConnectionState) {
    case DeviceConnected:
        return QStringLiteral("Connected");
    case DeviceConnecting:
        return QStringLiteral("Connecting");
    case DeviceDisconnected:
        return QStringLiteral("Disconnected");
    }

    return QStringLiteral("Disconnected");
}

QString DataTransmitController::deviceConnectionStateText() const
{
    return m_deviceConnectionStateText;
}

QString DataTransmitController::deviceNetworkInterface() const
{
    return m_deviceNetworkInterface;
}

QString DataTransmitController::deviceIpAddress() const
{
    return m_deviceIpAddress;
}

bool DataTransmitController::rndisReady() const
{
    return m_rndisReady;
}

bool DataTransmitController::controlListening() const
{
    return m_controlListening;
}

bool DataTransmitController::statusListening() const
{
    return m_statusListening;
}

bool DataTransmitController::streamListening() const
{
    return m_streamListening;
}

bool DataTransmitController::deviceControlClientConnected() const
{
    return m_deviceControlClientConnected;
}

bool DataTransmitController::deviceStatusClientConnected() const
{
    return m_deviceStatusClientConnected;
}

bool DataTransmitController::deviceStreamClientConnected() const
{
    return m_deviceStreamClientConnected;
}

QString DataTransmitController::lastHeartbeatTimestamp() const
{
    return m_lastHeartbeatTimestamp;
}

QVariantList DataTransmitController::experimentChannels() const
{
    return m_experimentChannels;
}

QString DataTransmitController::lastError() const
{
    return m_lastError;
}

bool DataTransmitController::autoReconnectEnabled() const
{
    return m_autoReconnectEnabled;
}

void DataTransmitController::startConnection()
{
    if (m_running) {
        emit logMessage(QStringLiteral("Communication manager is already running"));
        qDebug() << "[DataTransmit] startConnection ignored because manager is already running";
        return;
    }

    m_running = true;
    m_ipConfiguredAdapterName.clear();
    resetConnectFailures();
    updateDeviceUiConnectionState(DeviceConnecting);
    resetDeviceChannelInfo();
    resetExperimentChannels();
    setLastError(QString());
    setConnectionState(INIT);
    emit logMessage(QStringLiteral("Start establishing RNDIS TCP communication"));
    qDebug() << "[DataTransmit] startConnection, target device ip=" << kDeviceIp
             << "vid=" << kDeviceVid << "pid=" << kDevicePid;
    ensureConnection();
}

void DataTransmitController::stopConnection()
{
    qDebug() << "[DataTransmit] stopConnection";
    m_running = false;
    m_retryTimer->stop();
    m_controlChannel->disconnectFromHost();
    m_statusChannel->disconnectFromHost();
    m_streamChannel->disconnectFromHost();
    m_ipConfiguredAdapterName.clear();
    resetConnectFailures();
    updateDeviceUiConnectionState(DeviceDisconnected);
    resetDeviceChannelInfo();
    resetExperimentChannels();
    setConnectionState(INIT);
    setAdapterStatus(QStringLiteral("Stopped"));
    setDeviceStatus(QStringLiteral("Not checked"));
}

void DataTransmitController::retryNow()
{
    qDebug() << "[DataTransmit] retryNow";
    if (!m_running) {
        m_running = true;
    }
    m_retryTimer->stop();
    ensureConnection();
}

bool DataTransmitController::sendControlCommand(const QString &command, const QVariantMap &payload)
{
    if (command.trimmed().isEmpty()) {
        setLastError(QStringLiteral("Control command cannot be empty"));
        return false;
    }

    QVariantMap message = payload;
    message.insert(QStringLiteral("cmd"), command);
    return sendJson(m_controlChannel, message);
}

bool DataTransmitController::sendStatusMessage(const QVariantMap &payload)
{
    return sendJson(m_statusChannel, payload);
}

bool DataTransmitController::sendStreamMessage(const QVariantMap &payload)
{
    return sendJson(m_streamChannel, payload);
}

void DataTransmitController::setAutoReconnectEnabled(bool enabled)
{
    if (m_autoReconnectEnabled == enabled) {
        return;
    }

    m_autoReconnectEnabled = enabled;
    emit autoReconnectEnabledChanged();
}

void DataTransmitController::ensureConnection()
{
    if (!m_running) {
        qDebug() << "[DataTransmit] ensureConnection skipped because manager is not running";
        return;
    }

    qDebug() << "[DataTransmit] ensureConnection state=" << stateToText(m_connectionState)
             << "control=" << controlConnected()
             << "status=" << statusConnected()
             << "stream=" << streamConnected();

    if (!ensureAdminPrivilege()) {
        recordConnectFailure();
        setConnectionState(WAIT_DEVICE);
        qWarning() << "[DataTransmit] admin privilege check failed";
        scheduleRetry(2000);
        return;
    }

    QString adapter = findAdapterByVidPid();
    if (adapter.isEmpty()) {
        adapter = findRndisAdapter();
    }

    if (adapter.isEmpty()) {
        recordConnectFailure();
        setAdapterName(QString());
        setAdapterStatus(QStringLiteral("RNDIS adapter not found"));
        setDeviceStatus(QStringLiteral("Waiting for device"));
        setConnectionState(WAIT_ADAPTER);
        qDebug() << "[DataTransmit] RNDIS adapter not found yet";
        scheduleRetry(2000);
        return;
    }

    setAdapterName(adapter);
    setAdapterStatus(QStringLiteral("Adapter detected"));
    qDebug() << "[DataTransmit] detected adapter:" << adapter;

    setConnectionState(CONFIGURE_IP);
    if (!ensureAdapterConfigured()) {
        recordConnectFailure();
        qWarning() << "[DataTransmit] ensureAdapterConfigured failed, adapter=" << m_adapterName;
        scheduleRetry(2000);
        return;
    }

    setConnectionState(WAIT_DEVICE_READY);
    if (!pingDevice()) {
        recordConnectFailure();
        setDeviceStatus(QStringLiteral("Device not reachable"));
        qWarning() << "[DataTransmit] ping device failed, target=" << kDeviceIp;
        scheduleRetry(1000);
        return;
    }

    setDeviceStatus(QStringLiteral("Device online"));
    if (m_deviceUiConnectionState == DeviceDisconnected) {
        updateDeviceUiConnectionState(DeviceConnecting);
    }
    qDebug() << "[DataTransmit] ping device success, target=" << kDeviceIp;

    if (!controlConnected()) {
        setConnectionState(CONNECT_CONTROL);
        qDebug() << "[DataTransmit] connecting control channel";
        connectChannel(m_controlChannel);
        scheduleRetry(1000);
        return;
    }

    if (!statusConnected()) {
        setConnectionState(CONNECT_STATUS);
        qDebug() << "[DataTransmit] connecting status channel";
        connectChannel(m_statusChannel);
        scheduleRetry(1000);
        return;
    }

    if (!streamConnected()) {
        setConnectionState(CONNECT_STREAM);
        qDebug() << "[DataTransmit] connecting stream channel";
        connectChannel(m_streamChannel);
        scheduleRetry(1000);
        return;
    }

    if (allChannelsConnected()) {
        resetConnectFailures();
        updateDeviceUiConnectionState(DeviceConnected);
        setConnectionState(ONLINE);
        setLastError(QString());
        emit logMessage(QStringLiteral("All three TCP channels are connected"));
        qDebug() << "[DataTransmit] all tcp channels connected, communication online";
    }
}

void DataTransmitController::connectChannel(TcpChannelClient *channel)
{
    if (!channel) {
        return;
    }

    emit logMessage(QStringLiteral("Connecting %1 channel on port %2").arg(channel->name()).arg(channel->port()));
    qDebug() << "[DataTransmit] connectChannel" << channel->name() << "port=" << channel->port();
    channel->connectToHost(QString::fromLatin1(kDeviceIp));
}

bool DataTransmitController::sendJson(TcpChannelClient *channel, const QVariantMap &payload)
{
    if (!channel) {
        return false;
    }

    QString errorMessage;
    const bool success = channel->sendMessage(QJsonObject::fromVariantMap(payload), &errorMessage);
    if (!success) {
        setLastError(errorMessage);
        qWarning() << "[DataTransmit] sendJson failed, channel=" << channel->name() << "error=" << errorMessage;
    } else {
        qDebug() << "[DataTransmit] sendJson success, channel=" << channel->name() << "payload=" << payload;
    }
    return success;
}

void DataTransmitController::bindChannels()
{
    connect(m_controlChannel, &TcpChannelClient::connected, this, [this]() {
        handleChannelConnected(QStringLiteral("control"));
    });
    connect(m_statusChannel, &TcpChannelClient::connected, this, [this]() {
        handleChannelConnected(QStringLiteral("status"));
    });
    connect(m_streamChannel, &TcpChannelClient::connected, this, [this]() {
        handleChannelConnected(QStringLiteral("stream"));
    });

    connect(m_controlChannel, &TcpChannelClient::disconnected, this, [this]() { handleChannelDisconnected(); });
    connect(m_statusChannel, &TcpChannelClient::disconnected, this, [this]() { handleChannelDisconnected(); });
    connect(m_streamChannel, &TcpChannelClient::disconnected, this, [this]() { handleChannelDisconnected(); });

    connect(m_controlChannel, &TcpChannelClient::errorOccurred, this, [this](const QString &errorMessage) {
        handleChannelError(m_controlChannel, errorMessage);
    });
    connect(m_statusChannel, &TcpChannelClient::errorOccurred, this, [this](const QString &errorMessage) {
        handleChannelError(m_statusChannel, errorMessage);
    });
    connect(m_streamChannel, &TcpChannelClient::errorOccurred, this, [this](const QString &errorMessage) {
        handleChannelError(m_streamChannel, errorMessage);
    });

    connect(m_controlChannel, &TcpChannelClient::messageReceived, this, [this](const QVariantMap &message) {
        handleChannelMessage(QStringLiteral("control"), message);
    });
    connect(m_statusChannel, &TcpChannelClient::messageReceived, this, [this](const QVariantMap &message) {
        handleChannelMessage(QStringLiteral("status"), message);
    });
    connect(m_streamChannel, &TcpChannelClient::messageReceived, this, [this](const QVariantMap &message) {
        handleChannelMessage(QStringLiteral("stream"), message);
    });
}

void DataTransmitController::handleChannelMessage(const QString &channelName, const QVariantMap &message)
{
    qDebug() << "[DataTransmit]" << channelName << "message received:" << message;
    emit rawMessageReceived(channelName, message);

    if (channelName == QStringLiteral("control")) {
        emit controlMessageReceived(message);
    } else if (channelName == QStringLiteral("status")) {
        updateDeviceChannelInfo(message);
        updateExperimentChannels(message);
        emit statusMessageReceived(message);
    } else if (channelName == QStringLiteral("stream")) {
        emit streamMessageReceived(message);
    }
}

void DataTransmitController::handleChannelConnected(const QString &channelName)
{
    if (m_deviceUiConnectionState == DeviceDisconnected) {
        updateDeviceUiConnectionState(DeviceConnecting);
    }
    emit channelStatusChanged();
    emit logMessage(QStringLiteral("%1 channel connected").arg(channelName));
    qDebug() << "[DataTransmit]" << channelName << "tcp connected";

    if (channelName == QStringLiteral("control")) {
        const bool ok = sendControlCommand(QStringLiteral("ping"));
        qDebug() << "[DataTransmit] control handshake ping sent, ok=" << ok;
    }

    if (m_running) {
        ensureConnection();
    }
}

void DataTransmitController::handleChannelError(TcpChannelClient *channel, const QString &errorMessage)
{
    qWarning() << "[DataTransmit] channel error, channel="
               << (channel ? channel->name() : QStringLiteral("unknown"))
               << "error=" << errorMessage;
    setLastError(errorMessage);
    recordConnectFailure();
    emit channelStatusChanged();

    if (m_running && m_autoReconnectEnabled) {
        setConnectionState(RECONNECTING);
        scheduleRetry(1000);
    }
}

void DataTransmitController::handleChannelDisconnected()
{
    emit channelStatusChanged();
    qWarning() << "[DataTransmit] channel disconnected, control/status/stream ="
               << controlConnected() << statusConnected() << streamConnected();
    if (!statusConnected()) {
        resetDeviceChannelInfo();
        resetExperimentChannels();
    }
    if (!m_running) {
        return;
    }

    recordConnectFailure();
    setConnectionState(RECONNECTING);
    setDeviceStatus(QStringLiteral("Channel disconnected, retrying"));
    emit logMessage(QStringLiteral("Channel disconnected, entering reconnect state"));
    scheduleRetry(1000);
}

void DataTransmitController::recordConnectFailure()
{
    ++m_connectFailureCount;
    if (m_connectFailureCount >= kConnectFailureThreshold) {
        // 只影响界面展示；scheduleRetry 仍会继续驱动后台重连。
        updateDeviceUiConnectionState(DeviceDisconnected);
    } else if (m_deviceUiConnectionState != DeviceDisconnected) {
        updateDeviceUiConnectionState(DeviceConnecting);
    }
}

void DataTransmitController::resetConnectFailures()
{
    m_connectFailureCount = 0;
}

void DataTransmitController::updateDeviceUiConnectionState(DeviceUiConnectionState state)
{
    if (m_deviceUiConnectionState == state) {
        return;
    }

    const DeviceUiConnectionState previousState = m_deviceUiConnectionState;
    m_deviceUiConnectionState = state;
    emit deviceUiConnectionStateChanged();

    if (state == DeviceConnected) {
        // 重新连上后，允许下一次真正断开时再次提示用户。
        m_deviceUnavailablePromptActive = false;
        return;
    }

    if (state == DeviceDisconnected
            && previousState != DeviceDisconnected
            && !m_deviceUnavailablePromptActive) {
        // 专用弹框信号，避免 QML 监听通用状态变化导致“连接中 -> 已连接”误弹。
        m_deviceUnavailablePromptActive = true;
        emit deviceUnavailablePromptRequested();
    }
}

void DataTransmitController::setConnectionState(ConnectionState state)
{
    if (m_connectionState == state) {
        return;
    }

    qDebug() << "[DataTransmit] state change:" << stateToText(m_connectionState) << "->" << stateToText(state);
    m_connectionState = state;
    emit connectionStateChanged();
}

void DataTransmitController::setAdapterName(const QString &name)
{
    if (m_adapterName == name) {
        return;
    }

    m_ipConfiguredAdapterName.clear();
    m_adapterName = name;
    emit adapterNameChanged();
}

void DataTransmitController::setAdapterStatus(const QString &status)
{
    if (m_adapterStatus == status) {
        return;
    }

    m_adapterStatus = status;
    emit adapterStatusChanged();
}

void DataTransmitController::setDeviceStatus(const QString &status)
{
    if (m_deviceStatus == status) {
        return;
    }

    m_deviceStatus = status;
    emit deviceStatusChanged();
}

void DataTransmitController::setLastError(const QString &error)
{
    if (m_lastError == error) {
        return;
    }

    if (!error.isEmpty()) {
        qWarning() << "[DataTransmit] lastError =" << error;
    }
    m_lastError = error;
    emit lastErrorChanged();
}

void DataTransmitController::updateDeviceChannelInfo(const QVariantMap &message)
{
    bool changed = false;

    auto updateString = [&changed](QString &target, const QString &value) {
        if (target != value) {
            target = value;
            changed = true;
        }
    };

    auto updateBool = [&changed](bool &target, bool value) {
        if (target != value) {
            target = value;
            changed = true;
        }
    };

    updateString(m_deviceConnectionStateText, message.value(QStringLiteral("connection_state")).toString());
    updateString(m_deviceNetworkInterface, message.value(QStringLiteral("network_interface")).toString());
    updateString(m_deviceIpAddress, message.value(QStringLiteral("device_ip")).toString());
    updateString(m_lastHeartbeatTimestamp, message.value(QStringLiteral("timestamp")).toString());
    updateBool(m_rndisReady, message.value(QStringLiteral("rndis_ready")).toBool());
    updateBool(m_controlListening, message.value(QStringLiteral("control_listening")).toBool());
    updateBool(m_statusListening, message.value(QStringLiteral("status_listening")).toBool());
    updateBool(m_streamListening, message.value(QStringLiteral("stream_listening")).toBool());
    updateBool(m_deviceControlClientConnected, message.value(QStringLiteral("control_client_connected")).toBool());
    updateBool(m_deviceStatusClientConnected, message.value(QStringLiteral("status_client_connected")).toBool());
    updateBool(m_deviceStreamClientConnected, message.value(QStringLiteral("stream_client_connected")).toBool());

    if (changed) {
        emit deviceChannelInfoChanged();
    }
}

void DataTransmitController::resetDeviceChannelInfo()
{
    bool changed = false;

    auto resetString = [&changed](QString &target) {
        if (!target.isEmpty()) {
            target.clear();
            changed = true;
        }
    };

    auto resetBool = [&changed](bool &target) {
        if (target) {
            target = false;
            changed = true;
        }
    };

    resetString(m_deviceConnectionStateText);
    resetString(m_deviceNetworkInterface);
    resetString(m_deviceIpAddress);
    resetString(m_lastHeartbeatTimestamp);
    resetBool(m_rndisReady);
    resetBool(m_controlListening);
    resetBool(m_statusListening);
    resetBool(m_streamListening);
    resetBool(m_deviceControlClientConnected);
    resetBool(m_deviceStatusClientConnected);
    resetBool(m_deviceStreamClientConnected);

    if (changed) {
        emit deviceChannelInfoChanged();
    }
}

void DataTransmitController::updateExperimentChannels(const QVariantMap &message)
{
    const QVariantList incomingChannels = message.value(QStringLiteral("experiment_channels")).toList();
    if (incomingChannels.isEmpty()) {
        return;
    }

    QVariantList updatedChannels = m_experimentChannels;
    bool changed = false;

    for (const QVariant &channelItem : incomingChannels) {
        const QVariantMap channelMap = channelItem.toMap();
        const int channel = channelMap.value(QStringLiteral("channel"), -1).toInt();
        if (channel < 0 || channel >= updatedChannels.size()) {
            continue;
        }

        if (updatedChannels.at(channel).toMap() != channelMap) {
            updatedChannels[channel] = channelMap;
            changed = true;
        }
    }

    if (changed) {
        m_experimentChannels = updatedChannels;
        emit experimentChannelsChanged();
    }
}

void DataTransmitController::resetExperimentChannels()
{
    QVariantList defaults;
    for (int channel = 0; channel < 4; ++channel) {
        QVariantMap initialStatus;
        initialStatus.insert(QStringLiteral("channel"), channel);
        initialStatus.insert(QStringLiteral("running"), false);
        initialStatus.insert(QStringLiteral("hasSample"), false);
        initialStatus.insert(QStringLiteral("isCovered"), false);
        initialStatus.insert(QStringLiteral("remainingSeconds"), 0);
        defaults.append(initialStatus);
    }

    if (m_experimentChannels != defaults) {
        m_experimentChannels = defaults;
        emit experimentChannelsChanged();
    }
}

void DataTransmitController::scheduleRetry(int intervalMs)
{
    if (!m_running || !m_autoReconnectEnabled) {
        qDebug() << "[DataTransmit] scheduleRetry ignored, running=" << m_running
                 << "autoReconnect=" << m_autoReconnectEnabled;
        return;
    }

    if (!m_retryTimer->isActive()) {
        qDebug() << "[DataTransmit] scheduleRetry after" << intervalMs << "ms";
        m_retryTimer->start(intervalMs);
    } else {
        qDebug() << "[DataTransmit] retry timer already active";
    }
}

bool DataTransmitController::ensureAdminPrivilege()
{
#ifdef Q_OS_WIN
    SID_IDENTIFIER_AUTHORITY ntAuthority = SECURITY_NT_AUTHORITY;
    PSID adminGroup = nullptr;
    BOOL isAdmin = FALSE;
    const BOOL sidReady = AllocateAndInitializeSid(&ntAuthority,
                                                   2,
                                                   SECURITY_BUILTIN_DOMAIN_RID,
                                                   DOMAIN_ALIAS_RID_ADMINS,
                                                   0, 0, 0, 0, 0, 0,
                                                   &adminGroup);
    if (sidReady) {
        CheckTokenMembership(nullptr, adminGroup, &isAdmin);
        FreeSid(adminGroup);
    }

    if (!isAdmin) {
        setLastError(QStringLiteral("Administrator privilege is required to configure adapter IP"));
        setAdapterStatus(QStringLiteral("Missing administrator privilege"));
        emit logMessage(QStringLiteral("Current process is not elevated"));
        qWarning() << "[DataTransmit] current process is not elevated";
        return false;
    }
#endif
    qDebug() << "[DataTransmit] admin privilege check passed";
    return true;
}

QString DataTransmitController::findAdapterByVidPid() const
{
#ifdef Q_OS_WIN
    const QString script = QStringLiteral(
        "$usbVid='%1';"
        "$usbPid='%2';"
        "Get-CimInstance Win32_NetworkAdapter | "
        "Where-Object { $_.NetConnectionID -and $_.PNPDeviceID -and $_.PNPDeviceID.ToUpper().Contains(\"VID_\" + $usbVid) -and $_.PNPDeviceID.ToUpper().Contains(\"PID_\" + $usbPid) } | "
        "Select-Object -ExpandProperty NetConnectionID")
            .arg(QString::fromLatin1(kDeviceVid), QString::fromLatin1(kDevicePid));

    QProcess process;
    process.start(QStringLiteral("powershell"),
                  QStringList() << QStringLiteral("-NoProfile")
                                << QStringLiteral("-ExecutionPolicy") << QStringLiteral("Bypass")
                                << QStringLiteral("-Command") << script);
    if (!process.waitForFinished(5000)) {
        qWarning() << "[DataTransmit] powershell VID/PID lookup timeout";
        return QString();
    }

    const QString stdOutput = QString::fromLocal8Bit(process.readAllStandardOutput()).trimmed();
    const QString stdError = QString::fromLocal8Bit(process.readAllStandardError()).trimmed();
    if (!stdError.isEmpty()) {
        qWarning() << "[DataTransmit] powershell VID/PID lookup stderr =" << stdError;
    }

    if (process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0 || stdOutput.isEmpty()) {
        qDebug() << "[DataTransmit] no adapter matched VID/PID"
                 << QString("VID_%1 PID_%2").arg(kDeviceVid, kDevicePid);
        return QString();
    }

    const QStringList lines = stdOutput.split(QRegularExpression(QStringLiteral("[\r\n]+")),
                                              QString::SkipEmptyParts);
    if (!lines.isEmpty()) {
        qDebug() << "[DataTransmit] VID/PID matched adapter:" << lines.first();
        return lines.first().trimmed();
    }
#endif
    return QString();
}

QString DataTransmitController::findRndisAdapter() const
{
    QString fallbackAdapter;
    const QList<QNetworkInterface> interfaces = QNetworkInterface::allInterfaces();
    for (const QNetworkInterface &iface : interfaces) {
        if (!(iface.flags() & QNetworkInterface::IsUp) && !(iface.flags() & QNetworkInterface::IsRunning)) {
            continue;
        }

        const QString humanName = iface.humanReadableName();
        const QString name = iface.name();
        qDebug() << "[DataTransmit] inspect adapter" << humanName << "(" << name << ")";
        if (containsRndisKeyword(humanName) || containsRndisKeyword(name)) {
            qDebug() << "[DataTransmit] adapter keyword matched:" << humanName << name;
            return humanName.isEmpty() ? name : humanName;
        }

        for (const QNetworkAddressEntry &entry : iface.addressEntries()) {
            if (entry.ip().toString() == QString::fromLatin1(kPcIp)
                || entry.ip().toString().startsWith(QStringLiteral("192.168.0."))) {
                fallbackAdapter = humanName.isEmpty() ? name : humanName;
            }
        }
    }
    if (!fallbackAdapter.isEmpty()) {
        qDebug() << "[DataTransmit] use fallback adapter:" << fallbackAdapter;
    }
    return fallbackAdapter;
}

bool DataTransmitController::isAdapterIpConfigured(const QString &adapterName) const
{
    if (adapterName.trimmed().isEmpty()) {
        return false;
    }

    const QList<QNetworkInterface> interfaces = QNetworkInterface::allInterfaces();
    for (const QNetworkInterface &iface : interfaces) {
        const QString displayName = iface.humanReadableName().isEmpty() ? iface.name() : iface.humanReadableName();
        if (displayName != adapterName && iface.name() != adapterName) {
            continue;
        }

        for (const QNetworkAddressEntry &entry : iface.addressEntries()) {
            if (entry.ip().toString() == QString::fromLatin1(kPcIp)) {
                return true;
            }
        }
        return false;
    }

    return false;
}

bool DataTransmitController::ensureAdapterConfigured()
{
    if (isAdapterIpConfigured(m_adapterName)) {
        setAdapterStatus(QStringLiteral("IP configured as 192.168.0.1"));
        m_ipConfiguredAdapterName = m_adapterName;
        qDebug() << "[DataTransmit] adapter already configured with ip" << kPcIp;
        return true;
    }

    if (!m_ipConfiguredAdapterName.isEmpty() && m_ipConfiguredAdapterName == m_adapterName) {
        setAdapterStatus(QStringLiteral("IP configured as 192.168.0.1"));
        qDebug() << "[DataTransmit] skip netsh because adapter was configured earlier in this process:"
                 << m_adapterName;
        return true;
    }

#ifdef Q_OS_WIN
    QProcess process;
    process.start(QStringLiteral("netsh"),
                  QStringList()
                  << QStringLiteral("interface")
                  << QStringLiteral("ip")
                  << QStringLiteral("set")
                  << QStringLiteral("address")
                  << QStringLiteral("name=%1").arg(m_adapterName)
                  << QStringLiteral("static")
                  << QString::fromLatin1(kPcIp)
                  << QString::fromLatin1(kMask));
    qDebug() << "[DataTransmit] running netsh for adapter" << m_adapterName;
    process.waitForFinished(5000);

    if (process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0) {
        const QString stderrOutput = QString::fromLocal8Bit(process.readAllStandardError()).trimmed();
        setLastError(stderrOutput.isEmpty() ? QStringLiteral("Failed to configure adapter by netsh") : stderrOutput);
        setAdapterStatus(QStringLiteral("IP configure failed"));
        qWarning() << "[DataTransmit] netsh configure failed, stderr=" << stderrOutput;
        return false;
    }
#endif

    setAdapterStatus(QStringLiteral("IP configured as 192.168.0.1"));
    emit logMessage(QStringLiteral("Adapter IP configured as 192.168.0.1/24"));
    m_ipConfiguredAdapterName = m_adapterName;
    qDebug() << "[DataTransmit] adapter ip configured successfully";
    return true;
}

bool DataTransmitController::pingDevice() const
{
    QProcess process;
#ifdef Q_OS_WIN
    process.start(QStringLiteral("ping"), QStringList() << QStringLiteral("-n") << QStringLiteral("1")
                  << QStringLiteral("-w") << QStringLiteral("1000") << QString::fromLatin1(kDeviceIp));
#else
    process.start(QStringLiteral("ping"), QStringList() << QStringLiteral("-c") << QStringLiteral("1")
                  << QStringLiteral("-W") << QStringLiteral("1") << QString::fromLatin1(kDeviceIp));
#endif
    qDebug() << "[DataTransmit] ping device start:" << kDeviceIp;

    if (!process.waitForFinished(3000)) {
        qWarning() << "[DataTransmit] ping process timeout";
        return false;
    }

    const bool success = process.exitStatus() == QProcess::NormalExit && process.exitCode() == 0;
    qDebug() << "[DataTransmit] ping device result, exitCode=" << process.exitCode() << "success=" << success;
    return success;
}

bool DataTransmitController::allChannelsConnected() const
{
    return controlConnected() && statusConnected() && streamConnected();
}

QString DataTransmitController::stateToText(ConnectionState state)
{
    switch (state) {
    case INIT:
        return QStringLiteral("Init");
    case WAIT_DEVICE:
        return QStringLiteral("Wait Device");
    case WAIT_ADAPTER:
        return QStringLiteral("Wait Adapter");
    case CONFIGURE_IP:
        return QStringLiteral("Configure IP");
    case WAIT_DEVICE_READY:
        return QStringLiteral("Wait Device Ready");
    case CONNECT_CONTROL:
        return QStringLiteral("Connect Control");
    case CONNECT_STATUS:
        return QStringLiteral("Connect Status");
    case CONNECT_STREAM:
        return QStringLiteral("Connect Stream");
    case ONLINE:
        return QStringLiteral("Online");
    case RECONNECTING:
        return QStringLiteral("Reconnecting");
    }

    return QStringLiteral("Unknown");
}
