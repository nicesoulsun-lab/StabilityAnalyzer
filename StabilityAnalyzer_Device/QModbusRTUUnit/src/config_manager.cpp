#include "inc/config_manager.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>
#include "logmanager.h"
ConfigManager::ConfigManager(QObject* parent)
    : QObject(parent)
{
    // 设置默认系统配置
    m_systemConfig = defaultSystemConfig();
}

bool ConfigManager::loadFromFile(const QString& filename)
{
    QMutexLocker locker(&m_mutex);
    
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        LOG_INFO() << "Failed to open config file:" << filename;
        return false;
    }
    
    QByteArray data = file.readAll();
    file.close();
    
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
    
    if (parseError.error != QJsonParseError::NoError) {
        LOG_INFO() << "Failed to parse config file:" << filename
                  << "Error:" << parseError.errorString();
        return false;
    }
    
    if (!doc.isObject()) {
        LOG_INFO() << "Invalid config format in file:" << filename;
        return false;
    }
    
    bool success = loadFromJson(doc.object());
    if (success) {
        emit configLoaded();
        emit configChanged();
    }
    
    return success;
}

bool ConfigManager::saveToFile(const QString& filename)
{
    QMutexLocker locker(&m_mutex);
    
    QJsonDocument doc(toJson());
    QByteArray data = doc.toJson(QJsonDocument::Indented);
    
    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        LOG_INFO() << "Failed to open config file for writing:" << filename;
        return false;
    }
    
    qint64 bytesWritten = file.write(data);
    file.close();
    
    bool success = (bytesWritten == data.size());
    if (success) {
        emit configSaved();
    } else {
        LOG_INFO() << "Failed to write config file:" << filename;
    }
    
    return success;
}

bool ConfigManager::loadFromJson(const QJsonObject& json)
{
    
    // 清空当前配置
    m_devices.clear();
    
    // 加载系统配置
    if (json.contains("system") && json["system"].isObject()) {
        QJsonObject systemObj = json["system"].toObject();
        
        if (systemObj.contains("autoStart") && systemObj["autoStart"].isBool()) {
            m_systemConfig.autoStart = systemObj["autoStart"].toBool();
        }
        
        if (systemObj.contains("autoReconnect") && systemObj["autoReconnect"].isBool()) {
            m_systemConfig.autoReconnect = systemObj["autoReconnect"].toBool();
        }
        
        if (systemObj.contains("monitorInterval") && systemObj["monitorInterval"].isDouble()) {
            m_systemConfig.monitorInterval = systemObj["monitorInterval"].toInt();
        }
        
        // 加载客户端配置
        if (systemObj.contains("client") && systemObj["client"].isObject()) {
            QJsonObject clientObj = systemObj["client"].toObject();
            
            if (clientObj.contains("workerThreadCount") && clientObj["workerThreadCount"].isDouble()) {
//                m_systemConfig.clientConfig.workerThreadCount = clientObj["workerThreadCount"].toInt();
            }
            
            if (clientObj.contains("maxQueueSize") && clientObj["maxQueueSize"].isDouble()) {
                m_systemConfig.clientConfig.maxQueueSize = clientObj["maxQueueSize"].toInt();
            }
            
//            if (clientObj.contains("cacheTimeout") && clientObj["cacheTimeout"].isDouble()) {
//                m_systemConfig.clientConfig.cacheTimeout = clientObj["cacheTimeout"].toInt();
//            }
            
//            if (clientObj.contains("cacheMaxSize") && clientObj["cacheMaxSize"].isDouble()) {
//                m_systemConfig.clientConfig.cacheMaxSize = clientObj["cacheMaxSize"].toInt();
//            }
        }
    }
    
    // 加载设备配置
    if (json.contains("devices") && json["devices"].isArray()) {
        QJsonArray devicesArray = json["devices"].toArray();
        
        for (const QJsonValue& deviceValue : devicesArray) {
            if (deviceValue.isObject()) {
                DeviceConfig device = jsonToDevice(deviceValue.toObject());
                if (device.slaveId > 0) {
                    m_devices[device.slaveId] = device;
                }
            }
        }
    }
    
    return validateConfig();
}

