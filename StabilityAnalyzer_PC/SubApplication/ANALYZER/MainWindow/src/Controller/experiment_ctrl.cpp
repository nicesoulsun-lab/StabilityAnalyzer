#include "inc/Controller/experiment_ctrl.h"

#include "../../../SqlOrm/inc/SqlOrmManager.h"
#include "../../../DataTransmit/inc/Controller/datatransmitcontroller.h"
#include "../../../TaskScheduler/inc/modbustaskscheduler.h"
//#include "datatransmitcontroller.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QEventLoop>
#include <QSettings>
#include <QTimer>
#include <QUuid>
#include <QDebug>

namespace {

constexpr int kChannelCount = 4;

bool isValidChannelIndex(int channel)
{
    return channel >= 0 && channel < kChannelCount;
}

bool isChannelRunningOnDevice(DataTransmitController *controller, int channel)
{
    if (!controller || !isValidChannelIndex(channel)) {
        return false;
    }

    const QVariantList experimentChannels = controller->experimentChannels();
    if (channel < 0 || channel >= experimentChannels.size()) {
        return false;
    }

    return experimentChannels.at(channel).toMap().value(QStringLiteral("running")).toBool();
}

int findLatestExperimentId(SqlOrmManager *dbManager)
{
    if (!dbManager) {
        return 0;
    }

    int maxId = 0;
    const QVector<QVariantMap> experiments = dbManager->getAllExperiments();
    for (const QVariantMap &experiment : experiments) {
        maxId = qMax(maxId, experiment.value(QStringLiteral("id")).toInt());
    }
    return maxId;
}

}

ExperimentCtrl::ExperimentCtrl(QObject *parent)
    : QObject(parent)
    , m_dbManager(SqlOrmManager::instance())
    , m_scheduler(ModbusTaskScheduler::instance())
{
    for (int i = 0; i < kChannelCount; ++i) {
        const Channel channel = static_cast<Channel>(i);
        m_scanTimers[channel] = new QTimer(this);
        m_experimentTimers[channel] = new QTimer(this);
        m_experimentIds[channel] = 0;
        m_currentScanCounts[channel] = 0;
        m_startTimes[channel] = 0;
        m_runningFlags[channel] = false;
        m_slaveIds[channel] = i + 1;

        SerialConfig serialCfg;
        switch (channel) {
        case ChannelA: serialCfg.portName = "COM1"; break;
        case ChannelB: serialCfg.portName = "COM2"; break;
        case ChannelC: serialCfg.portName = "COM3"; break;
        case ChannelD: serialCfg.portName = "COM4"; break;
        }
        serialCfg.baudRate = 9600;
        serialCfg.dataBits = 8;
        serialCfg.parity = "NoParity";
        serialCfg.stopBits = 1;
        serialCfg.flowControl = "NoFlowControl";
        m_serialConfigs[channel] = serialCfg;

        connect(m_scanTimers[channel], &QTimer::timeout, this, [this, channel]() {
            onScanTimer(channel);
        });
        connect(m_experimentTimers[channel], &QTimer::timeout, this, [this, channel]() {
            onExperimentTimeout(channel);
        });
    }

    for (int i = 0; i < kChannelCount; ++i) {
        loadSerialConfig(i);
    }
}

ExperimentCtrl::~ExperimentCtrl()
{
    for (int i = 0; i < kChannelCount; ++i) {
        const Channel channel = static_cast<Channel>(i);
        if (m_scanTimers.value(channel)) {
            m_scanTimers[channel]->stop();
        }
        if (m_experimentTimers.value(channel)) {
            m_experimentTimers[channel]->stop();
        }
    }
}

void ExperimentCtrl::setDataTransmitController(DataTransmitController *controller)
{
    m_dataTransmitCtrl = controller;
    if (m_dataTransmitCtrl) {
        connect(m_dataTransmitCtrl, &DataTransmitController::experimentChannelsChanged,
                this, &ExperimentCtrl::syncExperimentChannelsFromDevice, Qt::UniqueConnection);
        syncExperimentChannelsFromDevice();
    }
}

