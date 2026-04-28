#include "Channel/tcpchannelclient.h"

// tcpchannelclient.cpp:
// 实现三路 TCP 通道共用的基础网络能力，包括建连、发包、按换行拆包和错误转发。
#include <QAbstractSocket>
#include <QHostAddress>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QDebug>
#include <QNetworkProxy>
#include <QTcpSocket>
#include <new>

namespace {

bool isHighFrequencyStatusMessage(const QString &channelName, const QJsonObject &message)
{
    if (channelName != QStringLiteral("status")) {
        return false;
    }

    const QString type = message.value(QStringLiteral("type")).toString();
    return type == QStringLiteral("heartbeat") || type == QStringLiteral("status_snapshot");
}

}

TcpChannelClient::TcpChannelClient(const QString &name, quint16 port, QObject *parent)
    : QObject(parent)
    , m_name(name)
    , m_port(port)
    , m_socket(new QTcpSocket(this))
{
    // RNDIS 通信属于本地直连链路，显式禁用代理，避免被系统代理配置干扰。
    m_socket->setProxy(QNetworkProxy::NoProxy);

    // 这里不区分业务通道，只负责把 socket 事件翻译成统一信号。
    connect(m_socket, &QTcpSocket::connected, this, &TcpChannelClient::connected);
    connect(m_socket, &QTcpSocket::disconnected, this, &TcpChannelClient::disconnected);
    connect(m_socket, &QTcpSocket::readyRead, this, &TcpChannelClient::onReadyRead);
    connect(m_socket,
            QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::error),
            this,
            &TcpChannelClient::onErrorOccurred);
}

TcpChannelClient::~TcpChannelClient() = default;

// 返回当前通道名。
QString TcpChannelClient::name() const
{
    return m_name;
}

// 返回当前通道端口。
quint16 TcpChannelClient::port() const
{
    return m_port;
}

// 返回底层 socket 是否处于已连接状态。
bool TcpChannelClient::isConnected() const
{
    return m_socket->state() == QAbstractSocket::ConnectedState;
}

// 建立到目标主机的连接。
void TcpChannelClient::connectToHost(const QString &host)
{
    if (isConnected()) {
        qDebug() << "[DataTransmit][" << m_name << "] already connected to"
                 << m_socket->peerAddress().toString() << ":" << m_socket->peerPort();
        return;
    }

    if (m_socket->state() == QAbstractSocket::ConnectingState
            || m_socket->state() == QAbstractSocket::HostLookupState) {
        qDebug() << "[DataTransmit][" << m_name << "] connect already in progress, state="
                 << m_socket->state();
        return;
    }

    if (m_socket->state() != QAbstractSocket::UnconnectedState) {
        qDebug() << "[DataTransmit][" << m_name << "] abort stale socket state before reconnect, state="
                 << m_socket->state();
        m_socket->abort();
    }

    qDebug() << "[DataTransmit][" << m_name << "] connectToHost begin, host=" << host << "port=" << m_port;
    m_socket->connectToHost(host, m_port);
}

// 主动断开连接并清空未处理缓冲区。
void TcpChannelClient::disconnectFromHost()
{
    qDebug() << "[DataTransmit][" << m_name << "] disconnectFromHost";
    m_buffer.clear();
    m_socket->abort();
}

// 发送一帧 JSON 消息。
// 按协议要求在末尾追加 '\n'，作为对端拆包边界。
bool TcpChannelClient::sendMessage(const QJsonObject &message, QString *errorMessage)
{
    if (!isConnected()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("%1 channel is not connected").arg(m_name);
        }
        qWarning() << "[DataTransmit][" << m_name << "] send failed because socket is not connected";
        return false;
    }

    QByteArray payload = QJsonDocument(message).toJson(QJsonDocument::Compact);
    payload.append('\n');
    qDebug() << "[DataTransmit][" << m_name << "] send" << payload.trimmed();

    const qint64 bytesWritten = m_socket->write(payload);
    if (bytesWritten != payload.size()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("%1 channel write failed").arg(m_name);
        }
        qWarning() << "[DataTransmit][" << m_name << "] write failed, bytesWritten="
                   << bytesWritten << "payloadSize=" << payload.size();
        return false;
    }

    qDebug() << "[DataTransmit][" << m_name << "] write queued, bytes=" << bytesWritten;
    return true;
}

// 读取 socket 新到达的数据并继续拆包。
void TcpChannelClient::onReadyRead()
{
    const QByteArray chunk = m_socket->readAll();
    if (m_name != QStringLiteral("status")) {
        qDebug() << "[DataTransmit][" << m_name << "] readyRead bytes=" << chunk.size();
    }
    m_buffer.append(chunk);
    processIncomingBuffer();
}

// 将底层 socket 错误转发给上层。
void TcpChannelClient::onErrorOccurred()
{
    qWarning() << "[DataTransmit][" << m_name << "] socket error="
               << m_socket->error() << m_socket->errorString();
    emit errorOccurred(m_socket->errorString());
}

void TcpChannelClient::processIncomingBuffer()
{
    // 协议以换行作为一帧结束标记，因此这里循环消费完整报文并保留未完成尾包。
    int newlineIndex = m_buffer.indexOf('\n');
    while (newlineIndex >= 0) {
        const QByteArray line = m_buffer.left(newlineIndex).trimmed();
        m_buffer.remove(0, newlineIndex + 1);

        if (!line.isEmpty()) {
            QJsonParseError parseError;
            const QJsonDocument document = QJsonDocument::fromJson(line, &parseError);
            if (parseError.error == QJsonParseError::NoError && document.isObject()) {
                try {
                    const QJsonObject object = document.object();
                    if (!isHighFrequencyStatusMessage(m_name, object)) {
                        qDebug() << "[DataTransmit][" << m_name << "] recv" << line.size();
                    }
                    emit messageReceived(object.toVariantMap());
                } catch (const std::bad_alloc &) {
                    qWarning() << "[DataTransmit][" << m_name
                               << "] insufficient memory while decoding JSON payload, bytes="
                               << line.size();
                    emit errorOccurred(QStringLiteral("%1 channel payload is too large to decode safely").arg(m_name));
                    m_buffer.clear();
                    return;
                }
            } else {
                qWarning() << "[DataTransmit][" << m_name << "] invalid json, error="
                           << parseError.errorString() << "payload=" << line;
                emit errorOccurred(QStringLiteral("%1 channel received invalid JSON").arg(m_name));
            }
        }

        newlineIndex = m_buffer.indexOf('\n');
    }
}
