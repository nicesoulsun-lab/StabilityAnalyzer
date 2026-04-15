#include "inc/Controller/experiment_ctrl.h"
#include "../../../SqlOrm/inc/SqlOrmManager.h"
#include "../../../TaskScheduler/inc/modbustaskscheduler.h"
#include <QDebug>
#include <QSettings>
#include <QDateTime>
#include <QtMath>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDir>
#include <QMutex>
#include <QCoreApplication>

ExperimentCtrl::ExperimentCtrl(QObject *parent)
    : QObject(parent)
    , m_dbManager(SqlOrmManager::instance())
    , m_scheduler(ModbusTaskScheduler::instance())
    , m_schedulerInitialized(false)
{
    qDebug() << "[ExperimentCtrl] 初始化中...";
    
    for (int i = 0; i < 4; ++i) {
        Channel channel = static_cast<Channel>(i);
        m_scanTimers[channel] = new QTimer();
        m_experimentTimers[channel] = new QTimer();
        m_currentScanCounts[channel] = 0;
        m_runningFlags[channel] = false;
        m_slaveIds[channel] = 1;
        
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
        
        connect(m_scanTimers[channel], &QTimer::timeout, [this, channel]() {
            onScanTimer(channel);
        });
        
        connect(m_experimentTimers[channel], &QTimer::timeout, [this, channel]() {
            onExperimentTimeout(channel);
        });
    }
    
    connect(m_scheduler, &ModbusTaskScheduler::taskCompleted,
            this, &ExperimentCtrl::onSchedulerTaskCompleted);
    
    for (int i = 0; i < 4; ++i) {
        loadSerialConfig(i);
    }
    
    qDebug() << "[ExperimentCtrl] 初始化完成";
}

ExperimentCtrl::~ExperimentCtrl()
{
    for (auto channel : {ChannelA, ChannelB, ChannelC, ChannelD}) {
        if (m_runningFlags[channel]) {
            stopExperiment(channel);
        }
        if (m_scanTimers[channel]) {
            m_scanTimers[channel]->stop();
            delete m_scanTimers[channel];
        }
        if (m_experimentTimers[channel]) {
            m_experimentTimers[channel]->stop();
            delete m_experimentTimers[channel];
        }
    }
    
    if (m_schedulerInitialized && m_scheduler->isRunning()) {
        m_scheduler->stopScheduler();
    }
    qDebug() << "[ExperimentCtrl] 析构";
}

bool ExperimentCtrl::initializeScheduler(const QString& configDirPath)
{
    QString configPath = configDirPath.isEmpty()
            ? QCoreApplication::applicationDirPath() + "/config/experiment_devices"
            : configDirPath;
    
    QDir dir(configPath);
    if (!dir.exists()) {
        dir.mkpath(".");
        qDebug() << "[ExperimentCtrl] 创建配置目录:" << configPath;
    }
    
    generateDefaultConfig(configPath);
    
    if (!m_scheduler->loadConfigurationFromDirectory(configPath)) {
        qWarning() << "[ExperimentCtrl] 加载配置文件失败，尝试使用默认配置";
        return false;
    }
    
    if (!m_scheduler->startScheduler()) {
        qWarning() << "[ExperimentCtrl] 启动任务调度器失败";
        return false;
    }
    
    m_schedulerInitialized = true;
    qDebug() << "[ExperimentCtrl] 任务调度器初始化成功，配置路径:" << configPath;
    return true;
}