QJsonObject ConfigManager::toJson() const
{
    QMutexLocker locker(&m_mutex);
    
    QJsonObject json;
    
    // 系统配置
    QJsonObject systemObj;
    systemObj["autoStart"] = m_systemConfig.autoStart;
    systemObj["autoReconnect"] = m_systemConfig.autoReconnect;
    systemObj["monitorInterval"] = m_systemConfig.monitorInterval;
    
    // 客户端配置
    QJsonObject clientObj;
//    clientObj["workerThreadCount"] = m_systemConfig.clientConfig.workerThreadCount;
    clientObj["maxQueueSize"] = m_systemConfig.clientConfig.maxQueueSize;
//    clientObj["cacheTimeout"] = m_systemConfig.clientConfig.cacheTimeout;
//    clientObj["cacheMaxSize"] = m_systemConfig.clientConfig.cacheMaxSize;
    
    systemObj["client"] = clientObj;
    json["system"] = systemObj;
    
    // 设备配置
    QJsonArray devicesArray;
    for (const DeviceConfig& device : m_devices) {
        devicesArray.append(deviceToJson(device));
    }
    json["devices"] = devicesArray;
    
    return json;
}

void ConfigManager::addDevice(const DeviceConfig& device)
{
    QMutexLocker locker(&m_mutex);
    
    if (device.slaveId <= 0) {
        qWarning() << "Invalid slave ID:" << device.slaveId;
        return;
    }
    
    m_devices[device.slaveId] = device;
    emit deviceAdded(device.slaveId);
    emit configChanged();
}

void ConfigManager::removeDevice(int slaveId)
{
    QMutexLocker locker(&m_mutex);
    
    if (m_devices.contains(slaveId)) {
        m_devices.remove(slaveId);
        emit deviceRemoved(slaveId);
        emit configChanged();
    }
}

ConfigManager::DeviceConfig ConfigManager::getDevice(int slaveId) const
{
    QMutexLocker locker(&m_mutex);
    
    if (m_devices.contains(slaveId)) {
        return m_devices[slaveId];
    }
    
    return DeviceConfig();
}

QList<int> ConfigManager::getDeviceIds() const
{
    QMutexLocker locker(&m_mutex);
    return m_devices.keys();
}

void ConfigManager::setSystemConfig(const SystemConfig& config)
{
    QMutexLocker locker(&m_mutex);
    m_systemConfig = config;
    emit configChanged();
}

ConfigManager::SystemConfig ConfigManager::getSystemConfig() const
{
    QMutexLocker locker(&m_mutex);
    return m_systemConfig;
}

void ConfigManager::setRegisterMap(int slaveId, const QMap<int, RegisterInfo>& registerMap)
{
    QMutexLocker locker(&m_mutex);
    
    if (m_devices.contains(slaveId)) {
        m_devices[slaveId].registers = registerMap;
        emit configChanged();
    }
}

QMap<int, RegisterInfo> ConfigManager::getRegisterMap(int slaveId) const
{
    QMutexLocker locker(&m_mutex);
    
    if (m_devices.contains(slaveId)) {
        return m_devices[slaveId].registers;
    }
    
    return QMap<int, RegisterInfo>();
}

ConfigManager::DeviceConfig ConfigManager::defaultDeviceConfig()
{
    DeviceConfig config;
    config.slaveId = 1;
    config.name = "Device 1";
    config.description = "Default Modbus Device";
    config.manufacturer = "Unknown";
    config.model = "Generic";
    config.responseTimeout = 1000;
    config.maxRetries = 3;
    
    // 默认串口配置
    SerialPortConfig serialConfig;
    serialConfig.portName = "COM1";
    serialConfig.baudRate = 9600;
    serialConfig.dataBits = QSerialPort::Data8;
    serialConfig.parity = QSerialPort::NoParity;
    serialConfig.stopBits = QSerialPort::OneStop;
    serialConfig.flowControl = QSerialPort::NoFlowControl;
    serialConfig.interFrameDelay = 5;
    serialConfig.responseTimeout = 1000;
    serialConfig.maxRetries = 3;
    
    config.serialConfig = serialConfig;
    
    return config;
}

