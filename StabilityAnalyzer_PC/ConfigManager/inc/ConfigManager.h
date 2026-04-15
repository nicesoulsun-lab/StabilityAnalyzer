#ifndef CONFIGMANAGER_H
#define CONFIGMANAGER_H

#include "configmanager_global.h"
#include "IConfigInterface.h"
#include <QString>
#include <QVariant>
#include <QSharedPointer>
#include <QObject>
#include <QMetaType>
/**
 * 配置文件管理类，调用入口
**/
class CONFIGMANAGER_EXPORT ConfigManager
{
public:
    enum ConfigFormat {
        INI,
        XML,
        JSON,
        NONE
    };
    
    ConfigManager();
    ~ConfigManager();
    static ConfigManager*instance();

    // 加载配置文件
    bool load(const QString& filePath, ConfigFormat format = INI);
    
    // 保存配置文件
    bool save(const QString& filePath = QString());
    
    // 读取配置值
    QVariant getValue(const QString& section, const QString& key, 
                     const QVariant& defaultValue = QVariant());
    
    // 设置配置值
    void setValue(const QString& section, const QString& key, 
                 const QVariant& value);
    
    // 获取所有配置节
    QStringList getSections();
    
    // 获取指定节的所有键
    QStringList getKeys(const QString& section);
    
    // 检查配置节是否存在
    bool containsSection(const QString& section);
    
    // 检查配置键是否存在
    bool containsKey(const QString& section, const QString& key);
    
    // 删除配置节
    void removeSection(const QString& section);
    
    // 删除配置键
    void removeKey(const QString& section, const QString& key);
    
    // 清空所有配置
    void clear();
    
    // 获取当前文件路径
    QString getCurrentFilePath() const;
    
    // 获取当前配置格式
    ConfigFormat getCurrentFormat() const;
    
    // 根据文件扩展名自动检测格式
    static ConfigFormat detectFormat(const QString& filePath);

private:
    QSharedPointer<IConfigInterface> m_config; //配置文件接口
    QString m_currentFilePath;
    ConfigFormat m_currentFormat;
    
    // 创建指定格式的配置处理器
    QSharedPointer<IConfigInterface> createConfigHandler(ConfigFormat format);
};
#define CONFIGMANAGER ConfigManager::instance()
#endif // CONFIGMANAGER_H
