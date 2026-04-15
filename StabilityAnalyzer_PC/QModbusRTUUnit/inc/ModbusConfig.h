#ifndef MODBUSCONFIG_H
#define MODBUSCONFIG_H

#include <QObject>
#include <QSettings>
#include "qmodbusrtuunit_global.h"

/**
 * @brief The ModbusConfig class
 * Modbus RTU 配置管理类
 * 提供统一的配置管理，支持配置文件保存和加载
 * 现在没用到
 */
class QMODBUSRTUUNIT_EXPORT ModbusConfig : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString portName READ portName WRITE setPortName NOTIFY portNameChanged)
    Q_PROPERTY(int baudRate READ baudRate WRITE setBaudRate NOTIFY baudRateChanged)
    Q_PROPERTY(int dataBits READ dataBits WRITE setDataBits NOTIFY dataBitsChanged)
    Q_PROPERTY(int parity READ parity WRITE setParity NOTIFY parityChanged)
    Q_PROPERTY(int stopBits READ stopBits WRITE setStopBits NOTIFY stopBitsChanged)
    Q_PROPERTY(int responseTimeout READ responseTimeout WRITE setResponseTimeout NOTIFY responseTimeoutChanged)
    Q_PROPERTY(int retryCount READ retryCount WRITE setRetryCount NOTIFY retryCountChanged)
    Q_PROPERTY(bool autoReconnect READ autoReconnect WRITE setAutoReconnect NOTIFY autoReconnectChanged)

public:
    explicit ModbusConfig(QObject *parent = nullptr);
    virtual ~ModbusConfig();

    // 配置管理
    bool loadFromFile(const QString &filename);
    bool saveToFile(const QString &filename);

    // 属性访问器
    QString portName() const;
    int baudRate() const;
    int dataBits() const;
    int parity() const;
    int stopBits() const;
    int responseTimeout() const;
    int retryCount() const;
    bool autoReconnect() const;

    // 属性设置器
    void setPortName(const QString &portName);
    void setBaudRate(int baudRate);
    void setDataBits(int dataBits);
    void setParity(int parity);
    void setStopBits(int stopBits);
    void setResponseTimeout(int timeout);
    void setRetryCount(int count);
    void setAutoReconnect(bool autoReconnect);

signals:
    void portNameChanged(const QString &portName);
    void baudRateChanged(int baudRate);
    void dataBitsChanged(int dataBits);
    void parityChanged(int parity);
    void stopBitsChanged(int stopBits);
    void responseTimeoutChanged(int timeout);
    void retryCountChanged(int count);
    void autoReconnectChanged(bool autoReconnect);

private:
    QSettings *m_settings;
    
    QString m_portName;
    int m_baudRate;
    int m_dataBits;
    int m_parity;
    int m_stopBits;
    int m_responseTimeout;
    int m_retryCount;
    bool m_autoReconnect;
};

#endif // MODBUSCONFIG_H