void ExperimentCtrl::generateDefaultConfig(const QString& configDirPath)
{
    QString channelNames[] = {"ChannelA", "ChannelB", "ChannelC", "ChannelD"};
    QString portNames[] = {"COM1", "COM2", "COM3", "COM4"};
    
    for (int i = 0; i < 4; ++i) {
        Channel ch = static_cast<Channel>(i);
        QString filePath = configDirPath + "/" + channelNames[i] + ".json";
        
        if (QFile::exists(filePath)) {
            continue;
        }
        
        const SerialConfig& cfg = m_serialConfigs[ch];
        int slaveId = m_slaveIds[ch];
        
        QJsonObject deviceObj;
        deviceObj["deviceId"] = QString::number(slaveId);
        deviceObj["deviceName"] = channelNames[i];
        
        QJsonObject serialObj;
        serialObj["portName"] = cfg.portName.isEmpty() ? portNames[i] : cfg.portName;
        serialObj["baudRate"] = cfg.baudRate;
        serialObj["dataBits"] = cfg.dataBits;
        serialObj["parity"] = cfg.parity;
        serialObj["stopBits"] = cfg.stopBits;
        serialObj["flowControl"] = cfg.flowControl;
        deviceObj["serialConfig"] = serialObj;
        
        QJsonArray tasksArray;
        
        QJsonObject readSensorTask;
        readSensorTask["taskName"] = "read_sensor_data";
        readSensorTask["type"] = "read_input_registers";
        readSensorTask["startAddress"] = 0;
        readSensorTask["quantity"] = 6;
        readSensorTask["taskType"] = "user_task";
        tasksArray.append(readSensorTask);
        
        QJsonObject setTempTask;
        setTempTask["taskName"] = "set_temperature";
        setTempTask["type"] = "write_single_register";
        setTempTask["startAddress"] = 1;
        setTempTask["quantity"] = 1;
        setTempTask["taskType"] = "user_task";
        tasksArray.append(setTempTask);
        
        QJsonObject setScanRangeTask;
        setScanRangeTask["taskName"] = "set_scan_range";
        setScanRangeTask["type"] = "write_multiple_registers";
        setScanRangeTask["startAddress"] = 2;
        setScanRangeTask["quantity"] = 2;
        setScanRangeTask["taskType"] = "user_task";
        tasksArray.append(setScanRangeTask);
        
        QJsonObject startScanTask;
        startScanTask["taskName"] = "start_scan";
        startScanTask["type"] = "write_single_coil";
        startScanTask["startAddress"] = 1;
        startScanTask["quantity"] = 1;
        startScanTask["taskType"] = "user_task";
        tasksArray.append(startScanTask);
        
        QJsonObject stopScanTask;
        stopScanTask["taskName"] = "stop_scan";
        stopScanTask["type"] = "write_single_coil";
        stopScanTask["startAddress"] = 1;
        stopScanTask["quantity"] = 1;
        stopScanTask["taskType"] = "user_task";
        tasksArray.append(stopScanTask);
        
        QJsonObject enableTempCtrlTask;
        enableTempCtrlTask["taskName"] = "enable_temperature_control";
        enableTempCtrlTask["type"] = "write_single_coil";
        enableTempCtrlTask["startAddress"] = 2;
        enableTempCtrlTask["quantity"] = 1;
        enableTempCtrlTask["taskType"] = "user_task";
        tasksArray.append(enableTempCtrlTask);
        
        QJsonObject disableTempCtrlTask;
        disableTempCtrlTask["taskName"] = "disable_temperature_control";
        disableTempCtrlTask["type"] = "write_single_coil";
        disableTempCtrlTask["startAddress"] = 2;
        disableTempCtrlTask["quantity"] = 1;
        disableTempCtrlTask["taskType"] = "user_task";
        tasksArray.append(disableTempCtrlTask);
        
        deviceObj["tasks"] = tasksArray;
        
        QJsonDocument doc(deviceObj);
        QFile file(filePath);
        if (file.open(QIODevice::WriteOnly)) {
            file.write(doc.toJson(QJsonDocument::Indented));
            file.close();
            qDebug() << "[ExperimentCtrl] 生成默认配置文件:" << filePath;
        } else {
            qWarning() << "[ExperimentCtrl] 无法写入配置文件:" << filePath;
        }
    }
}

QString ExperimentCtrl::getDeviceId(int channel) const
{
    Channel ch = static_cast<Channel>(channel);
    return QString::number(m_slaveIds[ch]);
}

