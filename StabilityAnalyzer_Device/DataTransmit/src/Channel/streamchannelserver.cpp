#include "streamchannelserver.h"

StreamChannelServer::StreamChannelServer(QObject *parent)
    : TcpChannelServer(QStringLiteral("stream"), 9002, parent)
{
}

bool StreamChannelServer::sendStreamMessage(const QJsonObject &payload)
{
    return sendJsonMessage(payload);
}
