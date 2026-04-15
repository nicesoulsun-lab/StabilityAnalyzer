#include "ModbusConfig.h"
#include <QDebug>
#include <QSerialPort>
#include <QCoreApplication>
#include "logmanager.h"
ModbusConfig::ModbusConfig(QObject *parent)
    : QObject(parent)
    , m_settings(nullptr)
    , m_portName("COM1")
    , m_baudRate(9600)
    , m_dataBits(QSerialPort::Data8)
    , m_parity(QSerialPort::NoParity)
    , m_stopBits(QSerialPort::OneStop)
    , m_responseTimeout(3000)
    , m_retryCount(3)
    , m_autoReconnect(true)
{
    //读取配置文件获取串口信息
    QString cofigPath = QCoreApplication::applicationDirPath() + "/config.ini";
    LOG_INFO()<<"读取配置文件获取串口通信设置......" << cofigPath;
    loadFromFile(cofigPath);
}

ModbusConfig::~ModbusConfig()
{
    if (m_settings) {
        delete m_settings;
    }
}

bool ModbusConfig::loadFromFile(const QString &filename)
{
    if (m_settings) {
        delete m_settings;
    }
    
    m_settings = new QSettings(filename, QSettings::IniFormat);
    
    if (m_settings->status() != QSettings::NoError) {
        qWarning() << "Failed to load config file:" << filename;
        return false;
    }
    
    // 加载配置值
    setPortName(m_settings->value("Serial/PortName", m_portName).toString());
    setBaudRate(m_settings->value("Serial/BaudRate", m_baudRate).toInt());
    setDataBits(m_settings->value("Serial/DataBits", m_dataBits).toInt());
    setParity(m_settings->value("Serial/Parity", m_parity).toInt());
    setStopBits(m_settings->value("Serial/StopBits", m_stopBits).toInt());
    setResponseTimeout(m_settings->value("Communication/ResponseTimeout", m_responseTimeout).toInt());
    setRetryCount(m_settings->value("Communication/RetryCount", m_retryCount).toInt());
    setAutoReconnect(m_settings->value("Communication/AutoReconnect", m_autoReconnect).toBool());
    
    return true;
}

bool ModbusConfig::saveToFile(const QString &filename)
{
    if (!m_settings || m_settings->fileName() != filename) {
        if (m_settings) {
            delete m_settings;
        }
        m_settings = new QSettings(filename, QSettings::IniFormat);
    }
    
    // 保存配置值
    m_settings->setValue("Serial/PortName", m_portName);
    m_settings->setValue("Serial/BaudRate", m_baudRate);
    m_settings->setValue("Serial/DataBits", m_dataBits);
    m_settings->setValue("Serial/Parity", m_parity);
    m_settings->setValue("Serial/StopBits", m_stopBits);
    m_settings->setValue("Communication/ResponseTimeout", m_responseTimeout);
    m_settings->setValue("Communication/RetryCount", m_retryCount);
    m_settings->setValue("Communication/AutoReconnect", m_autoReconnect);
    
    m_settings->sync();
    
    return m_settings->status() == QSettings::NoError;
}

// 属性访问器实现
QString ModbusConfig::portName() const { return m_portName; }
int ModbusConfig::baudRate() const { return m_baudRate; }
int ModbusConfig::dataBits() const { return m_dataBits; }
int ModbusConfig::parity() const { return m_parity; }
int ModbusConfig::stopBits() const { return m_stopBits; }
int ModbusConfig::responseTimeout() const { return m_responseTimeout; }
int ModbusConfig::retryCount() const { return m_retryCount; }
bool ModbusConfig::autoReconnect() const { return m_autoReconnect; }

// 属性设置器实现
void ModbusConfig::setPortName(const QString &portName)
{
    if (m_portName != portName) {
        m_portName = portName;
        emit portNameChanged(m_portName);
    }
}

void ModbusConfig::setBaudRate(int baudRate)
{
    if (m_baudRate != baudRate) {
        m_baudRate = baudRate;
        emit baudRateChanged(m_baudRate);
    }
}

void ModbusConfig::setDataBits(int dataBits)
{
    if (m_dataBits != dataBits) {
        m_dataBits = dataBits;
        emit dataBitsChanged(m_dataBits);
    }
}

void ModbusConfig::setParity(int parity)
{
    if (m_parity != parity) {
        m_parity = parity;
        emit parityChanged(m_parity);
    }
}

void ModbusConfig::setStopBits(int stopBits)
{
    if (m_stopBits != stopBits) {
        m_stopBits = stopBits;
        emit stopBitsChanged(m_stopBits);
    }
}

void ModbusConfig::setResponseTimeout(int timeout)
{
    if (m_responseTimeout != timeout) {
        m_responseTimeout = timeout;
        emit responseTimeoutChanged(m_responseTimeout);
    }
}

void ModbusConfig::setRetryCount(int count)
{
    if (m_retryCount != count) {
        m_retryCount = count;
        emit retryCountChanged(m_retryCount);
    }
}

void ModbusConfig::setAutoReconnect(bool autoReconnect)
{
    if (m_autoReconnect != autoReconnect) {
        m_autoReconnect = autoReconnect;
        emit autoReconnectChanged(m_autoReconnect);
    }
}
