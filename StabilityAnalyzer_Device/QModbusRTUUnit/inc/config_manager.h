#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <QObject>
#include <QSettings>
#include <QJsonObject>
#include <QJsonArray>
#include "modbus_client.h"
#include "qmodbusrtuunit_global.h"
/**
**读取moudbusrtu相关配置文件
**注：操作的是json文件
**现在没用到
*/
class QMODBUSRTUUNIT_EXPORT ConfigManager : public QObject
{
    Q_OBJECT

public:
    struct DeviceConfig {
        int slaveId;
        QString name;
        QString description;
        QString manufacturer;
        QString model;
        SerialPortConfig serialConfig;
        QMap<int, RegisterInfo> registers;
        int responseTimeout;
        int maxRetries;
        
        DeviceConfig() : slaveId(1), responseTimeout(1000), maxRetries(3) {}
    };

    struct SystemConfig {
        ModbusClient::ClientConfig clientConfig;
//        Logger::LogConfig logConfig;
        bool autoStart;
        bool autoReconnect;
        int monitorInterval;
        
        SystemConfig() : autoStart(false), autoReconnect(true), monitorInterval(1000) {}
    };

    explicit ConfigManager(QObject* parent = nullptr);
    
    // 配置文件管理
    bool loadFromFile(const QString& filename);
    bool saveToFile(const QString& filename);
    bool loadFromJson(const QJsonObject& json);
    QJsonObject toJson() const;
    
    // 设备配置
    void addDevice(const DeviceConfig& device);
    void removeDevice(int slaveId);
    DeviceConfig getDevice(int slaveId) const;
    QList<int> getDeviceIds() const;
    
    // 系统配置
    void setSystemConfig(const SystemConfig& config);
    SystemConfig getSystemConfig() const;
    
    // 注册器配置
    void setRegisterMap(int slaveId, const QMap<int, RegisterInfo>& registerMap);
    QMap<int, RegisterInfo> getRegisterMap(int slaveId) const;
    
    // 默认配置
    static DeviceConfig defaultDeviceConfig();
    static SystemConfig defaultSystemConfig();
    
    // 验证配置
    bool validateConfig() const;
    QStringList validateDeviceConfig(int slaveId) const;
    
signals:
    void configLoaded();
    void configSaved();
    void configChanged();
    void deviceAdded(int slaveId);
    void deviceRemoved(int slaveId);
    
private:
    QMap<int, DeviceConfig> m_devices;
    SystemConfig m_systemConfig;
    mutable QMutex m_mutex;
    
    // 序列化辅助方法
    QJsonObject deviceToJson(const DeviceConfig& device) const;
    DeviceConfig jsonToDevice(const QJsonObject& json) const;
    QJsonObject registerToJson(const RegisterInfo& reg) const;
    RegisterInfo jsonToRegister(const QJsonObject& json) const;
};
#endif