void ExperimentCtrl::saveParams(int channel, const QVariantMap &params)
{
    if (!isValidChannelIndex(channel)) {
        emit operationFailed(tr("无效的实验通道"));
        return;
    }

    const Channel ch = static_cast<Channel>(channel);
    const ExperimentParams expParams = paramsFromVariantMap(params);
    m_params[ch] = expParams;

    QSettings settings("StabilityAnalyzer", "ExperimentParams");
    settings.beginGroup(getChannelKey(channel));
    settings.setValue("projectId", expParams.projectId);
    settings.setValue("projectName", expParams.projectName);
    settings.setValue("sampleName", expParams.sampleName);
    settings.setValue("operatorName", expParams.operatorName);
    settings.setValue("description", expParams.description);
    settings.setValue("durationDays", expParams.durationDays);
    settings.setValue("durationHours", expParams.durationHours);
    settings.setValue("durationMinutes", expParams.durationMinutes);
    settings.setValue("durationSeconds", expParams.durationSeconds);
    settings.setValue("intervalHours", expParams.intervalHours);
    settings.setValue("intervalMinutes", expParams.intervalMinutes);
    settings.setValue("intervalSeconds", expParams.intervalSeconds);
    settings.setValue("scanCount", expParams.scanCount);
    settings.setValue("temperatureControl", expParams.temperatureControl);
    settings.setValue("targetTemperature", expParams.targetTemperature);
    settings.setValue("scanRangeStart", expParams.scanRangeStart);
    settings.setValue("scanRangeEnd", expParams.scanRangeEnd);
    settings.setValue("scanStep", expParams.scanStep);
    settings.endGroup();

    //emit operationInfo(tr("参数保存成功"));

    qDebug()<<"参数保存成功";
}

QVariantMap ExperimentCtrl::loadParams(int channel)
{
    if (!isValidChannelIndex(channel)) {
        return QVariantMap();
    }

    QSettings settings("StabilityAnalyzer", "ExperimentParams");
    settings.beginGroup(getChannelKey(channel));

    QVariantMap params;
    params.insert("projectId", settings.value("projectId", 0));
    params.insert("projectName", settings.value("projectName", ""));
    params.insert("sampleName", settings.value("sampleName", ""));
    params.insert("operatorName", settings.value("operatorName", ""));
    params.insert("description", settings.value("description", ""));
    params.insert("durationDays", settings.value("durationDays", 0));
    params.insert("durationHours", settings.value("durationHours", 0));
    params.insert("durationMinutes", settings.value("durationMinutes", 0));
    params.insert("durationSeconds", settings.value("durationSeconds", 0));
    params.insert("intervalHours", settings.value("intervalHours", 0));
    params.insert("intervalMinutes", settings.value("intervalMinutes", 0));
    params.insert("intervalSeconds", settings.value("intervalSeconds", 0));
    params.insert("scanCount", settings.value("scanCount", 0));
    params.insert("temperatureControl", settings.value("temperatureControl", false));
    params.insert("targetTemperature", settings.value("targetTemperature", 0.0));
    params.insert("scanRangeStart", settings.value("scanRangeStart", 0));
    params.insert("scanRangeEnd", settings.value("scanRangeEnd", 55));
    params.insert("scanStep", settings.value("scanStep", 20));
    settings.endGroup();

    const Channel ch = static_cast<Channel>(channel);
    if (!m_params.contains(ch)) {
        m_params[ch] = paramsFromVariantMap(params);
    }

    return params;
}

