#include "Channel/controlchannelclient.h"

ControlChannelClient::ControlChannelClient(QObject *parent)
    : TcpChannelClient(QStringLiteral("control"), 9000, parent)
{
}
