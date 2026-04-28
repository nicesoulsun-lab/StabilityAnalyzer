#include "inc/Common/experiment_state_store.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QSettings>

namespace {

ExperimentParams paramsFromVariantMap(const QVariantMap& params)
{
    ExperimentParams expParams;
    expParams.projectId = params.value("projectId", 0).toInt();
    expParams.projectName = params.value("projectName", "").toString();
    expParams.sampleName = params.value("sampleName", "").toString();
    expParams.operatorName = params.value("operatorName", "").toString();
    expParams.description = params.value("description", "").toString();

    expParams.durationDays = params.value("durationDays", 0).toInt();
    expParams.durationHours = params.value("durationHours", 0).toInt();
    expParams.durationMinutes = params.value("durationMinutes", 0).toInt();
    expParams.durationSeconds = params.value("durationSeconds", 0).toInt();

    expParams.intervalHours = params.value("intervalHours", 0).toInt();
    expParams.intervalMinutes = params.value("intervalMinutes", 0).toInt();
    expParams.intervalSeconds = params.value("intervalSeconds", 0).toInt();

    expParams.scanCount = params.value("scanCount", 0).toInt();
    expParams.temperatureControl = params.value("temperatureControl", false).toBool();
    expParams.targetTemperature = params.value("targetTemperature", 0.0).toDouble();
    expParams.scanRangeStart = params.value("scanRangeStart", 0).toInt();
    expParams.scanRangeEnd = params.value("scanRangeEnd", 0).toInt();
    expParams.scanStep = params.value("scanStep", 20).toInt();
    return expParams;
}

QVariantMap paramsToVariantMap(const ExperimentParams& params)
{
    QVariantMap data;
    data["projectId"] = params.projectId;
    data["projectName"] = params.projectName;
    data["sampleName"] = params.sampleName;
    data["operatorName"] = params.operatorName;
    data["description"] = params.description;

    data["durationDays"] = params.durationDays;
    data["durationHours"] = params.durationHours;
    data["durationMinutes"] = params.durationMinutes;
    data["durationSeconds"] = params.durationSeconds;

    data["intervalHours"] = params.intervalHours;
    data["intervalMinutes"] = params.intervalMinutes;
    data["intervalSeconds"] = params.intervalSeconds;

    data["scanCount"] = params.scanCount;
    data["temperatureControl"] = params.temperatureControl;
    data["targetTemperature"] = params.targetTemperature;

    data["scanRangeStart"] = params.scanRangeStart;
    data["scanRangeEnd"] = params.scanRangeEnd;
    data["scanStep"] = params.scanStep;
    return data;
}

QVariantMap defaultParamsVariantMap()
{
    QVariantMap params;
    params["projectId"] = 0;
    params["projectName"] = "";
    params["sampleName"] = "";
    params["operatorName"] = "";
    params["description"] = "";
    params["durationDays"] = 0;
    params["durationHours"] = 0;
    params["durationMinutes"] = 0;
    params["durationSeconds"] = 0;
    params["intervalHours"] = 0;
    params["intervalMinutes"] = 0;
    params["intervalSeconds"] = 0;
    params["scanCount"] = 0;
    params["temperatureControl"] = false;
    params["targetTemperature"] = 0.0;
    params["scanRangeStart"] = 0;
    params["scanRangeEnd"] = 0;
    params["scanStep"] = 20;
    return params;
}

QString experimentStateIniFilePath()
{
    return QCoreApplication::applicationDirPath() + "/config/experiment_paras/experiment_state.ini";
}

void ensureExperimentStateIniPathReady()
{
    const QFileInfo info(experimentStateIniFilePath());
    QDir dir = info.absoluteDir();
    if (!dir.exists()) {
        dir.mkpath(".");
    }
}

QString settingsGroupPath(const QString& rootGroup, const QString& channelKey)
{
    return rootGroup + "/" + channelKey;
}

bool settingsGroupExists(QSettings& settings, const QString& groupPath)
{
    settings.beginGroup(groupPath);
    const bool exists = !settings.allKeys().isEmpty() || !settings.childGroups().isEmpty();
    settings.endGroup();
    return exists;
}

QVariantMap readParamsFromCurrentGroup(QSettings& settings)
{
    QVariantMap params = defaultParamsVariantMap();
    params["projectId"] = settings.value("projectId", params.value("projectId"));
    params["projectName"] = settings.value("projectName", params.value("projectName"));
    params["sampleName"] = settings.value("sampleName", params.value("sampleName"));
    params["operatorName"] = settings.value("operatorName", params.value("operatorName"));
    params["description"] = settings.value("description", params.value("description"));
    params["durationDays"] = settings.value("durationDays", params.value("durationDays"));
    params["durationHours"] = settings.value("durationHours", params.value("durationHours"));
    params["durationMinutes"] = settings.value("durationMinutes", params.value("durationMinutes"));
    params["durationSeconds"] = settings.value("durationSeconds", params.value("durationSeconds"));
    params["intervalHours"] = settings.value("intervalHours", params.value("intervalHours"));
    params["intervalMinutes"] = settings.value("intervalMinutes", params.value("intervalMinutes"));
    params["intervalSeconds"] = settings.value("intervalSeconds", params.value("intervalSeconds"));
    params["scanCount"] = settings.value("scanCount", params.value("scanCount"));
    params["temperatureControl"] = settings.value("temperatureControl", params.value("temperatureControl"));
    params["targetTemperature"] = settings.value("targetTemperature", params.value("targetTemperature"));
    params["scanRangeStart"] = settings.value("scanRangeStart", params.value("scanRangeStart"));
    params["scanRangeEnd"] = settings.value("scanRangeEnd", params.value("scanRangeEnd"));
    params["scanStep"] = settings.value("scanStep", params.value("scanStep"));
    return params;
}

void writeParamsToCurrentGroup(QSettings& settings, const QVariantMap& params)
{
    for (auto it = params.constBegin(); it != params.constEnd(); ++it) {
        settings.setValue(it.key(), it.value());
    }
}

} // namespace

