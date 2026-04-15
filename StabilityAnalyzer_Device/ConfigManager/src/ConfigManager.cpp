#include "inc/ConfigManager.h"
#include "inc/IniConfig.h"
#include "inc/XmlConfig.h"
#include "inc/JsonConfig.h"
#include <QFileInfo>
#include <QDebug>

ConfigManager::ConfigManager()
    : m_currentFormat(INI)
{
}

ConfigManager::~ConfigManager()
{
}

ConfigManager *ConfigManager::instance()
{
    static ConfigManager* a = nullptr;
    if(a==nullptr){
        a = new ConfigManager;
    }
    return a;
}

//加载文件
bool ConfigManager::load(const QString& filePath, ConfigFormat format)
{
    if (filePath.isEmpty()) {
        qWarning() << "File path is empty";
        return false;
    }
    
    // 如果未指定格式，则自动检测
    if (format == NONE) {
        format = detectFormat(filePath);
    }
    
    //根据文件类型获取配置类
    m_config = createConfigHandler(format);
    if (!m_config) {
        qWarning() << "Failed to create config handler for format:" << format;
        return false;
    }
    
    //加载配置文件
    bool success = m_config->load(filePath);
    if (success) {
        m_currentFilePath = filePath;
        m_currentFormat = format;
    } else {
        m_config.clear();
    }
    
    return success;
}

//保存
bool ConfigManager::save(const QString& filePath)
{
    if (!m_config) {
        qWarning() << "No config loaded";
        return false;
    }
    
    QString savePath = filePath.isEmpty() ? m_currentFilePath : filePath;
    if (savePath.isEmpty()) {
        qWarning() << "No file path specified for saving";
        return false;
    }
    
    bool success = m_config->save(savePath);
    if (success && filePath.isEmpty()) {
        m_currentFilePath = savePath;
    }
    
    return success;
}

//获取键值
QVariant ConfigManager::getValue(const QString& section, const QString& key, 
                                const QVariant& defaultValue)
{
    if (!m_config) {
        qWarning() << "No config loaded";
        return defaultValue;
    }
    
    return m_config->getValue(section, key, defaultValue);
}

//设置键值
void ConfigManager::setValue(const QString& section, const QString& key, 
                            const QVariant& value)
{
    if (!m_config) {
        qWarning() << "No config loaded";
        return;
    }
    
    m_config->setValue(section, key, value);
}

QStringList ConfigManager::getSections()
{
    if (!m_config) {
        qWarning() << "No config loaded";
        return QStringList();
    }
    
    return m_config->getSections();
}

QStringList ConfigManager::getKeys(const QString& section)
{
    if (!m_config) {
        qWarning() << "No config loaded";
        return QStringList();
    }
    
    return m_config->getKeys(section);
}

bool ConfigManager::containsSection(const QString& section)
{
    if (!m_config) {
        qWarning() << "No config loaded";
        return false;
    }
    
    return m_config->containsSection(section);
}

bool ConfigManager::containsKey(const QString& section, const QString& key)
{
    if (!m_config) {
        qWarning() << "No config loaded";
        return false;
    }
    
    return m_config->containsKey(section, key);
}

void ConfigManager::removeSection(const QString& section)
{
    if (!m_config) {
        qWarning() << "No config loaded";
        return;
    }
    
    m_config->removeSection(section);
}

void ConfigManager::removeKey(const QString& section, const QString& key)
{
    if (!m_config) {
        qWarning() << "No config loaded";
        return;
    }
    
    m_config->removeKey(section, key);
}

void ConfigManager::clear()
{
    if (!m_config) {
        qWarning() << "No config loaded";
        return;
    }
    
    m_config->clear();
}

QString ConfigManager::getCurrentFilePath() const
{
    return m_currentFilePath;
}

ConfigManager::ConfigFormat ConfigManager::getCurrentFormat() const
{
    return m_currentFormat;
}

//检测是什么类型的文件
ConfigManager::ConfigFormat ConfigManager::detectFormat(const QString& filePath)
{
    QFileInfo fileInfo(filePath);
    QString suffix = fileInfo.suffix().toLower();
    
    if (suffix == "ini" || suffix == "conf" || suffix == "cfg") {
        return INI;
    } else if (suffix == "xml") {
        return XML;
    } else if (suffix == "json" || suffix == "js") {
        return JSON;
    }
    
    // 默认使用INI格式
    return INI;
}

//创建不同文件的处理器，不同类型的文件使用不同的接口
QSharedPointer<IConfigInterface> ConfigManager::createConfigHandler(ConfigFormat format)
{
    switch (format) {
    case INI:
        return QSharedPointer<IConfigInterface>(new IniConfig());
    case XML:
        return QSharedPointer<IConfigInterface>(new XmlConfig());
    case JSON:
        return QSharedPointer<IConfigInterface>(new JsonConfig());
    default:
        return QSharedPointer<IConfigInterface>();
    }
}
