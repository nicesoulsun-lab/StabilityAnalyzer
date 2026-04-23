#include "Channel/streamchannelclient.h"

StreamChannelClient::StreamChannelClient(QObject *parent)
    : TcpChannelClient(QStringLiteral("stream"), 9002, parent)
{
}
