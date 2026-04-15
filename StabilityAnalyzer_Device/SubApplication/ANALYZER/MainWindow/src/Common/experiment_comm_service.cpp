#include "inc/Common/experiment_comm_service.h"

/**
 * @file experiment_comm_service.cpp
 * @brief ExperimentCommService 的实现。
 */

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QtMath>

namespace {
constexpr int kChannelCount = 4;

/**
 * @brief 以低字在前的顺序写入 32 位无符号整型。
 */
void appendUInt32LittleEndian(QVector<quint16>& writeData, quint32 value)
{
    writeData.append(static_cast<quint16>(value & 0xFFFF));
    writeData.append(static_cast<quint16>((value >> 16) & 0xFFFF));
}
}

ExperimentCommService::ExperimentCommService(ModbusTaskScheduler* scheduler)
    : m_scheduler(scheduler)
{
}

/**
 * @brief 初始化调度器并确保运行目录下存在配置文件。
 */
bool ExperimentCommService::initializeScheduler(const QString& configDirPath,
                                                const SerialConfigProvider& serialConfigProvider,
                                                const SlaveIdProvider& slaveIdProvider,
                                                bool* schedulerInitialized) const
{
    if (!m_scheduler || !schedulerInitialized) {
        return false;
    }

    qDebug() << "[ExperimentCommService][Init] initializeScheduler configPath=" << configDirPath;

    QDir dir(configDirPath);
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    generateDefaultConfig(configDirPath, serialConfigProvider, slaveIdProvider);

    if (!m_scheduler->loadConfigurationFromDirectory(configDirPath)) {
        qWarning() << "[ExperimentCommService] failed to load configuration from" << configDirPath;
        return false;
    }

    if (!m_scheduler->startScheduler()) {
        qWarning() << "[ExperimentCommService] failed to start scheduler";
        return false;
    }

    *schedulerInitialized = true;
    qDebug() << "[ExperimentCommService][Init] scheduler started, connectedDevices="
             << m_scheduler->connectedDevices();
    return true;
}

/**
 * @brief 生成实验设备默认配置。
 */
void ExperimentCommService::generateDefaultConfig(const QString& configDirPath,
                                                  const SerialConfigProvider& serialConfigProvider,
                                                  const SlaveIdProvider& slaveIdProvider) const
{
    const QString channelNames[] = {"ChannelA", "ChannelB", "ChannelC", "ChannelD"};
    const QString portNames[] = {"COM1", "COM2", "COM3", "COM4"};

    for (int i = 0; i < kChannelCount; ++i) {
        const QString filePath = configDirPath + "/" + channelNames[i] + ".json";
        if (QFileInfo::exists(filePath)) {
            qDebug() << "[ExperimentCommService][Config] keep existing file, skip overwrite:" << filePath;
            continue;
        }

        const SerialConfig cfg = serialConfigProvider ? serialConfigProvider(i) : SerialConfig{};

        QJsonObject serialObj;
        serialObj["portName"] = cfg.portName.isEmpty() ? portNames[i] : cfg.portName;
        serialObj["baudRate"] = cfg.baudRate;
        serialObj["dataBits"] = cfg.dataBits;
        serialObj["parity"] = cfg.parity;
        serialObj["stopBits"] = cfg.stopBits;
        serialObj["flowControl"] = cfg.flowControl;
        serialObj["interFrameDelay"] = 5;
        serialObj["responseTimeout"] = 1000;
        serialObj["maxRetries"] = 3;

        QJsonArray taskList;
        auto addTask = [&taskList](int op, const QString& name, const QString& type, int address, int count) {
            QJsonObject t;
            t["operate"] = op;
            t["name"] = name;
            t["type"] = type;
            t["address"] = address;
            t["count"] = count;
            taskList.append(t);
        };

        addTask(1, "read_start_flag", "READ_HOLDING_REGISTERS", 0, 1);
        addTask(1, "read_status_block_0_23", "READ_HOLDING_REGISTERS", 0, 24);
        addTask(1, "read_run_status", "READ_HOLDING_REGISTERS", 1, 1);
        addTask(1, "read_cover_status", "READ_HOLDING_REGISTERS", 10, 1);
        addTask(1, "read_sample_status", "READ_HOLDING_REGISTERS", 11, 1);
        addTask(1, "read_realtime_values", "READ_HOLDING_REGISTERS", 12, 2);
        addTask(1, "read_current_temperature", "READ_HOLDING_REGISTERS", 16, 1);
        addTask(1, "read_storage_status", "READ_HOLDING_REGISTERS", 20, 4);

        addTask(1, "read_scan_data_a_0", "READ_INPUT_REGISTERS", 0, 100);
        addTask(1, "read_scan_data_a_100", "READ_INPUT_REGISTERS", 100, 100);
        addTask(1, "read_scan_data_a_200", "READ_INPUT_REGISTERS", 200, 100);
        addTask(1, "read_scan_data_a_300", "READ_INPUT_REGISTERS", 300, 100);
        addTask(1, "read_scan_data_a_400", "READ_INPUT_REGISTERS", 400, 100);
        addTask(1, "read_scan_data_b_500", "READ_INPUT_REGISTERS", 500, 100);
        addTask(1, "read_scan_data_b_600", "READ_INPUT_REGISTERS", 600, 100);
        addTask(1, "read_scan_data_b_700", "READ_INPUT_REGISTERS", 700, 100);
        addTask(1, "read_scan_data_b_800", "READ_INPUT_REGISTERS", 800, 100);
        addTask(1, "read_scan_data_b_900", "READ_INPUT_REGISTERS", 900, 100);

        addTask(1, "write_start_flag", "WRITE_MULTIPLE_REGISTERS", 0, 1);
        addTask(1, "write_storage_a_state", "WRITE_MULTIPLE_REGISTERS", 22, 1);
        addTask(1, "write_storage_b_state", "WRITE_MULTIPLE_REGISTERS", 23, 1);
        addTask(1, "set_temperature_control", "WRITE_MULTIPLE_REGISTERS", 14, 1);
        addTask(1, "set_temperature", "WRITE_MULTIPLE_REGISTERS", 15, 1);
        addTask(1, "set_scan_range", "WRITE_MULTIPLE_REGISTERS", 2, 4);
        addTask(1, "set_step", "WRITE_MULTIPLE_REGISTERS", 8, 1);

        QJsonObject deviceObj;
        deviceObj["slaveId"] = slaveIdProvider ? slaveIdProvider(i) : (i + 1);
        deviceObj["name"] = channelNames[i];
        deviceObj["description"] = QString("%1 modbus device").arg(channelNames[i]);
        deviceObj["manufacturer"] = "TBD";
        deviceObj["model"] = "TBD";
        deviceObj["serialConfig"] = serialObj;
        deviceObj["taskList"] = taskList;

        QJsonObject root;
        root["device"] = deviceObj;

        QFile file(filePath);
        if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
            file.close();
            qDebug() << "[ExperimentCommService][Config] wrote" << filePath
                     << "taskCount=" << taskList.size();
        } else {
            qWarning() << "[ExperimentCommService] cannot write config file" << filePath;
        }
    }
}

