#ifndef STATUSCHANNELCLIENT_H
#define STATUSCHANNELCLIENT_H

// StatusChannelClient:
// 9001 状态通道封装。
// 后续周期状态、心跳、在线信息、告警等消息都从这里收发。

#include "Channel/tcpchannelclient.h"

class DATATRANSMIT_EXPORT StatusChannelClient : public TcpChannelClient
{
    Q_OBJECT

public:
    // 9001：状态通道，后续承载心跳、在线状态、告警等周期消息。
    explicit StatusChannelClient(QObject *parent = nullptr);
};

#endif // STATUSCHANNELCLIENT_H
