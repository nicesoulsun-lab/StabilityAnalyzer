#ifndef STREAMCHANNELCLIENT_H
#define STREAMCHANNELCLIENT_H

// StreamChannelClient:
// 9002 实时数据通道封装。
// 后续实验实时曲线、采样序号、进度等高频流式数据都从这里收发。

#include "Channel/tcpchannelclient.h"

class DATATRANSMIT_EXPORT StreamChannelClient : public TcpChannelClient
{
    Q_OBJECT

public:
    // 9002：实时数据通道，后续承载实验流数据和进度推送。
    explicit StreamChannelClient(QObject *parent = nullptr);
};

#endif // STREAMCHANNELCLIENT_H