/**
 * @brief 连接调度器管理的 Modbus 设备。
 */
bool ExperimentCommService::connectModbusDevice(int channel, bool* schedulerInitialized,
                                                const QString& defaultConfigPath,
                                                const SerialConfigProvider& serialConfigProvider,
                                                const SlaveIdProvider& slaveIdProvider) const
{
    if (!m_scheduler || !schedulerInitialized) {
        return false;
    }

    if (!*schedulerInitialized) {
        if (!initializeScheduler(defaultConfigPath, serialConfigProvider, slaveIdProvider, schedulerInitialized)) {
            qWarning() << "[ExperimentCommService] scheduler init failed, channel" << channel;
            return false;
        }
    }

    if (m_scheduler->isRunning()) {
        return true;
    }

    return m_scheduler->startScheduler();
}

/**
 * @brief 在没有实验运行时停止调度器。
 */
void ExperimentCommService::disconnectModbusDevice(bool schedulerInitialized,
                                                   const RunningCheck& anyChannelRunning) const
{
    if (!m_scheduler || !schedulerInitialized || !m_scheduler->isRunning()) {
        return;
    }

    if (anyChannelRunning && anyChannelRunning()) {
        return;
    }

    m_scheduler->stopScheduler();
}

/**
 * @brief 查询指定通道当前是否已连接。
 */
bool ExperimentCommService::isModbusConnected(int channel, bool schedulerInitialized,
                                              const DeviceIdProvider& deviceIdProvider) const
{
    if (!m_scheduler || !schedulerInitialized) {
        qDebug() << "[ExperimentCommService][ConnCheck] channel=" << channel
                 << "schedulerInitialized=false";
        return false;
    }

    const bool running = m_scheduler->isRunning();
    const QString deviceId = deviceIdProvider ? deviceIdProvider(channel) : QString::number(channel + 1);
    return running && (m_scheduler->getDeviceStatus(deviceId) == "Connected");
}

/**
 * @brief 发送业务控制命令。
 */
