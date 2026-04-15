#include "inc/Common/experiment_state_store.h"

#include <QSettings>

namespace {
ExperimentParams paramsFromVariantMap(const QVariantMap& params)
{
    ExperimentParams expParams;
    expParams.projectId = params.value("projectId", 0).toInt();
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
}

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

    QSettings settings("StabilityAnalyzer", "ExperimentParams");
    settings.beginGroup(channelKey(channel));
    const QVariantMap data = paramsToVariantMap(params);
    for (auto it = data.constBegin(); it != data.constEnd(); ++it) {
        settings.setValue(it.key(), it.value());
    }
    settings.endGroup();
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
    QSettings settings("StabilityAnalyzer", "ExperimentParams");
    settings.beginGroup(channelKey(channel));

    QVariantMap params;
    params["projectId"] = settings.value("projectId", 0);
    params["sampleName"] = settings.value("sampleName", "");
    params["operatorName"] = settings.value("operatorName", "");
    params["description"] = settings.value("description", "");

    params["durationDays"] = settings.value("durationDays", 0);
    params["durationHours"] = settings.value("durationHours", 0);
    params["durationMinutes"] = settings.value("durationMinutes", 0);
    params["durationSeconds"] = settings.value("durationSeconds", 0);

    params["intervalHours"] = settings.value("intervalHours", 0);
    params["intervalMinutes"] = settings.value("intervalMinutes", 0);
    params["intervalSeconds"] = settings.value("intervalSeconds", 0);

    params["scanCount"] = settings.value("scanCount", 0);
    params["temperatureControl"] = settings.value("temperatureControl", false);
    params["targetTemperature"] = settings.value("targetTemperature", 0.0);

    params["scanRangeStart"] = settings.value("scanRangeStart", 0);
    params["scanRangeEnd"] = settings.value("scanRangeEnd", 0);
    params["scanStep"] = settings.value("scanStep", 20);
    settings.endGroup();

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
    QSettings settings("StabilityAnalyzer", "ModbusConfig");
    settings.beginGroup(channelKey(channel));

    const SerialConfig cfg = m_serialConfigs.value(channel);
    settings.setValue("portName", cfg.portName);
    settings.setValue("baudRate", cfg.baudRate);
    settings.setValue("dataBits", cfg.dataBits);
    settings.setValue("parity", cfg.parity);
    settings.setValue("stopBits", cfg.stopBits);
    settings.setValue("slaveId", m_slaveIds.value(channel, channel + 1));

    settings.endGroup();
}

void ExperimentStateStore::loadSerialConfig(int channel)
{
    QSettings settings("StabilityAnalyzer", "ModbusConfig");
    settings.beginGroup(channelKey(channel));

    SerialConfig cfg = m_serialConfigs.value(channel);
    cfg.portName = settings.value("portName", cfg.portName).toString();
    cfg.baudRate = settings.value("baudRate", cfg.baudRate).toInt();
    cfg.dataBits = settings.value("dataBits", cfg.dataBits).toInt();
    cfg.parity = settings.value("parity", cfg.parity).toString();
    cfg.stopBits = settings.value("stopBits", cfg.stopBits).toInt();
    m_serialConfigs[channel] = cfg;
    m_slaveIds[channel] = settings.value("slaveId", m_slaveIds.value(channel, channel + 1)).toInt();

    settings.endGroup();
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
