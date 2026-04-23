#ifndef TCPCHANNELSERVER_H
#define TCPCHANNELSERVER_H

#include "datatransmit_global.h"

#include <QObject>
#include <QJsonObject>
#include <QHostAddress>

class QTcpServer;
class QTcpSocket;

/*
 * 文件功能：
 * TcpChannelServer 是 Device 侧三路 TCP 通道的公共基类。
 * 它统一处理端口监听、客户端接入、JSON + 换行分帧协议、
 * 粘包半包拆分、以及基础日志和错误转发。
 */
class DATATRANSMIT_EXPORT TcpChannelServer : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool listening READ isListening NOTIFY listeningChanged)
    Q_PROPERTY(bool clientConnected READ isClientConnected NOTIFY clientConnectedChanged)
    Q_PROPERTY(QString channelName READ channelName CONSTANT)
    Q_PROPERTY(quint16 port READ port CONSTANT)
    Q_PROPERTY(QString clientAddress READ clientAddress NOTIFY clientConnectedChanged)

public:
    /*
     * 函数功能：
     * 创建一个通用 TCP 通道服务端。
     * @param channelName 当前通道名称，用于日志输出和上层状态展示。
     * @param port 当前通道监听端口。
     */
    explicit TcpChannelServer(const QString &channelName, quint16 port, QObject *parent = nullptr);
    ~TcpChannelServer() override;

    /*
     * 函数功能：
     * 启动当前通道的 TCP 监听。
     * @param address 监听地址，默认监听全部 IPv4 地址。
     * @return 监听是否成功。
     */
    bool startListening(const QHostAddress &address = QHostAddress::AnyIPv4);

    /*
     * 函数功能：
     * 停止监听并主动断开当前客户端。
     */
    void stopListening();

    /*
     * 函数功能：
     * 向当前连接的客户端发送一条 JSON 消息。
     * 协议格式统一为 JSON 文本后追加 '\n'。
     * @param message 要发送的 JSON 对象。
     * @return 是否写入到 socket。
     */
    bool sendJsonMessage(const QJsonObject &message);

    bool isListening() const;
    bool isClientConnected() const;
    QString channelName() const;
    quint16 port() const;
    QString clientAddress() const;

signals:
    /* 信号功能：监听状态发生变化时通知上层。 */
    void listeningChanged();

    /* 信号功能：客户端连接状态变化时通知上层。 */
    void clientConnectedChanged();

    /*
     * 信号功能：
     * 上报解析成功的 JSON 消息。
     * @param message 收到的一条完整 JSON 消息。
     */
    void messageReceived(const QJsonObject &message);

    /*
     * 信号功能：
     * 上报收到的原始协议文本，便于调试通信。
     * @param payload 当前帧的原始文本内容。
     */
    void rawMessageReceived(const QByteArray &payload);

    /* 信号功能：上报通道日志文本。 */
    void logMessage(const QString &message);

    /* 信号功能：上报通道错误文本。 */
    void errorOccurred(const QString &message);

protected:
    /*
     * 函数功能：
     * 派生类可重写该函数，处理某个通道自己的协议逻辑。
     * 基类在默认实现中只转发 messageReceived 信号。
     * @param message 收到的一条 JSON 消息。
     */
    virtual void handleJsonMessage(const QJsonObject &message);

    /* 函数功能：统一格式化日志并发给上层。 */
    void emitLog(const QString &message);

    /* 属性功能：当前通道的展示名称。 */
    QString m_channelName;

    /* 属性功能：当前通道固定监听端口。 */
    quint16 m_port = 0;

private slots:
    /* 函数功能：接收新的 TCP 客户端连接，只保留最后一个有效连接。 */
    void handleNewConnection();

    /* 函数功能：处理客户端发来的字节流，并按换行分帧后继续解析。 */
    void handleReadyRead();

    /* 函数功能：响应客户端断开事件，更新当前通道状态。 */
    void handleClientDisconnected();

    /* 函数功能：响应 socket 错误并向上层透出。 */
    void handleSocketError();

private:
    /* 函数功能：释放旧客户端连接，并重置缓存。 */
    void resetClient();

    /* 函数功能：从接收缓存中提取一行协议文本并解析 JSON。 */
    void processBufferedMessages();

    /* 属性功能：当前通道的 TCP 监听器。 */
    QTcpServer *m_server = nullptr;

    /* 属性功能：当前接入的客户端 socket。 */
    QTcpSocket *m_clientSocket = nullptr;

    /* 属性功能：当前客户端未处理完的接收缓存，用于处理粘包和半包。 */
    QByteArray m_receiveBuffer;
};

#endif // TCPCHANNELSERVER_H