ConfigManager::SystemConfig ConfigManager::defaultSystemConfig()
{
    SystemConfig config;
    config.autoStart = false;
    config.autoReconnect = true;
    config.monitorInterval = 1000;
    
    // 默认客户端配置
    ModbusClient::ClientConfig clientConfig;
//    clientConfig.workerThreadCount = 2;
    clientConfig.maxQueueSize = 100;
//    clientConfig.cacheTimeout = 30000; // 30秒
//    clientConfig.cacheMaxSize = 1000;
    
    config.clientConfig = clientConfig;
    
    return config;
}

bool ConfigManager::validateConfig() const
{
//    QMutexLocker locker(&m_mutex); 调用这个方法的地方已经加锁了，在被调用的方法里面不需要加锁，加锁会导致死锁
    
    // 检查系统配置
    if (m_systemConfig.monitorInterval <= 0) {
        LOG_INFO() << "Invalid monitor interval:" << m_systemConfig.monitorInterval;
        return false;
    }
    
    // 检查设备配置
    for (const DeviceConfig& device : m_devices) {
        QStringList errors = validateDeviceConfig(device.slaveId);
        if (!errors.isEmpty()) {
            LOG_INFO() << "Device validation failed for slave" << device.slaveId << ":" << errors;
            return false;
        }
    }
    
    return true;
}

QStringList ConfigManager::validateDeviceConfig(int slaveId) const
{
//    QMutexLocker locker(&m_mutex);
    
    QStringList errors;
    
    if (!m_devices.contains(slaveId)) {
        errors << "Device not found for slave ID: " + QString::number(slaveId);
        return errors;
    }
    
    const DeviceConfig& device = m_devices[slaveId];
    
    // 检查基本配置
    if (device.slaveId <= 0 || device.slaveId > 247) {
        errors << "Invalid slave ID: " + QString::number(device.slaveId);
    }
    
    if (device.name.isEmpty()) {
        errors << "Device name is empty";
    }
    
    if (device.serialConfig.portName.isEmpty()) {
        errors << "Serial port name is empty";
    }
    
    if (device.serialConfig.baudRate <= 0) {
        errors << "Invalid baud rate: " + QString::number(device.serialConfig.baudRate);
    }
    
    if (device.responseTimeout <= 0) {
        errors << "Invalid response timeout: " + QString::number(device.responseTimeout);
    }
    
    if (device.maxRetries < 0) {
        errors << "Invalid max retries: " + QString::number(device.maxRetries);
    }
    
    // 检查寄存器配置
//    for (auto it = device.registers.begin(); it != device.registers.end(); ++it) {
//        const RegisterInfo& reg = it.value();
        
//        if (reg.address < 0 || reg.address > 65535) {
//            errors << "Invalid register address: " + QString::number(reg.address);
//        }
        
//        if (reg.count <= 0 || reg.count > 125) {
//            errors << "Invalid register count: " + QString::number(reg.count);
//        }
        
//        if (reg.name.isEmpty()) {
//            errors << "Register name is empty for address: " + QString::number(reg.address);
//        }
//    }
    
    return errors;
}

