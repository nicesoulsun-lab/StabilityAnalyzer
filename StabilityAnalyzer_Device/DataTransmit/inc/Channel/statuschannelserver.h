#ifndef STATUSCHANNELSERVER_H
#define STATUSCHANNELSERVER_H

#include "tcpchannelserver.h"

/*
 * 文件功能：
 * StatusChannelServer 表示 Device 侧 9001 状态通道。
 * 该通道用于持续推送在线状态、心跳和告警等状态信息。
 */
class DATATRANSMIT_EXPORT StatusChannelServer : public TcpChannelServer
{
    Q_OBJECT

public:
    /* 函数功能：创建状态通道服务端，固定监听 9001 端口。 */
    explicit StatusChannelServer(QObject *parent = nullptr);

    /*
     * 函数功能：
     * 发送状态通道消息。
     * @param payload 要推送给 PC 的状态消息。
     * @return 是否成功写入 socket。
     */
    bool sendStatusMessage(const QJsonObject &payload);
};

#endif // STATUSCHANNELSERVER_H