bool ExperimentCommService::sendControlCommand(int channel, const QString& command,
                                               const QVariantMap& params, bool schedulerInitialized,
                                               const DeviceIdProvider& deviceIdProvider) const
{
    if (!m_scheduler || !schedulerInitialized || !m_scheduler->isRunning()) {
        return false;
    }

    const QString deviceId = deviceIdProvider ? deviceIdProvider(channel) : QString::number(channel + 1);
    QVector<quint16> result;
    QVector<quint16> writeData;
    QString taskName;

    if (command == "set_temperature") {
        writeData.append(static_cast<quint16>(qRound(params.value("temperature", 0.0).toDouble() * 10.0)));
        taskName = "set_temperature";
    } else if (command == "set_temperature_control") {
        writeData.append(static_cast<quint16>(params.value("enabled", 0).toInt()));
        taskName = "set_temperature_control";
    } else if (command == "set_scan_range") {
        appendUInt32LittleEndian(writeData, static_cast<quint32>(qRound(params.value("start", 0.0).toDouble() * 1000.0)));
        appendUInt32LittleEndian(writeData, static_cast<quint32>(qRound(params.value("end", 0.0).toDouble() * 1000.0)));
        taskName = "set_scan_range";
    } else if (command == "set_step") {
        writeData.append(static_cast<quint16>(params.value("step", 20).toInt()));
        taskName = "set_step";
    } else if (command == "start_scan" || command == "write_start_flag") {
        writeData.append(static_cast<quint16>(params.value("value", 1).toInt()));
        taskName = "write_start_flag";
    } else if (command == "stop_scan") {
        writeData.append(0);
        taskName = "write_start_flag";
    } else if (command == "write_storage_a_state") {
        writeData.append(static_cast<quint16>(params.value("value", 3).toInt()));
        taskName = "write_storage_a_state";
    } else if (command == "write_storage_b_state") {
        writeData.append(static_cast<quint16>(params.value("value", 3).toInt()));
        taskName = "write_storage_b_state";
    } else {
        qWarning() << "[ExperimentCommService] unknown command" << command;
        return false;
    }

    m_scheduler->executeUserTask(deviceId, taskName, 1, result, writeData, command);
    qDebug() << "[ExperimentCommService][CMD] channel=" << channel
             << "command=" << command
             << "task=" << taskName
             << "writeData=" << writeData
             << "resultSize=" << result.size();
    return true;
}

/**
 * @brief 读取实时状态块并转换为首页/业务使用字段。
 */
QVariantMap ExperimentCommService::readRealtimeStatus(int channel, bool schedulerInitialized,
                                                      const DeviceIdProvider& deviceIdProvider) const
{
    QVariantMap data;
    if (!m_scheduler || !schedulerInitialized || !m_scheduler->isRunning()) {
        return data;
    }

    const QString deviceId = deviceIdProvider ? deviceIdProvider(channel) : QString::number(channel + 1);
    QVector<quint16> statusBlock;
    m_scheduler->executeUserTask(deviceId, "read_status_block_0_23", 1, statusBlock, {}, "read_status_block_0_23");
    if (statusBlock.size() < 24) {
        qWarning() << "[ExperimentCommService][READ] insufficient result, task=read_status_block_0_23";
        return {};
    }

    const int startFlag = statusBlock.value(0);
    const int runStatus = statusBlock.value(1);
    const int coverStatus = statusBlock.value(10);
    const int sampleStatus = statusBlock.value(11);

    data["startFlag"] = startFlag;
    data["runStatus"] = runStatus;
    data["running"] = (runStatus == 1 || runStatus == 2);
    data["coverStatus"] = coverStatus;
    data["isCovered"] = (coverStatus != 0);
    data["sampleStatus"] = sampleStatus;
    data["hasSample"] = (sampleStatus != 0);
    data["transmission"] = statusBlock.value(12) / 10.0;
    data["backscatter"] = statusBlock.value(13) / 10.0;
    data["currentTemperature"] = statusBlock.value(16) / 10.0;
    data["storageAReadableCount"] = statusBlock.value(20);
    data["storageBReadableCount"] = statusBlock.value(21);
    data["storageAState"] = statusBlock.value(22);
    data["storageBState"] = statusBlock.value(23);
    return data;
}

/**
 * @brief 读取首页所需的实时传感器数据。
 */
QVariantMap ExperimentCommService::readSensorData(int channel, bool schedulerInitialized,
                                                  const DeviceIdProvider& deviceIdProvider) const
{
    const QVariantMap status = readRealtimeStatus(channel, schedulerInitialized, deviceIdProvider);
    if (status.isEmpty()) {
        return {};
    }

    QVariantMap data;
    data["height"] = 0.0;
    data["backscatter_intensity"] = status.value("backscatter", 0.0).toDouble();
    data["transmission_intensity"] = status.value("transmission", 0.0).toDouble();
    data["temperature"] = status.value("currentTemperature", 0.0).toDouble();
    return data;
}
