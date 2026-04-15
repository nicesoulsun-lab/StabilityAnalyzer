#ifndef JSONCONFIG_H
#define JSONCONFIG_H

#include "IConfigInterface.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QVariantMap>
#include <QMap>
/**
 * json配置文件 - 支持任意层级架构
 * */
class CONFIGMANAGER_EXPORT JsonConfig : public IConfigInterface
{
public:
    JsonConfig();
    ~JsonConfig() override = default;
    
    // IConfigInterface 接口实现
    bool load(const QString& filePath) override;
    bool save(const QString& filePath) override;
    QVariant getValue(const QString& section, const QString& key, 
                     const QVariant& defaultValue = QVariant()) override;
    void setValue(const QString& section, const QString& key, 
                 const QVariant& value) override;
    QStringList getSections() override;
    QStringList getKeys(const QString& section) override;
    bool containsSection(const QString& section) override;
    bool containsKey(const QString& section, const QString& key) override;
    void removeSection(const QString& section) override;
    void removeKey(const QString& section, const QString& key) override;
    void clear() override;

    // 多级路径接口
    QVariant getValueByPath(const QString& path, const QVariant& defaultValue = QVariant());
    void setValueByPath(const QString& path, const QVariant& value);
    bool containsPath(const QString& path);
    void removePath(const QString& path);
    QVariantMap getSubTree(const QString& path);
    QStringList getAllPaths(const QString& prefix = QString());

private:
    QJsonDocument m_doc; //json文件
    QString m_currentFilePath; //当前文件路径
    QVariantMap m_data; //存储json文件数据map
    
    void loadDataFromJson();
    void saveDataToJson();
    
    // 路径解析和递归遍历工具函数
    QVariant getValueFromPath(const QVariantMap& data, const QStringList& pathParts, int index);
    void setValueToPath(QVariantMap& data, const QStringList& pathParts, int index, const QVariant& value);
    bool containsPath(const QVariantMap& data, const QStringList& pathParts, int index);
    void removePath(QVariantMap& data, const QStringList& pathParts, int index);
    QVariantMap getSubTree(const QVariantMap& data, const QStringList& pathParts, int index);
    void getAllPaths(const QVariantMap& data, const QString& currentPath, QStringList& result);
    
    // JSON转换工具函数
    QVariant jsonValueToVariant(const QJsonValue& jsonValue);
    QJsonValue variantToJsonValue(const QVariant& variant);
    QVariantMap jsonObjectToVariantMap(const QJsonObject& jsonObject);
    QJsonObject variantMapToJsonObject(const QVariantMap& variantMap);
};

#endif // JSONCONFIG_H