void ExperimentCtrl::saveParams(int channel, const QVariantMap& params)
{
    Channel ch = static_cast<Channel>(channel);
    qDebug()<<"[ExperimentCtrl]:"<<params;

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
    
    expParams.scanRangeStart = params.value("scanRangeStart", 0.0).toInt();
    expParams.scanRangeEnd = params.value("scanRangeEnd", 0.0).toInt();

    expParams.scanStep = params.value("scanStep", 0.0).toInt();
    
    m_params[ch] = expParams;
    
    QSettings settings("StabilityAnalyzer", "ExperimentParams");
    QString key = getChannelKey(channel);
    
    settings.beginGroup(key);
    settings.setValue("projectId", expParams.projectId);
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
    
    qDebug() << "[ExperimentCtrl] 保存参数成功，通道:" << channel;
    emit operationInfo(tr("参数保存成功"));
}

QVariantMap ExperimentCtrl::loadParams(int channel)
{
    Channel ch = static_cast<Channel>(channel);
    
    QSettings settings("StabilityAnalyzer", "ExperimentParams");
    QString key = getChannelKey(channel);
    
    settings.beginGroup(key);
    
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
    
    params["scanRangeStart"] = settings.value("scanRangeStart", 0.0);
    params["scanRangeEnd"] = settings.value("scanRangeEnd", 0.0);

    params["scanStep"] = settings.value("scanStep", 20);
    
    settings.endGroup();
    
    if (m_params.find(ch) == m_params.end()) {
        ExperimentParams expParams;
        expParams.projectId = params["projectId"].toInt();
        expParams.sampleName = params["sampleName"].toString();
        expParams.operatorName = params["operatorName"].toString();
        expParams.description = params["description"].toString();
        expParams.durationDays = params["durationDays"].toInt();
        expParams.durationHours = params["durationHours"].toInt();
        expParams.durationMinutes = params["durationMinutes"].toInt();
        expParams.durationSeconds = params["durationSeconds"].toInt();
        expParams.intervalHours = params["intervalHours"].toInt();
        expParams.intervalMinutes = params["intervalMinutes"].toInt();
        expParams.intervalSeconds = params["intervalSeconds"].toInt();
        expParams.scanCount = params["scanCount"].toInt();
        expParams.temperatureControl = params["temperatureControl"].toBool();
        expParams.targetTemperature = params["targetTemperature"].toDouble();
        expParams.scanRangeStart = params["scanRangeStart"].toInt();
        expParams.scanRangeEnd = params["scanRangeEnd"].toInt();
        expParams.scanStep = params["scanStep"].toInt();
        m_params[ch] = expParams;
    }
    
    qDebug() << "[ExperimentCtrl] 加载参数成功，通道:" << channel;
    return params;
}

