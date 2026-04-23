#include "Channel/statuschannelclient.h"

StatusChannelClient::StatusChannelClient(QObject *parent)
    : TcpChannelClient(QStringLiteral("status"), 9001, parent)
{
}
