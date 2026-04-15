#include "inc/IniConfig.h"
#include <QDebug>
/**
 * 读取ini文件
**/
IniConfig::IniConfig()
    : m_settings(nullptr)
{
}

IniConfig::~IniConfig()
{
    if (m_settings) {
        delete m_settings;
        m_settings = nullptr;
    }
}

//加载配置文件
bool IniConfig::load(const QString& filePath)
{
    if (m_settings) {
        delete m_settings;
        m_settings = nullptr;
    }
    
    m_settings = new QSettings(filePath, QSettings::IniFormat);
    
    if (m_settings->status() != QSettings::NoError) {
        qWarning() << "Failed to load INI file:" << filePath;
        delete m_settings;
        m_settings = nullptr;
        return false;
    }
    
    m_currentFilePath = filePath;
    loadDataFromSettings();
    return true;
}

//保存配置文件
bool IniConfig::save(const QString& filePath)
{
    QString savePath = filePath.isEmpty() ? m_currentFilePath : filePath;
    
    if (savePath.isEmpty()) {
        qWarning() << "No file path specified for saving";
        return false;
    }
    
    //删掉之前的配置文件
    if (m_settings) {
        delete m_settings;
        m_settings = nullptr;
    }
    
    m_settings = new QSettings(savePath, QSettings::IniFormat);
    saveDataToSettings();
    
    //重新同步
    m_settings->sync();
    bool success = (m_settings->status() == QSettings::NoError);
    
    if (!success) {
        qWarning() << "Failed to save INI file:" << savePath;
    }
    
    return success;
}

//获取配置节的键值
QVariant IniConfig::getValue(const QString& section, const QString& key, 
                            const QVariant& defaultValue)
{
    if (!m_data.contains(section) || !m_data[section].contains(key)) {
        return defaultValue;
    }
    
    return m_data[section][key];
}

//设置配置节的键值
void IniConfig::setValue(const QString& section, const QString& key, 
                        const QVariant& value)
{
    if (!m_data.contains(section)) {
        m_data[section] = QMap<QString, QVariant>();
    }
    
    m_data[section][key] = value;
}

//获取所有的配置节
QStringList IniConfig::getSections()
{
    return m_data.keys();
}

//获取这个配置节的所有键值
QStringList IniConfig::getKeys(const QString& section)
{
    if (!m_data.contains(section)) {
        return QStringList();
    }
    
    return m_data[section].keys();
}

//判断是否存在这个配置节
bool IniConfig::containsSection(const QString& section)
{
    return m_data.contains(section);
}

//判断这个配置节的键是否存在
bool IniConfig::containsKey(const QString& section, const QString& key)
{
    return m_data.contains(section) && m_data[section].contains(key);
}

//删除配置节
void IniConfig::removeSection(const QString& section)
{
    m_data.remove(section);
}

//删除配置节的键值
void IniConfig::removeKey(const QString& section, const QString& key)
{
    if (m_data.contains(section)) {
        m_data[section].remove(key);
    }
}

void IniConfig::clear()
{
    m_data.clear();
}

//加载数据从配置文件
void IniConfig::loadDataFromSettings()
{
    m_data.clear();
    
    if (!m_settings) {
        return;
    }
    
    QStringList sections = m_settings->childGroups();
    
    for (const QString& section : sections) {
        m_settings->beginGroup(section);
        //获取这个配置节下的所有键
        QStringList keys = m_settings->childKeys();
        QMap<QString, QVariant> sectionData;
        
        //遍历所有键获取对应值
        for (const QString& key : keys) {
            QVariant value = m_settings->value(key);
            sectionData[key] = value;
        }
        
        m_data[section] = sectionData;
        m_settings->endGroup();
    }
}

//保存数据到配置文件
void IniConfig::saveDataToSettings()
{
    if (!m_settings) {
        return;
    }
    
    m_settings->clear();
    
    //遍历所有数据存储
    for (auto it = m_data.begin(); it != m_data.end(); ++it) {
        const QString& section = it.key();
        const QMap<QString, QVariant>& sectionData = it.value();
        
        m_settings->beginGroup(section);
        
        for (auto keyIt = sectionData.begin(); keyIt != sectionData.end(); ++keyIt) {
            m_settings->setValue(keyIt.key(), keyIt.value());
        }
        
        m_settings->endGroup();
    }
}