QJsonObject ConfigManager::deviceToJson(const DeviceConfig& device) const
{
    QJsonObject deviceObj;
    
    deviceObj["slaveId"] = device.slaveId;
    deviceObj["name"] = device.name;
    deviceObj["description"] = device.description;
    deviceObj["manufacturer"] = device.manufacturer;
    deviceObj["model"] = device.model;
    deviceObj["responseTimeout"] = device.responseTimeout;
    deviceObj["maxRetries"] = device.maxRetries;
    
    // 串口配置
    QJsonObject serialObj;
    serialObj["portName"] = device.serialConfig.portName;
    serialObj["baudRate"] = device.serialConfig.baudRate;
    serialObj["dataBits"] = static_cast<int>(device.serialConfig.dataBits);
    serialObj["parity"] = static_cast<int>(device.serialConfig.parity);
    serialObj["stopBits"] = static_cast<int>(device.serialConfig.stopBits);
    serialObj["flowControl"] = static_cast<int>(device.serialConfig.flowControl);
    serialObj["interFrameDelay"] = device.serialConfig.interFrameDelay;
    serialObj["responseTimeout"] = device.serialConfig.responseTimeout;
    serialObj["maxRetries"] = device.serialConfig.maxRetries;
    
    deviceObj["serialConfig"] = serialObj;
    
    // 寄存器配置
    QJsonObject registersObj;
    for (auto it = device.registers.begin(); it != device.registers.end(); ++it) {
        registersObj[QString::number(it.key())] = registerToJson(it.value());
    }
    deviceObj["registers"] = registersObj;
    
    return deviceObj;
}

ConfigManager::DeviceConfig ConfigManager::jsonToDevice(const QJsonObject& json) const
{
    DeviceConfig device;
    
    if (json.contains("slaveId") && json["slaveId"].isDouble()) {
        device.slaveId = json["slaveId"].toInt();
    }
    
    if (json.contains("name") && json["name"].isString()) {
        device.name = json["name"].toString();
    }
    
    if (json.contains("description") && json["description"].isString()) {
        device.description = json["description"].toString();
    }
    
    if (json.contains("manufacturer") && json["manufacturer"].isString()) {
        device.manufacturer = json["manufacturer"].toString();
    }
    
    if (json.contains("model") && json["model"].isString()) {
        device.model = json["model"].toString();
    }
    
    if (json.contains("responseTimeout") && json["responseTimeout"].isDouble()) {
        device.responseTimeout = json["responseTimeout"].toInt();
    }
    
    if (json.contains("maxRetries") && json["maxRetries"].isDouble()) {
        device.maxRetries = json["maxRetries"].toInt();
    }
    
    // 串口配置
    if (json.contains("serialConfig") && json["serialConfig"].isObject()) {
        QJsonObject serialObj = json["serialConfig"].toObject();
        
        if (serialObj.contains("portName") && serialObj["portName"].isString()) {
            device.serialConfig.portName = serialObj["portName"].toString();
        }
        
        if (serialObj.contains("baudRate") && serialObj["baudRate"].isDouble()) {
            device.serialConfig.baudRate = serialObj["baudRate"].toInt();
        }
        
        if (serialObj.contains("dataBits") && serialObj["dataBits"].isDouble()) {
            device.serialConfig.dataBits = static_cast<QSerialPort::DataBits>(serialObj["dataBits"].toInt());
        }
        
        if (serialObj.contains("parity") && serialObj["parity"].isDouble()) {
            device.serialConfig.parity = static_cast<QSerialPort::Parity>(serialObj["parity"].toInt());
        }
        
        if (serialObj.contains("stopBits") && serialObj["stopBits"].isDouble()) {
            device.serialConfig.stopBits = static_cast<QSerialPort::StopBits>(serialObj["stopBits"].toInt());
        }
        
        if (serialObj.contains("flowControl") && serialObj["flowControl"].isDouble()) {
            device.serialConfig.flowControl = static_cast<QSerialPort::FlowControl>(serialObj["flowControl"].toInt());
        }
        
        if (serialObj.contains("interFrameDelay") && serialObj["interFrameDelay"].isDouble()) {
            device.serialConfig.interFrameDelay = serialObj["interFrameDelay"].toInt();
        }
        
        if (serialObj.contains("responseTimeout") && serialObj["responseTimeout"].isDouble()) {
            device.serialConfig.responseTimeout = serialObj["responseTimeout"].toInt();
        }
        
        if (serialObj.contains("maxRetries") && serialObj["maxRetries"].isDouble()) {
            device.serialConfig.maxRetries = serialObj["maxRetries"].toInt();
        }
    }
    
    // 寄存器配置
    if (json.contains("registers") && json["registers"].isObject()) {
        QJsonObject registersObj = json["registers"].toObject();
        
        for (auto it = registersObj.begin(); it != registersObj.end(); ++it) {
            bool ok;
            int address = it.key().toInt(&ok);
            if (ok && it.value().isObject()) {
                RegisterInfo reg = jsonToRegister(it.value().toObject());
                device.registers[address] = reg;
            }
        }
    }
    
    return device;
}

