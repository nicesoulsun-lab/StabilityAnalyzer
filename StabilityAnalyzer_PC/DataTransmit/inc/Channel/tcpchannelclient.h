#ifndef TCPCHANNELCLIENT_H
#define TCPCHANNELCLIENT_H

// TcpChannelClient:
// 三路 TCP 通道共用的基础客户端。
// 负责 socket 建连、断连、JSON + 换行协议收发，以及 TCP 粘包/半包处理。

#include <QObject>
#include <QVariantMap>
#include "datatransmit_global.h"

class QJsonObject;
class QTcpSocket;

class DATATRANSMIT_EXPORT TcpChannelClient : public QObject
{
    Q_OBJECT

public:
    // 创建一个具名 TCP 通道客户端。
    // name 用于日志和错误信息；port 为目标端口。
    explicit TcpChannelClient(const QString &name, quint16 port, QObject *parent = nullptr);
    // 析构通道客户端。
    ~TcpChannelClient() override;

    // 返回当前通道名称。
    QString name() const;
    // 返回当前通道端口。
    quint16 port() const;
    // 返回底层 socket 是否处于已连接状态。
    bool isConnected() const;

    // 连接到指定主机和当前通道端口。
    void connectToHost(const QString &host);
    // 主动断开当前通道并清理缓冲区。
    void disconnectFromHost();
    // 协议层统一使用 JSON + '\n'，这里负责序列化和基础写入。
    bool sendMessage(const QJsonObject &message, QString *errorMessage = nullptr);

signals:
    // 底层 socket 连接成功。
    void connected();
    // 底层 socket 断开连接。
    void disconnected();
    // 底层 socket 或协议解析发生错误。
    void errorOccurred(const QString &errorMessage);
    // 收到一帧完整 JSON 消息。
    void messageReceived(const QVariantMap &message);

private slots:
    // 响应 socket 可读事件，继续消费接收缓冲区。
    void onReadyRead();
    // 响应底层 socket 错误。
    void onErrorOccurred();

private:
    // 读取侧按换行拆包，避免上层直接处理 TCP 粘包/半包问题。
    void processIncomingBuffer();

private:
    // 通道名称，仅用于日志和错误标识。
    QString m_name;
    // 通道端口。
    quint16 m_port;
    // 底层 TCP socket。
    QTcpSocket *m_socket;
    // 接收缓冲区，缓存尚未凑成完整一帧的数据。
    QByteArray m_buffer;
};

#endif // TCPCHANNELCLIENT_H