bool ExperimentCtrl::startExperiment(int channel, int creatorId)
{
    if (!isValidChannelIndex(channel)) {
        emit operationFailed(tr("无效的实验通道"));
        return false;
    }

    const Channel ch = static_cast<Channel>(channel);
    if (isChannelRunningOnDevice(m_dataTransmitCtrl, channel)) {
        emit operationFailed(tr("实验已在运行中"));
        return false;
    }

    if (!m_params.contains(ch)) {
        loadParams(channel);
    }
    if (!m_params.contains(ch)) {
        emit operationFailed(tr("请先设置实验参数"));
        return false;
    }

    if (!m_dataTransmitCtrl || m_dataTransmitCtrl->deviceUiConnectionStateText() != QStringLiteral("Connected")) {
        emit operationFailed(tr("设备未连接，请检查连接状态"));
        return false;
    }

    const ExperimentParams params = m_params.value(ch);
    QVariantMap response;
    QVariantMap request;
    request.insert("channel", channel);
    request.insert("creator_id", creatorId);
    request.insert("params", QVariantMap{
                       {"projectId", params.projectId},
                       {"projectName", params.projectName},
                       {"sampleName", params.sampleName},
                       {"operatorName", params.operatorName},
                       {"description", params.description},
                       {"durationDays", params.durationDays},
                       {"durationHours", params.durationHours},
                       {"durationMinutes", params.durationMinutes},
                       {"durationSeconds", params.durationSeconds},
                       {"intervalHours", params.intervalHours},
                       {"intervalMinutes", params.intervalMinutes},
                       {"intervalSeconds", params.intervalSeconds},
                       {"scanCount", params.scanCount},
                       {"temperatureControl", params.temperatureControl},
                       {"targetTemperature", params.targetTemperature},
                       {"scanRangeStart", params.scanRangeStart},
                       {"scanRangeEnd", params.scanRangeEnd},
                       {"scanStep", params.scanStep}
                   });

    if (!sendRequestAndWait(QStringLiteral("start_experiment"), request, &response, 60000)) {
        const QString message = response.value("message").toString();
        emit operationFailed(message.isEmpty() ? tr("启动实验失败") : message);
        return false;
    }

    const int deviceExperimentId = response.value(QStringLiteral("experiment_id"), 0).toInt();
    QVariantMap experimentData = buildExperimentData(params, creatorId);
    if (deviceExperimentId > 0) {
        experimentData.insert(QStringLiteral("id"), deviceExperimentId);
    }

    const int previousLatestExperimentId = findLatestExperimentId(m_dbManager);
    const bool added = m_dbManager ? m_dbManager->addExperiment(experimentData) : false;
    int experimentId = deviceExperimentId;
    if (!added) {
        emit operationFailed(tr("创建本地实验记录失败"));
        return false;
    }
    if (experimentId <= 0 || experimentId <= previousLatestExperimentId) {
        experimentId = findLatestExperimentId(m_dbManager);
    }
    if (experimentId <= 0) {
        emit operationFailed(tr("创建实验失败"));
        return false;
    }

    m_experimentIds[ch] = experimentId;
    m_currentScanCounts[ch] = 0;
    m_startTimes[ch] = QDateTime::currentMSecsSinceEpoch();
    m_runningFlags[ch] = true;

    emit experimentStarted(channel, experimentId);
    emit operationInfo(tr("实验开始"));
    return true;
}

bool ExperimentCtrl::stopExperiment(int channel)
{
    if (!isValidChannelIndex(channel)) {
        emit operationFailed(tr("无效的实验通道"));
        return false;
    }

    const Channel ch = static_cast<Channel>(channel);
    if (!isChannelRunningOnDevice(m_dataTransmitCtrl, channel)) {
        return false;
    }

    QVariantMap response;
    if (!sendRequestAndWait(QStringLiteral("stop_experiment"), {{"channel", channel}}, &response)) {
        const QString message = response.value("message").toString();
        emit operationFailed(message.isEmpty() ? tr("停止实验失败") : message);
        return false;
    }

    if (m_scanTimers.value(ch)) {
        m_scanTimers[ch]->stop();
    }
    if (m_experimentTimers.value(ch)) {
        m_experimentTimers[ch]->stop();
    }

    const int experimentId = m_experimentIds.value(ch, 0);
    if (experimentId > 0 && m_dbManager) {
        m_dbManager->updateExperimentStatus(experimentId, 1);
    }

    m_runningFlags[ch] = false;
    m_currentScanCounts[ch] = 0;
    m_startTimes[ch] = 0;
    m_experimentIds[ch] = 0;

    emit experimentStopped(channel, experimentId);
    emit operationInfo(tr("实验已停止"));
    return true;
}