ExperimentStateStore::ExperimentStateStore()
{
}

QString ExperimentStateStore::channelKey(int channel) const
{
    switch (channel) {
    case 0: return "ChannelA";
    case 1: return "ChannelB";
    case 2: return "ChannelC";
    case 3: return "ChannelD";
    default: return "Unknown";
    }
}

void ExperimentStateStore::setParams(int channel, const ExperimentParams& params)
{
    m_params[channel] = params;

    ensureExperimentStateIniPathReady();
    QSettings settings(experimentStateIniFilePath(), QSettings::IniFormat);
    settings.beginGroup(settingsGroupPath("ExperimentParams", channelKey(channel)));
    writeParamsToCurrentGroup(settings, paramsToVariantMap(params));
    settings.endGroup();
    settings.sync();
}

bool ExperimentStateStore::hasParams(int channel) const
{
    return m_params.contains(channel);
}

ExperimentParams ExperimentStateStore::params(int channel) const
{
    return m_params.value(channel);
}

QVariantMap ExperimentStateStore::loadParams(int channel)
{
    ensureExperimentStateIniPathReady();
    const QString groupPath = settingsGroupPath("ExperimentParams", channelKey(channel));

    QVariantMap params;
    QSettings fileSettings(experimentStateIniFilePath(), QSettings::IniFormat);
    if (settingsGroupExists(fileSettings, groupPath)) {
        fileSettings.beginGroup(groupPath);
        params = readParamsFromCurrentGroup(fileSettings);
        fileSettings.endGroup();
    } else {
        QSettings legacySettings("StabilityAnalyzer", "ExperimentParams");
        if (settingsGroupExists(legacySettings, channelKey(channel))) {
            legacySettings.beginGroup(channelKey(channel));
            params = readParamsFromCurrentGroup(legacySettings);
            legacySettings.endGroup();

            fileSettings.beginGroup(groupPath);
            writeParamsToCurrentGroup(fileSettings, params);
            fileSettings.endGroup();
            fileSettings.sync();
        } else {
            params = defaultParamsVariantMap();
        }
    }

    if (!m_params.contains(channel)) {
        m_params[channel] = paramsFromVariantMap(params);
    }
    return params;
}

void ExperimentStateStore::setDefaultSerialConfig(int channel, const SerialConfig& cfg)
{
    m_serialConfigs[channel] = cfg;
}

SerialConfig ExperimentStateStore::serialConfig(int channel) const
{
    return m_serialConfigs.value(channel);
}

void ExperimentStateStore::setSerialConfig(int channel, const SerialConfig& cfg)
{
    m_serialConfigs[channel] = cfg;
}

void ExperimentStateStore::setSlaveId(int channel, int slaveId)
{
    m_slaveIds[channel] = slaveId;
}

int ExperimentStateStore::slaveId(int channel, int fallback) const
{
    return m_slaveIds.value(channel, fallback);
}

void ExperimentStateStore::saveSerialConfig(int channel) const
{
    Q_UNUSED(channel);
    // Runtime Modbus settings are managed by JSON files under config/experiment_devices.
}

void ExperimentStateStore::loadSerialConfig(int channel)
{
    Q_UNUSED(channel);
    // Keep in-memory defaults here; runtime comm settings come from experiment_devices JSON.
}

void ExperimentStateStore::initializeChannelStatus(int channel, const QVariantMap& initialStatus)
{
    m_channelStatuses[channel] = initialStatus;
}

QVariantMap ExperimentStateStore::channelStatus(int channel) const
{
    return m_channelStatuses.value(channel, QVariantMap{});
}

bool ExperimentStateStore::updateChannelStatus(int channel, const QVariantMap& patch, QVariantMap* mergedStatus)
{
    QVariantMap status = m_channelStatuses.value(channel);
    bool changed = false;

    for (auto it = patch.constBegin(); it != patch.constEnd(); ++it) {
        if (status.value(it.key()) != it.value()) {
            status[it.key()] = it.value();
            changed = true;
        }
    }

    if (changed) {
        m_channelStatuses[channel] = status;
    }

    if (mergedStatus) {
        *mergedStatus = status;
    }
    return changed;
}

QVector<QVariantMap>* ExperimentStateStore::memoryCache(int channel)
{
    return &m_memoryDataCache[channel];
}

void ExperimentStateStore::clearMemoryCache(int channel)
{
    m_memoryDataCache[channel].clear();
}
