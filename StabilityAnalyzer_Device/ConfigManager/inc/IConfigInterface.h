#ifndef ICONFIGINTERFACE_H
#define ICONFIGINTERFACE_H

#include "configmanager_global.h"
#include <QString>
#include <QVariant>
#include <QMap>
/**
 * 所有类型配置文件的接口，其他类型的配置文件实现接口来实现自己的读取写入
 * */
class CONFIGMANAGER_EXPORT IConfigInterface
{
public:
    virtual ~IConfigInterface() = default;
    
    // 加载配置文件
    virtual bool load(const QString& filePath) = 0;
    
    // 保存配置文件
    virtual bool save(const QString& filePath) = 0;
    
    // 读取配置值，读取某个配置节的键对应的值
    virtual QVariant getValue(const QString& section, const QString& key, 
                             const QVariant& defaultValue = QVariant()) = 0;
    
    // 设置配置值，设置某个配置节的键对应的值
    virtual void setValue(const QString& section, const QString& key, 
                         const QVariant& value) = 0;
    
    // 获取所有配置节
    virtual QStringList getSections() = 0;
    
    // 获取指定节的所有键
    virtual QStringList getKeys(const QString& section) = 0;
    
    // 检查配置节是否存在
    virtual bool containsSection(const QString& section) = 0;
    
    // 检查配置键是否存在
    virtual bool containsKey(const QString& section, const QString& key) = 0;
    
    // 删除配置节
    virtual void removeSection(const QString& section) = 0;
    
    // 删除配置键
    virtual void removeKey(const QString& section, const QString& key) = 0;
    
    // 清空所有配置
    virtual void clear() = 0;
};

#endif // ICONFIGINTERFACE_H