void ExperimentCtrl::syncExperimentChannelsFromDevice()
{
    if (!m_dataTransmitCtrl
            || m_dataTransmitCtrl->deviceUiConnectionState() != DataTransmitController::DeviceConnected) {
        return;
    }

    const QVariantList channels = m_dataTransmitCtrl->experimentChannels();
    for (int channelIndex = 0; channelIndex < channels.size() && channelIndex < kChannelCount; ++channelIndex) {
        const Channel channel = static_cast<Channel>(channelIndex);
        const QVariantMap channelStatus = channels.at(channelIndex).toMap();
        const bool running = channelStatus.value(QStringLiteral("running")).toBool();
        const int reportedExperimentId = channelStatus.value(QStringLiteral("experiment_id")).toInt();

        if (running) {
            m_runningFlags[channel] = true;
            if (reportedExperimentId > 0) {
                m_experimentIds[channel] = reportedExperimentId;
            }
            continue;
        }

        const int experimentId = reportedExperimentId > 0
                ? reportedExperimentId
                : m_experimentIds.value(channel, 0);
        if (m_runningFlags.value(channel, false) || experimentId > 0) {
            finalizeStoppedChannel(channel, channelIndex, experimentId, experimentId > 0);
        }
    }
}

void ExperimentCtrl::finalizeStoppedChannel(Channel channel, int channelIndex, int experimentId, bool emitSignal)
{
    if (m_scanTimers.value(channel)) {
        m_scanTimers[channel]->stop();
    }
    if (m_experimentTimers.value(channel)) {
        m_experimentTimers[channel]->stop();
    }

    if (experimentId > 0 && m_dbManager) {
        m_dbManager->updateExperimentStatus(experimentId, 1);
    }

    m_runningFlags[channel] = false;
    m_currentScanCounts[channel] = 0;
    m_startTimes[channel] = 0;
    m_experimentIds[channel] = 0;

    qDebug() << "[ExperimentCtrl] finalized stopped experiment"
             << "channel=" << channelIndex
             << "experimentId=" << experimentId
             << "emitSignal=" << emitSignal;

    if (emitSignal) {
        emit experimentStopped(channelIndex, experimentId);
    }
}

bool ExperimentCtrl::isExperimentRunning(int channel) const
{
    if (!isValidChannelIndex(channel)) {
        return false;
    }
    return isChannelRunningOnDevice(m_dataTransmitCtrl, channel);
}

int ExperimentCtrl::getCurrentScanCount(int channel) const
{
    if (!isValidChannelIndex(channel)) {
        return 0;
    }
    return m_currentScanCounts.value(static_cast<Channel>(channel), 0);
}

int ExperimentCtrl::getCurrentExperimentId(int channel) const
{
    if (!isValidChannelIndex(channel)) {
        return 0;
    }
    return m_experimentIds.value(static_cast<Channel>(channel), 0);
}

qint64 ExperimentCtrl::getElapsedTime(int channel) const
{
    if (!isValidChannelIndex(channel)) {
        return 0;
    }

    if (!isChannelRunningOnDevice(m_dataTransmitCtrl, channel)) {
        return 0;
    }

    return (QDateTime::currentMSecsSinceEpoch()
            - m_startTimes.value(static_cast<Channel>(channel), 0)) / 1000;
}

void ExperimentCtrl::setSerialConfig(int channel, const QString &portName, int baudRate, int dataBits, int parity, int stopBits)
{
    if (!isValidChannelIndex(channel)) {
        return;
    }

    const Channel ch = static_cast<Channel>(channel);
    SerialConfig cfg = m_serialConfigs.value(ch);
    cfg.portName = portName;
    cfg.baudRate = baudRate;
    cfg.dataBits = dataBits;
    cfg.parity = (parity == 0) ? "NoParity"
               : (parity == 1) ? "OddParity"
               : (parity == 2) ? "EvenParity"
               : (parity == 3) ? "SpaceParity"
                               : "MarkParity";
    cfg.stopBits = stopBits;
    m_serialConfigs[ch] = cfg;
}