bool ExperimentCtrl::startExperiment(int channel, int creatorId)
{
    Channel ch = static_cast<Channel>(channel);
    
    if (m_runningFlags[ch]) {
        qWarning() << "[ExperimentCtrl] 实验已在运行中，通道:" << channel;
        emit operationFailed("实验已在运行中");
        return false;
    }
    
    if (m_params.find(ch) == m_params.end()) {
        qWarning() << "[ExperimentCtrl] 未设置实验参数，通道:" << channel;
        emit operationFailed("请先设置实验参数");
        return false;
    }
    
    if (!m_schedulerInitialized) {
        qWarning() << "[ExperimentCtrl] 调度器未初始化，通道:" << channel;
        emit operationFailed("请先初始化通信模块");
        return false;
    }
    
    const ExperimentParams& params = m_params[ch];
    
    QVariantMap experimentData;
    experimentData["project_id"] = params.projectId;
    experimentData["sample_name"] = params.sampleName;
    experimentData["operator_name"] = params.operatorName;
    experimentData["description"] = params.description;
    experimentData["creator_id"] = creatorId;
    
    int durationSeconds = calculateTotalSeconds(params.durationDays, params.durationHours, params.durationMinutes, params.durationSeconds);
    experimentData["duration"] = durationSeconds;
    
    int intervalSeconds = calculateTotalSeconds(0, params.intervalHours, params.intervalMinutes, params.intervalSeconds);
    experimentData["interval"] = intervalSeconds;
    
    experimentData["count"] = params.scanCount;
    experimentData["temperature_control"] = params.temperatureControl;
    experimentData["target_temp"] = params.targetTemperature;
    experimentData["scan_range_start"] = params.scanRangeStart;
    experimentData["scan_range_end"] = params.scanRangeEnd;
    experimentData["scan_step"] = params.scanStep;
    experimentData["status"] = 0;
    
    int experimentId = m_dbManager->addExperiment(experimentData);
    
    if (experimentId <= 0) {
        qWarning() << "[ExperimentCtrl] 创建实验失败，通道:" << channel;
        emit operationFailed("创建实验失败");
        return false;
    }
    
    m_experimentIds[ch] = experimentId;
    m_currentScanCounts[ch] = 0;
    m_startTimes[ch] = QDateTime::currentMSecsSinceEpoch();
    m_runningFlags[ch] = true;
    
    if (intervalSeconds > 0) {
        m_scanTimers[ch]->start(intervalSeconds * 1000);
    }
    
    if (durationSeconds > 0) {
        m_experimentTimers[ch]->start(durationSeconds * 1000);
    }
    
    if (params.temperatureControl) {
        sendControlCommand(channel, "enable_temperature_control", {});
        sendControlCommand(channel, "set_temperature", {{"temperature", params.targetTemperature}});
    } else {
        sendControlCommand(channel, "disable_temperature_control", {});
    }
    
    sendControlCommand(channel, "set_scan_range", {{"start", params.scanRangeStart}, {"end", params.scanRangeEnd}});
    sendControlCommand(channel, "set_step", {{"step", params.scanStep}});
    sendControlCommand(channel, "start_scan", {});
    
    qDebug() << "[ExperimentCtrl] 实验开始，通道:" << channel << "实验ID:" << experimentId;
    emit experimentStarted(channel, experimentId);
    emit operationInfo(tr("实验开始"));

    QVariantMap logData;
    logData["username"] = "";
    logData["user_id"] = creatorId;
    logData["operation"] = QString("开始了通道%1的实验").arg(channel);
    m_dbManager->addOperationLog(logData);
    
    return true;
}

bool ExperimentCtrl::stopExperiment(int channel)
{
    Channel ch = static_cast<Channel>(channel);
    
    if (!m_runningFlags[ch]) {
        qWarning() << "[ExperimentCtrl] 实验未运行，通道:" << channel;
        return false;
    }
    
    m_scanTimers[ch]->stop();
    m_experimentTimers[ch]->stop();
    
    sendControlCommand(channel, "stop_scan", {});
    sendControlCommand(channel, "disable_temperature_control", {});
    
    int experimentId = m_experimentIds[ch];
    m_dbManager->updateExperimentStatus(experimentId, 1);
    
    m_runningFlags[ch] = false;
    
    qDebug() << "[ExperimentCtrl] 实验停止，通道:" << channel << "实验ID:" << experimentId;
    emit experimentStopped(channel, experimentId);
    emit operationInfo(tr("实验已停止"));
    
    return true;
}

bool ExperimentCtrl::isExperimentRunning(int channel) const
{
    Channel ch = static_cast<Channel>(channel);
    return m_runningFlags[ch];
}

void ExperimentCtrl::saveExperimentData(int experimentId, const QVariantMap& data)
{
    QVariantMap dataToSave = data;
    dataToSave["experiment_id"] = experimentId;
    m_dbManager->addExperimentData(dataToSave);
    qDebug() << "[ExperimentCtrl] 保存实验数据，实验ID:" << experimentId;
}

void ExperimentCtrl::batchSaveExperimentData(int experimentId, const QVector<QVariantMap>& dataList)
{
    QVector<QVariantMap> dataToSave;
    for (const auto& data : dataList) {
        QVariantMap d = data;
        d["experiment_id"] = experimentId;
        dataToSave.append(d);
    }
    m_dbManager->batchAddExperimentData(dataToSave);
    qDebug() << "[ExperimentCtrl] 批量保存实验数据，数量:" << dataList.size();
}

