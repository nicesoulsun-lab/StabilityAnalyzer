#ifndef CONTROLCHANNELSERVER_H
#define CONTROLCHANNELSERVER_H

#include "tcpchannelserver.h"

/*
 * 文件功能：
 * ControlChannelServer 表示 Device 侧 9000 控制通道。
 * 当前阶段先承载基础握手和控制命令收发，后续再扩展业务命令。
 */
class DATATRANSMIT_EXPORT ControlChannelServer : public TcpChannelServer
{
    Q_OBJECT

public:
    /* 函数功能：创建控制通道服务端，固定监听 9000 端口。 */
    explicit ControlChannelServer(QObject *parent = nullptr);

    /*
     * 函数功能：
     * 发送控制通道响应消息。
     * @param payload 要回复给 PC 的控制消息。
     * @return 是否成功写入 socket。
     */
    bool sendControlMessage(const QJsonObject &payload);
};

#endif // CONTROLCHANNELSERVER_H