QJsonObject ConfigManager::registerToJson(const RegisterInfo& reg) const
{
    QJsonObject regObj;
    
//    regObj["address"] = reg.address;
//    regObj["count"] = reg.count;
//    regObj["name"] = reg.name;
//    regObj["description"] = reg.description;
//    regObj["dataType"] = static_cast<int>(reg.dataType);
//    regObj["accessMode"] = static_cast<int>(reg.accessMode);
//    regObj["scalingFactor"] = reg.scalingFactor;
//    regObj["offset"] = reg.offset;
//    regObj["unit"] = reg.unit;
//    regObj["minValue"] = reg.minValue;
//    regObj["maxValue"] = reg.maxValue;
//    regObj["defaultValue"] = reg.defaultValue;
//    regObj["isReadOnly"] = reg.isReadOnly;
//    regObj["isVolatile"] = reg.isVolatile;
//    regObj["updateInterval"] = reg.updateInterval;
    
    return regObj;
}

RegisterInfo ConfigManager::jsonToRegister(const QJsonObject& json) const
{
    RegisterInfo reg;
    
    if (json.contains("address") && json["address"].isDouble()) {
        reg.address = json["address"].toInt();
    }
    
//    if (json.contains("count") && json["count"].isDouble()) {
//        reg.count = json["count"].toInt();
//    }
    
    if (json.contains("name") && json["name"].isString()) {
        reg.name = json["name"].toString();
    }
    
    if (json.contains("description") && json["description"].isString()) {
        reg.description = json["description"].toString();
    }
    
//    if (json.contains("dataType") && json["dataType"].isDouble()) {
//        reg.dataType = static_cast<DataType>(json["dataType"].toInt());
//    }
    
//    if (json.contains("accessMode") && json["accessMode"].isDouble()) {
//        reg.accessMode = static_cast<AccessMode>(json["accessMode"].toInt());
//    }
    
//    if (json.contains("scalingFactor") && json["scalingFactor"].isDouble()) {
//        reg.scalingFactor = json["scalingFactor"].toDouble();
//    }
    
    if (json.contains("offset") && json["offset"].isDouble()) {
        reg.offset = json["offset"].toDouble();
    }
    
    if (json.contains("unit") && json["unit"].isString()) {
        reg.unit = json["unit"].toString();
    }
    
//    if (json.contains("minValue") && json["minValue"].isDouble()) {
//        reg.minValue = json["minValue"].toDouble();
//    }
    
//    if (json.contains("maxValue") && json["maxValue"].isDouble()) {
//        reg.maxValue = json["maxValue"].toDouble();
//    }
    
//    if (json.contains("defaultValue") && json["defaultValue"].isDouble()) {
//        reg.defaultValue = json["defaultValue"].toDouble();
//    }
    
//    if (json.contains("isReadOnly") && json["isReadOnly"].isBool()) {
//        reg.isReadOnly = json["isReadOnly"].toBool();
//    }
    
//    if (json.contains("isVolatile") && json["isVolatile"].isBool()) {
//        reg.isVolatile = json["isVolatile"].toBool();
//    }
    
//    if (json.contains("updateInterval") && json["updateInterval"].isDouble()) {
//        reg.updateInterval = json["updateInterval"].toInt();
//    }
    
    return reg;
}
