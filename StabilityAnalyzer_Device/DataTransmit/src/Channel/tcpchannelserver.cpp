#include "tcpchannelserver.h"

#include <QJsonDocument>
#include <QJsonParseError>
#include <QTcpServer>
#include <QTcpSocket>

TcpChannelServer::TcpChannelServer(const QString &channelName, quint16 port, QObject *parent)
    : QObject(parent)
    , m_channelName(channelName)
    , m_port(port)
    , m_server(new QTcpServer(this))
{
    connect(m_server, &QTcpServer::newConnection, this, &TcpChannelServer::handleNewConnection);
}

TcpChannelServer::~TcpChannelServer()
{
    stopListening();
}

bool TcpChannelServer::startListening(const QHostAddress &address)
{
    if (m_server->isListening()) {
        return true;
    }

    if (!m_server->listen(address, m_port)) {
        const QString message = QString("%1 listen failed: %2").arg(m_channelName, m_server->errorString());
        emit errorOccurred(message);
        emitLog(message);
        return false;
    }

    emit listeningChanged();
    emitLog(QString("%1 listening on %2:%3").arg(m_channelName, m_server->serverAddress().toString()).arg(m_port));
    return true;
}

void TcpChannelServer::stopListening()
{
    resetClient();

    if (m_server->isListening()) {
        m_server->close();
        emit listeningChanged();
        emitLog(QString("%1 stopped listening").arg(m_channelName));
    }
}

bool TcpChannelServer::sendJsonMessage(const QJsonObject &message)
{
    if (!m_clientSocket || m_clientSocket->state() != QAbstractSocket::ConnectedState) {
        return false;
    }

    QByteArray payload = QJsonDocument(message).toJson(QJsonDocument::Compact);
    payload.append('\n');
    const qint64 written = m_clientSocket->write(payload);
    return written == payload.size();
}

bool TcpChannelServer::isListening() const
{
    return m_server->isListening();
}

bool TcpChannelServer::isClientConnected() const
{
    return m_clientSocket && m_clientSocket->state() == QAbstractSocket::ConnectedState;
}

QString TcpChannelServer::channelName() const
{
    return m_channelName;
}

quint16 TcpChannelServer::port() const
{
    return m_port;
}

QString TcpChannelServer::clientAddress() const
{
    return m_clientSocket ? m_clientSocket->peerAddress().toString() : QString();
}

void TcpChannelServer::handleJsonMessage(const QJsonObject &message)
{
    emit messageReceived(message);
}

void TcpChannelServer::emitLog(const QString &message)
{
    emit logMessage(message);
}

void TcpChannelServer::handleNewConnection()
{
    QTcpSocket *nextClient = m_server->nextPendingConnection();
    if (!nextClient) {
        return;
    }

    resetClient();

    m_clientSocket = nextClient;
    m_clientSocket->setParent(this);
    connect(m_clientSocket, &QTcpSocket::readyRead, this, &TcpChannelServer::handleReadyRead);
    connect(m_clientSocket, &QTcpSocket::disconnected, this, &TcpChannelServer::handleClientDisconnected);
    connect(m_clientSocket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(handleSocketError()));

    emit clientConnectedChanged();
    emitLog(QString("%1 client connected: %2").arg(m_channelName, clientAddress()));
}

void TcpChannelServer::handleReadyRead()
{
    if (!m_clientSocket) {
        return;
    }

    m_receiveBuffer.append(m_clientSocket->readAll());
    processBufferedMessages();
}

void TcpChannelServer::handleClientDisconnected()
{
    emitLog(QString("%1 client disconnected").arg(m_channelName));
    resetClient();
}

void TcpChannelServer::handleSocketError()
{
    if (!m_clientSocket) {
        return;
    }

    const QString message = QString("%1 socket error: %2").arg(m_channelName, m_clientSocket->errorString());
    emit errorOccurred(message);
    emitLog(message);
}

void TcpChannelServer::resetClient()
{
    if (m_clientSocket) {
        m_clientSocket->disconnect(this);
        if (m_clientSocket->state() != QAbstractSocket::UnconnectedState) {
            m_clientSocket->disconnectFromHost();
        }
        m_clientSocket->deleteLater();
        m_clientSocket = nullptr;
        m_receiveBuffer.clear();
        emit clientConnectedChanged();
    }
}

void TcpChannelServer::processBufferedMessages()
{
    while (true) {
        const int newlineIndex = m_receiveBuffer.indexOf('\n');
        if (newlineIndex < 0) {
            return;
        }

        QByteArray frame = m_receiveBuffer.left(newlineIndex).trimmed();
        m_receiveBuffer.remove(0, newlineIndex + 1);

        if (frame.isEmpty()) {
            continue;
        }

        emit rawMessageReceived(frame);

        QJsonParseError error;
        const QJsonDocument document = QJsonDocument::fromJson(frame, &error);
        if (error.error != QJsonParseError::NoError || !document.isObject()) {
            const QString message = QString("%1 invalid json: %2").arg(m_channelName, error.errorString());
            emit errorOccurred(message);
            emitLog(message);
            continue;
        }

        handleJsonMessage(document.object());
    }
}