int ExperimentCtrl::getCurrentScanCount(int channel) const
{
    Channel ch = static_cast<Channel>(channel);
    return m_currentScanCounts[ch];
}

qint64 ExperimentCtrl::getElapsedTime(int channel) const
{
    Channel ch = static_cast<Channel>(channel);
    if (!m_runningFlags[ch]) {
        return 0;
    }
    return (QDateTime::currentMSecsSinceEpoch() - m_startTimes[ch]) / 1000;
}

void ExperimentCtrl::setSerialConfig(int channel, const QString& portName, int baudRate, int dataBits, int parity, int stopBits)
{
    Channel ch = static_cast<Channel>(channel);
    SerialConfig cfg = m_serialConfigs[ch];
    cfg.portName = portName;
    cfg.baudRate = baudRate;
    cfg.dataBits = dataBits;
    cfg.parity = (parity == 0) ? "NoParity" : (parity == 1) ? "OddParity"
                                                            : (parity == 2) ? "EvenParity" : (parity == 3) ? "SpaceParity" : "MarkParity";
    cfg.stopBits = stopBits;
    m_serialConfigs[ch] = cfg;
    qDebug() << "[ExperimentCtrl] 设置串口配置，通道:" << channel << "端口:" << portName;
}

void ExperimentCtrl::setSlaveId(int channel, int slaveId)
{
    Channel ch = static_cast<Channel>(channel);
    m_slaveIds[ch] = slaveId;
    qDebug() << "[ExperimentCtrl] 设置从站地址，通道:" << channel << "从站地址:" << slaveId;
}

bool ExperimentCtrl::connectModbusDevice(int channel)
{
    if (!m_schedulerInitialized) {
        QString configPath = QCoreApplication::applicationDirPath() + "/config/experiment_devices";
        if (!initializeScheduler(configPath)) {
            qWarning() << "[ExperimentCtrl] 调度器初始化失败，通道:" << channel;
            return false;
        }
    }
    
    if (m_scheduler->isRunning()) {
        qDebug() << "[ExperimentCtrl] 调度器已在运行中，通道:" << channel;
        return true;
    }
    
    if (!m_scheduler->startScheduler()) {
        qWarning() << "[ExperimentCtrl] 启动调度器失败，通道:" << channel;
        emit experimentError(channel, "启动Modbus调度器失败");
        return false;
    }
    
    qDebug() << "[ExperimentCtrl] Modbus设备连接成功（通过调度器），通道:" << channel;
    return true;
}

void ExperimentCtrl::disconnectModbusDevice(int channel)
{
    Q_UNUSED(channel)
    
    if (m_schedulerInitialized && m_scheduler->isRunning()) {
        bool anyRunning = false;
        for (auto flag : m_runningFlags) {
            if (flag) {
                anyRunning = true;
                break;
            }
        }
        
        if (!anyRunning) {
            m_scheduler->stopScheduler();
            qDebug() << "[ExperimentCtrl] Modbus设备已断开（停止调度器）";
        }
    }
}

bool ExperimentCtrl::isModbusConnected(int channel) const
{
    Q_UNUSED(channel)
    
    if (!m_schedulerInitialized) {
        return false;
    }
    return m_scheduler->isRunning() && m_scheduler->connectedDevices() > 0;
}

void ExperimentCtrl::saveSerialConfig(int channel)
{
    Channel ch = static_cast<Channel>(channel);
    
    QSettings settings("StabilityAnalyzer", "ModbusConfig");
    QString key = getChannelKey(channel);
    
    settings.beginGroup(key);
    
    const SerialConfig& cfg = m_serialConfigs[ch];
    settings.setValue("portName", cfg.portName);
    settings.setValue("baudRate", cfg.baudRate);
    settings.setValue("dataBits", cfg.dataBits);
    settings.setValue("parity", cfg.parity);
    settings.setValue("stopBits", cfg.stopBits);
    settings.setValue("slaveId", m_slaveIds[ch]);
    
    settings.endGroup();
    
    qDebug() << "[ExperimentCtrl] 保存串口配置，通道:" << channel;
}

