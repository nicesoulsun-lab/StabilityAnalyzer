#ifndef XMLCONFIG_H
#define XMLCONFIG_H

#include "IConfigInterface.h"
#include <QtXml/QDomDocument>
#include <QVariantMap>
#include <QMap>
/**
 * xml配置文件
 * */
class CONFIGMANAGER_EXPORT XmlConfig : public IConfigInterface
{
public:
    XmlConfig();
    ~XmlConfig() override = default;
    
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
    QDomDocument m_doc;
    QString m_currentFilePath;
    QVariantMap m_data;
    
    void loadDataFromXml();
    void saveDataToXml();
    
    // 路径解析和递归遍历工具函数
    QVariant getValueFromPath(const QVariantMap& data, const QStringList& pathParts, int index);
    void setValueToPath(QVariantMap& data, const QStringList& pathParts, int index, const QVariant& value);
    bool containsPath(const QVariantMap& data, const QStringList& pathParts, int index);
    void removePath(QVariantMap& data, const QStringList& pathParts, int index);
    QVariantMap getSubTree(const QVariantMap& data, const QStringList& pathParts, int index);
    void getAllPaths(const QVariantMap& data, const QString& currentPath, QStringList& result);
    
    // XML转换工具函数
    QVariantMap domElementToVariantMap(const QDomElement& element);
    void variantMapToDomElement(const QVariantMap& variantMap, QDomElement& parentElement, QDomDocument& doc);
    QVariant domElementValueToVariant(const QDomElement& element);
    void variantToDomElement(const QVariant& variant, QDomElement& parentElement, QDomDocument& doc, const QString& tagName);
};

#endif // XMLCONFIG_H