void ExperimentCtrl::setSlaveId(int channel, int slaveId)
{
    if (!isValidChannelIndex(channel)) {
        return;
    }
    m_slaveIds[static_cast<Channel>(channel)] = slaveId;
}

bool ExperimentCtrl::initializeScheduler(const QString &configDirPath)
{
    Q_UNUSED(configDirPath)
    // PC 侧实验控制已切到 Device 通信链路，这里保留接口兼容旧 QML/C++ 调用。
    return m_dataTransmitCtrl && m_dataTransmitCtrl->deviceUiConnectionStateText() == QStringLiteral("Connected");
}

bool ExperimentCtrl::connectModbusDevice(int channel)
{
    Q_UNUSED(channel)
    return initializeScheduler(QString());
}

void ExperimentCtrl::disconnectModbusDevice(int channel)
{
    Q_UNUSED(channel)
}

bool ExperimentCtrl::isModbusConnected(int channel) const
{
    Q_UNUSED(channel)
    return m_dataTransmitCtrl && m_dataTransmitCtrl->deviceUiConnectionStateText() == QStringLiteral("Connected");
}

void ExperimentCtrl::saveSerialConfig(int channel)
{
    if (!isValidChannelIndex(channel)) {
        return;
    }

    const Channel ch = static_cast<Channel>(channel);
    const SerialConfig cfg = m_serialConfigs.value(ch);
    QSettings settings("StabilityAnalyzer", "ModbusConfig");
    settings.beginGroup(getChannelKey(channel));
    settings.setValue("portName", cfg.portName);
    settings.setValue("baudRate", cfg.baudRate);
    settings.setValue("dataBits", cfg.dataBits);
    settings.setValue("parity", cfg.parity);
    settings.setValue("stopBits", cfg.stopBits);
    settings.setValue("slaveId", m_slaveIds.value(ch, channel + 1));
    settings.endGroup();
}

void ExperimentCtrl::loadSerialConfig(int channel)
{
    if (!isValidChannelIndex(channel)) {
        return;
    }

    const Channel ch = static_cast<Channel>(channel);
    SerialConfig cfg = m_serialConfigs.value(ch);
    QSettings settings("StabilityAnalyzer", "ModbusConfig");
    settings.beginGroup(getChannelKey(channel));
    cfg.portName = settings.value("portName", cfg.portName).toString();
    cfg.baudRate = settings.value("baudRate", cfg.baudRate).toInt();
    cfg.dataBits = settings.value("dataBits", cfg.dataBits).toInt();
    cfg.parity = settings.value("parity", cfg.parity).toString();
    cfg.stopBits = settings.value("stopBits", cfg.stopBits).toInt();
    m_slaveIds[ch] = settings.value("slaveId", channel + 1).toInt();
    settings.endGroup();
    m_serialConfigs[ch] = cfg;
}

void ExperimentCtrl::onScanTimer(int channel)
{
    Q_UNUSED(channel)
}

void ExperimentCtrl::onExperimentTimeout(int channel)
{
    Q_UNUSED(channel)
    // 实验持续时间和结束时机由 Device 端统一控制，PC 不再因本地定时器主动停实验。
}

void ExperimentCtrl::onSchedulerTaskCompleted(TaskResult res, QVector<quint16> data)
{
    Q_UNUSED(res)
    Q_UNUSED(data)
}

int ExperimentCtrl::calculateTotalSeconds(int days, int hours, int minutes, int seconds) const
{
    return days * 86400 + hours * 3600 + minutes * 60 + seconds;
}

QString ExperimentCtrl::getChannelKey(int channel) const
{
    switch (channel) {
    case 0: return QStringLiteral("ChannelA");
    case 1: return QStringLiteral("ChannelB");
    case 2: return QStringLiteral("ChannelC");
    case 3: return QStringLiteral("ChannelD");
    default: return QStringLiteral("Unknown");
    }
}

QString ExperimentCtrl::getDeviceId(int channel) const
{
    return QString::number(m_slaveIds.value(static_cast<Channel>(channel), channel + 1));
}

bool ExperimentCtrl::sendControlCommand(int channel, const QString &command, const QVariantMap &params)
{
    QVariantMap payload = params;
    payload.insert("channel", channel);
    return sendRequestAndWait(command, payload, nullptr);
}