void ExperimentCtrl::loadSerialConfig(int channel)
{
    Channel ch = static_cast<Channel>(channel);
    
    QSettings settings("StabilityAnalyzer", "ModbusConfig");
    QString key = getChannelKey(channel);
    
    settings.beginGroup(key);
    
    SerialConfig cfg = m_serialConfigs[ch];
    cfg.portName = settings.value("portName", cfg.portName).toString();
    cfg.baudRate = settings.value("baudRate", cfg.baudRate).toInt();
    cfg.dataBits = settings.value("dataBits", cfg.dataBits).toInt();
    cfg.parity = settings.value("parity", cfg.parity).toString();
    cfg.stopBits = settings.value("stopBits", cfg.stopBits).toInt();
    m_serialConfigs[ch] = cfg;
    m_slaveIds[ch] = settings.value("slaveId", m_slaveIds[ch]).toInt();
    
    settings.endGroup();
    
    qDebug() << "[ExperimentCtrl] 加载串口配置，通道:" << channel;
}

void ExperimentCtrl::onScanTimer(int channel)
{
    Channel ch = static_cast<Channel>(channel);
    
    if (!m_runningFlags[ch]) {
        return;
    }
    
    m_currentScanCounts[ch]++;
    
    QVariantMap sensorData = readSensorData(channel);
    
    if (!sensorData.isEmpty()) {
        int timestamp = static_cast<int>(QDateTime::currentMSecsSinceEpoch() / 1000);
        sensorData["timestamp"] = timestamp;
        
        saveExperimentData(m_experimentIds[ch], sensorData);
        
        emit scanCompleted(channel, m_currentScanCounts[ch], sensorData);
    }
    
    const ExperimentParams& params = m_params[ch];
    if (params.scanCount > 0 && m_currentScanCounts[ch] >= params.scanCount) {
        stopExperiment(channel);
    }
}

void ExperimentCtrl::onExperimentTimeout(int channel)
{
    qDebug() << "[ExperimentCtrl] 实验持续时间到，通道:" << channel;
    stopExperiment(channel);
}

int ExperimentCtrl::calculateTotalSeconds(int days, int hours, int minutes, int seconds) const
{
    return days * 86400 + hours * 3600 + minutes * 60 + seconds;
}

QString ExperimentCtrl::getChannelKey(int channel) const
{
    switch (channel) {
    case 0: return "ChannelA";
    case 1: return "ChannelB";
    case 2: return "ChannelC";
    case 3: return "ChannelD";
    default: return "Unknown";
    }
}

