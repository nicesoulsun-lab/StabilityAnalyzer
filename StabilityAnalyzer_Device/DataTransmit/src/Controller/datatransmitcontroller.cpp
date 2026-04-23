#include "datatransmitcontroller.h"

#include "controlchannelserver.h"
#include "statuschannelserver.h"
#include "streamchannelserver.h"
#include "rndismanager.h"

#include <QDateTime>
#include <QJsonArray>
#include <QJsonObject>
#include <QTimer>

DataTransmitController::DataTransmitController(QObject *parent)
    : QObject(parent)
    , m_rndisManager(new RndisManager(this))
    , m_controlServer(new ControlChannelServer(this))
    , m_statusServer(new StatusChannelServer(this))
    , m_streamServer(new StreamChannelServer(this))
    , m_startupTimer(new QTimer(this))
    , m_heartbeatTimer(new QTimer(this))
{
    m_startupTimer->setSingleShot(true);
    m_heartbeatTimer->setInterval(1000);

    connect(m_startupTimer, &QTimer::timeout, this, &DataTransmitController::performStartupStep);
    connect(m_heartbeatTimer, &QTimer::timeout, this, &DataTransmitController::sendHeartbeat);

    connect(m_rndisManager, &RndisManager::logMessage, this, &DataTransmitController::logMessage);
    connect(m_rndisManager, &RndisManager::readyChanged, this, &DataTransmitController::rndisReadyChanged);
    connect(m_rndisManager, &RndisManager::lastErrorChanged, this, [this]() {
        setLastError(m_rndisManager->lastError());
    });

    connect(m_controlServer, &ControlChannelServer::messageReceived, this, &DataTransmitController::handleControlMessage);
    connect(m_controlServer, &ControlChannelServer::messageReceived, this, [this](const QJsonObject &message) {
        emit controlMessageReceived(message.toVariantMap());
    });
    connect(m_statusServer, &StatusChannelServer::messageReceived, this, [this](const QJsonObject &message) {
        emit statusMessageReceived(message.toVariantMap());
    });
    connect(m_streamServer, &StreamChannelServer::messageReceived, this, [this](const QJsonObject &message) {
        emit streamMessageReceived(message.toVariantMap());
    });

    connect(m_controlServer, &ControlChannelServer::logMessage, this, &DataTransmitController::logMessage);
    connect(m_statusServer, &StatusChannelServer::logMessage, this, &DataTransmitController::logMessage);
    connect(m_streamServer, &StreamChannelServer::logMessage, this, &DataTransmitController::logMessage);

    connect(m_controlServer, &ControlChannelServer::errorOccurred, this, &DataTransmitController::handleTransportError);
    connect(m_statusServer, &StatusChannelServer::errorOccurred, this, &DataTransmitController::handleTransportError);
    connect(m_streamServer, &StreamChannelServer::errorOccurred, this, &DataTransmitController::handleTransportError);

    connect(m_controlServer, &ControlChannelServer::listeningChanged, this, &DataTransmitController::handleChannelStateChanged);
    connect(m_statusServer, &StatusChannelServer::listeningChanged, this, &DataTransmitController::handleChannelStateChanged);
    connect(m_streamServer, &StreamChannelServer::listeningChanged, this, &DataTransmitController::handleChannelStateChanged);
    connect(m_controlServer, &ControlChannelServer::clientConnectedChanged, this, &DataTransmitController::handleChannelStateChanged);
    connect(m_statusServer, &StatusChannelServer::clientConnectedChanged, this, &DataTransmitController::handleChannelStateChanged);
    connect(m_streamServer, &StreamChannelServer::clientConnectedChanged, this, &DataTransmitController::handleChannelStateChanged);

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

DataTransmitController::~DataTransmitController()
{
    stopConnection();
}

DataTransmitController::ConnectionState DataTransmitController::connectionState() const
{
    return m_connectionState;
}

QString DataTransmitController::connectionStateText() const
{
    switch (m_connectionState) {
    case Init:
        return QStringLiteral("INIT");
    case StartingRndis:
        return QStringLiteral("STARTING_RNDIS");
    case WaitingNetworkReady:
        return QStringLiteral("WAITING_NETWORK_READY");
    case StartingControlServer:
        return QStringLiteral("STARTING_CONTROL_SERVER");
    case StartingStatusServer:
        return QStringLiteral("STARTING_STATUS_SERVER");
    case StartingStreamServer:
        return QStringLiteral("STARTING_STREAM_SERVER");
    case Online:
        return QStringLiteral("ONLINE");
    case Restarting:
        return QStringLiteral("RESTARTING");
    }

    return QStringLiteral("UNKNOWN");
}

bool DataTransmitController::isRndisReady() const
{
    return m_rndisManager->isReady();
}

bool DataTransmitController::isControlListening() const
{
    return m_controlServer->isListening();
}

bool DataTransmitController::isStatusListening() const
{
    return m_statusServer->isListening();
}

bool DataTransmitController::isStreamListening() const
{
    return m_streamServer->isListening();
}

bool DataTransmitController::isControlClientConnected() const
{
    return m_controlServer->isClientConnected();
}

bool DataTransmitController::isStatusClientConnected() const
{
    return m_statusServer->isClientConnected();
}

bool DataTransmitController::isStreamClientConnected() const
{
    return m_streamServer->isClientConnected();
}

QString DataTransmitController::scriptPath() const
{
    return m_rndisManager->scriptPath();
}

void DataTransmitController::setScriptPath(const QString &scriptPath)
{
    if (m_rndisManager->scriptPath() == scriptPath) {
        return;
    }

    m_rndisManager->setScriptPath(scriptPath);
    emit scriptPathChanged();
}

QString DataTransmitController::networkInterface() const
{
    return m_rndisManager->interfaceName();
}

void DataTransmitController::setNetworkInterface(const QString &interfaceName)
{
    if (m_rndisManager->interfaceName() == interfaceName) {
        return;
    }

    m_rndisManager->setInterfaceName(interfaceName);
    emit networkInterfaceChanged();
}

QString DataTransmitController::deviceIp() const
{
    return m_rndisManager->deviceIp();
}

void DataTransmitController::setDeviceIp(const QString &deviceIp)
{
    if (m_rndisManager->deviceIp() == deviceIp) {
        return;
    }

    m_rndisManager->setDeviceIp(deviceIp);
    emit deviceIpChanged();
}

QVariantList DataTransmitController::experimentChannels() const
{
    return m_experimentChannels;
}

QString DataTransmitController::lastError() const
{
    return m_lastError;
}

bool DataTransmitController::autoRestartEnabled() const
{
    return m_autoRestartEnabled;
}

void DataTransmitController::setAutoRestartEnabled(bool enabled)
{
    if (m_autoRestartEnabled == enabled) {
        return;
    }

    m_autoRestartEnabled = enabled;
    emit autoRestartEnabledChanged();
}

void DataTransmitController::startConnection()
{
    m_manuallyStopped = false;
    setLastError(QString());
    setConnectionState(StartingRndis);
    m_startupTimer->start(0);
}

void DataTransmitController::stopConnection()
{
    m_manuallyStopped = true;
    m_startupTimer->stop();
    m_heartbeatTimer->stop();
    stopServers();
    setConnectionState(Init);
}

void DataTransmitController::restartNow()
{
    m_manuallyStopped = false;
    stopServers();
    setConnectionState(Restarting);
    m_startupTimer->start(0);
}

bool DataTransmitController::sendStatusMessage(const QVariantMap &payload)
{
    return m_statusServer->sendStatusMessage(QJsonObject::fromVariantMap(payload));
}

bool DataTransmitController::sendStreamMessage(const QVariantMap &payload)
{
    return m_streamServer->sendStreamMessage(QJsonObject::fromVariantMap(payload));
}

void DataTransmitController::updateExperimentChannelStatus(int channel, const QVariantMap &status)
{
    if (channel < 0 || channel >= m_experimentChannels.size()) {
        return;
    }

    QVariantMap mergedStatus = m_experimentChannels.at(channel).toMap();
    for (auto it = status.constBegin(); it != status.constEnd(); ++it) {
        mergedStatus.insert(it.key(), it.value());
    }
    mergedStatus.insert(QStringLiteral("channel"), channel);

    if (m_experimentChannels.at(channel).toMap() == mergedStatus) {
        return;
    }

    m_experimentChannels[channel] = mergedStatus;
    emit experimentChannelsChanged();

    if (m_statusServer->isClientConnected() && m_connectionState == Online) {
        pushStatusSnapshot(QStringLiteral("status_snapshot"));
    }
}

void DataTransmitController::performStartupStep()
{
    if (m_manuallyStopped) {
        return;
    }

    switch (m_connectionState) {
    case StartingRndis:
        if (!m_rndisManager->initialize()) {
            handleTransportError(QStringLiteral("Failed to initialize RNDIS"));
            return;
        }
        setConnectionState(WaitingNetworkReady);
        m_startupTimer->start(300);
        return;

    case WaitingNetworkReady:
        if (!m_rndisManager->refreshNetworkState()) {
            scheduleRestart(1000);
            return;
        }
        setConnectionState(StartingControlServer);
        m_startupTimer->start(0);
        return;

    case StartingControlServer:
        if (!m_controlServer->startListening()) {
            handleTransportError(QStringLiteral("Failed to start control server"));
            return;
        }
        setConnectionState(StartingStatusServer);
        m_startupTimer->start(0);
        return;

    case StartingStatusServer:
        if (!m_statusServer->startListening()) {
            handleTransportError(QStringLiteral("Failed to start status server"));
            return;
        }
        setConnectionState(StartingStreamServer);
        m_startupTimer->start(0);
        return;

    case StartingStreamServer:
        if (!m_streamServer->startListening()) {
            handleTransportError(QStringLiteral("Failed to start stream server"));
            return;
        }
        setConnectionState(Online);
        m_heartbeatTimer->start();
        emit logMessage(QStringLiteral("Device communication service is online"));
        return;

    case Restarting:
        setConnectionState(StartingRndis);
        m_startupTimer->start(0);
        return;

    case Init:
    case Online:
        return;
    }
}

void DataTransmitController::sendHeartbeat()
{
    if (m_connectionState != Online || !m_statusServer->isClientConnected()) {
        return;
    }

    pushStatusSnapshot(QStringLiteral("heartbeat"));
}

void DataTransmitController::handleControlMessage(const QJsonObject &message)
{
    const QString command = message.value(QStringLiteral("cmd")).toString();
    if (command.isEmpty()) {
        return;
    }

    if (command == QStringLiteral("ping")) {
        QJsonObject response;
        response.insert(QStringLiteral("type"), QStringLiteral("pong"));
        response.insert(QStringLiteral("timestamp"), QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
        m_controlServer->sendControlMessage(response);
        return;
    }

    if (command == QStringLiteral("get_device_info")) {
        m_controlServer->sendControlMessage(buildDeviceInfo());
        return;
    }

    if (command == QStringLiteral("get_channel_info")) {
        QJsonObject response = buildStatusSnapshot();
        response.insert(QStringLiteral("type"), QStringLiteral("channel_info"));
        m_controlServer->sendControlMessage(response);
        return;
    }

    QJsonObject response;
    response.insert(QStringLiteral("type"), QStringLiteral("ack"));
    response.insert(QStringLiteral("cmd"), command);
    response.insert(QStringLiteral("result"), QStringLiteral("unsupported"));
    m_controlServer->sendControlMessage(response);
}

void DataTransmitController::handleChannelStateChanged()
{
    emit channelsStateChanged();

    if (m_connectionState == Online && m_statusServer->isClientConnected()) {
        pushStatusSnapshot(QStringLiteral("status_snapshot"));
    }
}

void DataTransmitController::handleTransportError(const QString &message)
{
    if (m_manuallyStopped) {
        return;
    }

    setLastError(message);
    emit logMessage(message);
    scheduleRestart(2000);
}

void DataTransmitController::setConnectionState(DataTransmitController::ConnectionState state)
{
    if (m_connectionState == state) {
        return;
    }

    m_connectionState = state;
    emit connectionStateChanged();
}

void DataTransmitController::setLastError(const QString &errorText)
{
    if (m_lastError == errorText) {
        return;
    }

    m_lastError = errorText;
    emit lastErrorChanged();
}

void DataTransmitController::scheduleRestart(int delayMs)
{
    if (m_manuallyStopped) {
        return;
    }

    stopServers();
    m_heartbeatTimer->stop();
    setConnectionState(Restarting);

    if (!m_autoRestartEnabled) {
        return;
    }

    m_startupTimer->start(delayMs);
}

void DataTransmitController::stopServers()
{
    m_controlServer->stopListening();
    m_statusServer->stopListening();
    m_streamServer->stopListening();
}

QJsonObject DataTransmitController::buildStatusSnapshot() const
{
    QJsonObject snapshot;
    snapshot.insert(QStringLiteral("connection_state"), connectionStateText());
    snapshot.insert(QStringLiteral("rndis_ready"), isRndisReady());
    snapshot.insert(QStringLiteral("device_ip"), deviceIp());
    snapshot.insert(QStringLiteral("network_interface"), networkInterface());
    snapshot.insert(QStringLiteral("control_listening"), isControlListening());
    snapshot.insert(QStringLiteral("status_listening"), isStatusListening());
    snapshot.insert(QStringLiteral("stream_listening"), isStreamListening());
    snapshot.insert(QStringLiteral("control_client_connected"), isControlClientConnected());
    snapshot.insert(QStringLiteral("status_client_connected"), isStatusClientConnected());
    snapshot.insert(QStringLiteral("stream_client_connected"), isStreamClientConnected());

    QJsonArray experimentChannels;
    for (const QVariant &channelStatus : m_experimentChannels) {
        experimentChannels.append(QJsonObject::fromVariantMap(channelStatus.toMap()));
    }
    snapshot.insert(QStringLiteral("experiment_channels"), experimentChannels);
    return snapshot;
}

bool DataTransmitController::pushStatusSnapshot(const QString &messageType)
{
    if (!m_statusServer->isClientConnected()) {
        emit logMessage(QStringLiteral("Skip sending %1 because status client is not connected").arg(messageType));
        return false;
    }

    QJsonObject snapshot = buildStatusSnapshot();
    snapshot.insert(QStringLiteral("type"), messageType);
    snapshot.insert(QStringLiteral("timestamp"), QDateTime::currentDateTimeUtc().toString(Qt::ISODate));

    const bool success = m_statusServer->sendStatusMessage(snapshot);
    emit logMessage(QStringLiteral("Send %1 to status channel, success=%2")
                    .arg(messageType, success ? QStringLiteral("true") : QStringLiteral("false")));
    return success;
}

QJsonObject DataTransmitController::buildDeviceInfo() const
{
    QJsonObject info;
    info.insert(QStringLiteral("type"), QStringLiteral("device_info"));
    info.insert(QStringLiteral("device_ip"), deviceIp());
    info.insert(QStringLiteral("network_interface"), networkInterface());
    info.insert(QStringLiteral("script_path"), scriptPath());
    info.insert(QStringLiteral("control_port"), 9000);
    info.insert(QStringLiteral("status_port"), 9001);
    info.insert(QStringLiteral("stream_port"), 9002);
    info.insert(QStringLiteral("rndis_ready"), isRndisReady());
    return info;
}
