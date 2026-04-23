#ifndef STREAMCHANNELSERVER_H
#define STREAMCHANNELSERVER_H

#include "tcpchannelserver.h"

/*
 * 文件功能：
 * StreamChannelServer 表示 Device 侧 9002 实时数据通道。
 * 该通道用于持续输出实验实时数据和进度信息。
 */
class DATATRANSMIT_EXPORT StreamChannelServer : public TcpChannelServer
{
    Q_OBJECT

public:
    /* 函数功能：创建实时数据通道服务端，固定监听 9002 端口。 */
    explicit StreamChannelServer(QObject *parent = nullptr);

    /*
     * 函数功能：
     * 发送实时数据消息。
     * @param payload 要推送给 PC 的实时数据消息。
     * @return 是否成功写入 socket。
     */
    bool sendStreamMessage(const QJsonObject &payload);
};

#endif // STREAMCHANNELSERVER_H