bool ExperimentCtrl::sendControlCommand(int channel, const QString& command, const QVariantMap& params)
{
    Channel ch = static_cast<Channel>(channel);
    
    if (!m_schedulerInitialized || !m_scheduler->isRunning()) {
        qWarning() << "[ExperimentCtrl] 调度器未运行，通道:" << channel;
        return false;
    }
    
    QString deviceId = getDeviceId(channel);
    QVector<quint16> result;
    QVector<quint16> writeData;
    
    qDebug() << "[ExperimentCtrl] 发送控制命令，通道:" << channel << "命令:" << command << "参数:" << params;
    
    if (command == "set_temperature") {
        double temperature = params.value("temperature", 0.0).toDouble();
        quint16 tempValue = static_cast<quint16>(qRound(temperature * 10));
        writeData.append(tempValue);
        
        m_scheduler->executeUserTask(deviceId, "set_temperature", 1, result, writeData, command);
        qDebug() << "[ExperimentCtrl] 设置温度成功，通道:" << channel << "温度:" << temperature;
        return true;
    }
    else if (command == "set_scan_range") {
        double start = params.value("start", 0.0).toDouble();
        double end = params.value("end", 0.0).toDouble();
        
        writeData.append(static_cast<quint16>(qRound(start * 10)));
        writeData.append(static_cast<quint16>(qRound(end * 10)));
        
        m_scheduler->executeUserTask(deviceId, "set_scan_range", 1, result, writeData, command);
        qDebug() << "[ExperimentCtrl] 设置扫描区间成功，通道:" << channel << "开始:" << start << "结束:" << end;
        return true;
    }
    else if (command == "start_scan") {
        writeData.append(0xFF00);
        
        m_scheduler->executeUserTask(deviceId, "start_scan", 1, result, writeData, command);
        qDebug() << "[ExperimentCtrl] 启动扫描成功，通道:" << channel;
        return true;
    }
    else if (command == "stop_scan") {
        writeData.append(0x0000);
        
        m_scheduler->executeUserTask(deviceId, "stop_scan", 1, result, writeData, command);
        qDebug() << "[ExperimentCtrl] 停止扫描成功，通道:" << channel;
        return true;
    }
    else if (command == "enable_temperature_control") {
        writeData.append(0xFF00);
        
        m_scheduler->executeUserTask(deviceId, "enable_temperature_control", 1, result, writeData, command);
        qDebug() << "[ExperimentCtrl] 启用温度控制成功，通道:" << channel;
        return true;
    }
    else if (command == "disable_temperature_control") {
        writeData.append(0x0000);
        
        m_scheduler->executeUserTask(deviceId, "disable_temperature_control", 1, result, writeData, command);
        qDebug() << "[ExperimentCtrl] 禁用温度控制成功，通道:" << channel;
        return true;
    }
    
    qWarning() << "[ExperimentCtrl] 未知命令:" << command;
    return false;
}

struct SensorReadContext {
    int channel;
    bool completed;
    QVector<quint16> data;
};

static QHash<QString, SensorReadContext> g_sensorReadContexts;
static QMutex g_sensorMutex;

QVariantMap ExperimentCtrl::readSensorData(int channel)
{
    Channel ch = static_cast<Channel>(channel);
    QVariantMap data;
    
    if (!m_schedulerInitialized || !m_scheduler->isRunning()) {
        qWarning() << "[ExperimentCtrl] 调度器未运行，无法读取传感器数据，通道:" << channel;
        return data;
    }
    
    QString deviceId = getDeviceId(channel);
    QString contextKey = QString("sensor_read_%1_%2")
            .arg(channel)
            .arg(QDateTime::currentMSecsSinceEpoch());
    
    QVector<quint16> result;
    m_scheduler->executeUserTask(deviceId, "read_sensor_data", 1, result, {}, contextKey);
    
    if (result.size() >= 6) {
        qint16 heightRaw = static_cast<qint16>(result[0]);
        qint16 backscatterRaw = static_cast<qint16>(result[1]);
        qint16 transmissionRaw = static_cast<qint16>(result[2]);
        qint16 temperatureRaw = static_cast<qint16>(result[3]);
        qint16 pressureRaw = static_cast<qint16>(result[4]);
        qint16 humidityRaw = static_cast<qint16>(result[5]);
        
        data["height"] = heightRaw / 10.0;
        data["backscatter_intensity"] = backscatterRaw / 10.0;
        data["transmission_intensity"] = transmissionRaw / 10.0;
        data["temperature"] = temperatureRaw / 10.0;
        data["pressure"] = pressureRaw / 10.0;
        data["humidity"] = humidityRaw / 10.0;
        
        qDebug() << "[ExperimentCtrl] 读取传感器数据成功，通道:" << channel << "数据:" << data;
    } else {
        qWarning() << "[ExperimentCtrl] 读取传感器数据失败，返回数据量不足，通道:" << channel << "数量:" << result.size();
    }
    
    return data;
}

void ExperimentCtrl::onSchedulerTaskCompleted(TaskResult res, QVector<quint16> data)
{
    qDebug() << "[ExperimentCtrl] 收到调度器任务完成通知:"
             << "设备:" << res.deviceId
             << "任务:" << res.taskName
             << "备注:" << res.remark
             << "异常:" << res.isException
             << "数据量:" << data.size();
}
