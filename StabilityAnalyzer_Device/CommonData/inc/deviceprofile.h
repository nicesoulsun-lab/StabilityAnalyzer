#ifndef DEVICEPROFILE_H
#define DEVICEPROFILE_H

#include <QCoreApplication>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStringList>

struct DeviceProfile
{
    QString deviceModel = QStringLiteral("four_tower");
    int channelCount = 4;
    QStringList channelNames = {QStringLiteral("A"),
                                QStringLiteral("B"),
                                QStringLiteral("C"),
                                QStringLiteral("D")};

    QString channelName(int index) const
    {
        if (index < 0) {
            return QString();
        }

        if (index < channelNames.size() && !channelNames.at(index).trimmed().isEmpty()) {
            return channelNames.at(index).trimmed();
        }

        return QString(QChar('A' + index));
    }
};

inline DeviceProfile loadDeviceProfile()
{
    DeviceProfile profile;
    const QString profilePath = QCoreApplication::applicationDirPath()
            + QStringLiteral("/config/device_profile.json");

    QFile file(profilePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return profile;
    }

    const QJsonDocument document = QJsonDocument::fromJson(file.readAll());
    if (!document.isObject()) {
        return profile;
    }

    const QJsonObject object = document.object();
    profile.deviceModel = object.value(QStringLiteral("deviceModel"))
                                  .toString(profile.deviceModel);
    profile.channelCount = object.value(QStringLiteral("channelCount"))
                                   .toInt(profile.channelCount);
    profile.channelCount = qBound(1, profile.channelCount, 4);

    const QJsonArray channelNames = object.value(QStringLiteral("channelNames")).toArray();
    if (!channelNames.isEmpty()) {
        QStringList names;
        for (const QJsonValue &value : channelNames) {
            const QString name = value.toString().trimmed();
            if (!name.isEmpty()) {
                names.append(name);
            }
        }
        if (!names.isEmpty()) {
            profile.channelNames = names;
        }
    }

    while (profile.channelNames.size() < profile.channelCount) {
        profile.channelNames.append(QString(QChar('A' + profile.channelNames.size())));
    }

    return profile;
}

inline const DeviceProfile &deviceProfile()
{
    static const DeviceProfile profile = loadDeviceProfile();
    return profile;
}

#endif // DEVICEPROFILE_H
