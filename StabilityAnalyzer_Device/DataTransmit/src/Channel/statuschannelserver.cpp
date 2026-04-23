#include "statuschannelserver.h"

StatusChannelServer::StatusChannelServer(QObject *parent)
    : TcpChannelServer(QStringLiteral("status"), 9001, parent)
{
}

bool StatusChannelServer::sendStatusMessage(const QJsonObject &payload)
{
    return sendJsonMessage(payload);
}