QVariantMap ExperimentCtrl::readSensorData(int channel)
{
    Q_UNUSED(channel)
    return QVariantMap();
}

void ExperimentCtrl::generateDefaultConfig(const QString &configDirPath)
{
    Q_UNUSED(configDirPath)
}

bool ExperimentCtrl::sendRequestAndWait(const QString &command,
                                        const QVariantMap &payload,
                                        QVariantMap *response,
                                        int timeoutMs)
{
    if (response) {
        response->clear();
    }

    if (!m_dataTransmitCtrl) {
        return false;
    }

    const QString requestId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    QVariantMap request = payload;
    request.insert("request_id", requestId);

    QVariantMap replyPayload;
    bool finished = false;
    bool success = false;

    QEventLoop loop;
    QTimer timeoutTimer;
    timeoutTimer.setSingleShot(true);

    QMetaObject::Connection controlConn;
    QMetaObject::Connection timeoutConn;

    controlConn = connect(m_dataTransmitCtrl, &DataTransmitController::controlMessageReceived,
                          &loop, [&](const QVariantMap &message) {
        if (message.value("request_id").toString() != requestId) {
            return;
        }

        if (message.value("type").toString() != QStringLiteral("command_result")) {
            return;
        }

        replyPayload = message;
        success = message.value("success").toBool();
        finished = true;
        loop.quit();
    });

    timeoutConn = connect(&timeoutTimer, &QTimer::timeout, &loop, [&]() {
        replyPayload.insert("message", tr("等待设备响应超时"));
        finished = true;
        success = false;
        loop.quit();
    });

    const bool sendOk = m_dataTransmitCtrl->sendControlCommand(command, request);
    if (!sendOk) {
        disconnect(controlConn);
        disconnect(timeoutConn);
        replyPayload.insert("message", m_dataTransmitCtrl->lastError());
        if (response) {
            *response = replyPayload;
        }
        return false;
    }

    timeoutTimer.start(timeoutMs);
    loop.exec();

    disconnect(controlConn);
    disconnect(timeoutConn);

    if (!finished) {
        replyPayload.insert("message", tr("等待设备响应超时"));
        success = false;
    }

    if (response) {
        *response = replyPayload;
    }
    return success;
}

QVariantMap ExperimentCtrl::buildExperimentData(const ExperimentParams &params, int creatorId) const
{
    QVariantMap experimentData;
    experimentData.insert("project_id", params.projectId);
    experimentData.insert("sample_name", params.sampleName);
    experimentData.insert("operator_name", params.operatorName);
    experimentData.insert("description", params.description);
    experimentData.insert("creator_id", creatorId);
    experimentData.insert("duration", calculateTotalSeconds(params.durationDays,
                                                            params.durationHours,
                                                            params.durationMinutes,
                                                            params.durationSeconds));
    experimentData.insert("interval", calculateTotalSeconds(0,
                                                            params.intervalHours,
                                                            params.intervalMinutes,
                                                            params.intervalSeconds));
    experimentData.insert("count", params.scanCount);
    experimentData.insert("temperature_control", params.temperatureControl);
    experimentData.insert("target_temp", params.targetTemperature);
    experimentData.insert("scan_range_start", params.scanRangeStart);
    experimentData.insert("scan_range_end", params.scanRangeEnd);
    experimentData.insert("scan_step", params.scanStep);
    experimentData.insert("status", 0);
    return experimentData;
}

ExperimentCtrl::ExperimentParams ExperimentCtrl::paramsFromVariantMap(const QVariantMap &params) const
{
    ExperimentParams expParams;
    expParams.projectId = params.value("projectId", 0).toInt();
    expParams.projectName = params.value("projectName").toString();
    expParams.sampleName = params.value("sampleName").toString();
    expParams.operatorName = params.value("operatorName").toString();
    expParams.description = params.value("description").toString();
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
    expParams.scanRangeEnd = params.value("scanRangeEnd", 55).toInt();
    expParams.scanStep = params.value("scanStep", 20).toInt();
    return expParams;
}
