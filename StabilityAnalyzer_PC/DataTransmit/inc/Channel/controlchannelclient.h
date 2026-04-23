#ifndef CONTROLCHANNELCLIENT_H
#define CONTROLCHANNELCLIENT_H

// ControlChannelClient:
// 9000 控制通道封装。
// 后续设备信息、实验开始/停止、历史数据拉取等命令类协议都从这里走。

#include "Channel/tcpchannelclient.h"

class DATATRANSMIT_EXPORT ControlChannelClient : public TcpChannelClient
{
    Q_OBJECT

public:
    // 9000：控制通道，后续承载 ping、设备信息、开始/停止实验等命令。
    explicit ControlChannelClient(QObject *parent = nullptr);
};

#endif // CONTROLCHANNELCLIENT_H
