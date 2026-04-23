#include "controlchannelserver.h"

ControlChannelServer::ControlChannelServer(QObject *parent)
    : TcpChannelServer(QStringLiteral("control"), 9000, parent)
{
}

bool ControlChannelServer::sendControlMessage(const QJsonObject &payload)
{
    return sendJsonMessage(payload);
}
