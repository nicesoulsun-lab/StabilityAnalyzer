#include "inc/XmlConfig.h"
#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <QStringList>

XmlConfig::XmlConfig()
{
    // 创建根元素
    QDomElement root = m_doc.createElement("config");
    m_doc.appendChild(root);
}

//加载文件
bool XmlConfig::load(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Failed to open XML file:" << filePath;
        return false;
    }
    
    QString errorMsg;
    int errorLine, errorColumn;
    
    if (!m_doc.setContent(&file, &errorMsg, &errorLine, &errorColumn)) {
        qWarning() << "Failed to parse XML file:" << filePath
                  << "Error:" << errorMsg << "at line" << errorLine << "column" << errorColumn;
        file.close();
        return false;
    }
    
    file.close();
    m_currentFilePath = filePath;
    loadDataFromXml();
    return true;
}

//保存文件
bool XmlConfig::save(const QString& filePath)
{
    QString savePath = filePath.isEmpty() ? m_currentFilePath : filePath;
    
    if (savePath.isEmpty()) {
        qWarning() << "No file path specified for saving";
        return false;
    }
    
    saveDataToXml();
    
    QFile file(savePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "Failed to open XML file for writing:" << savePath;
        return false;
    }
    
    QTextStream out(&file);
    out.setCodec("UTF-8");
    m_doc.save(out, 4);
    file.close();
    
    return true;
}

//获取配置节下的键值
QVariant XmlConfig::getValue(const QString& section, const QString& key, 
                            const QVariant& defaultValue)
{
    // 将section/key作为两级路径处理
    QString path = section + "/" + key;
    return getValueByPath(path, defaultValue);
}

//设置配置节下的键值
void XmlConfig::setValue(const QString& section, const QString& key, 
                        const QVariant& value)
{
    // 将section/key作为两级路径处理
    QString path = section + "/" + key;
    setValueByPath(path, value);
}

//获取所有配置节
QStringList XmlConfig::getSections()
{
    // 返回第一层键作为sections
    QStringList sections;
    for (auto it = m_data.begin(); it != m_data.end(); ++it) {
        sections.append(it.key());
    }
    return sections;
}

//获取所有键
QStringList XmlConfig::getKeys(const QString& section)
{
    // 返回指定section下的第二层键
    if (!m_data.contains(section)) {
        return QStringList();
    }
    
    QVariant sectionData = m_data[section];
    if (sectionData.canConvert<QVariantMap>()) {
        QVariantMap sectionMap = sectionData.toMap();
        return sectionMap.keys();
    }
    
    return QStringList();
}

bool XmlConfig::containsSection(const QString& section)
{
    return m_data.contains(section);
}

bool XmlConfig::containsKey(const QString& section, const QString& key)
{
    if (!m_data.contains(section)) {
        return false;
    }
    
    QVariant sectionData = m_data[section];
    if (sectionData.canConvert<QVariantMap>()) {
        QVariantMap sectionMap = sectionData.toMap();
        return sectionMap.contains(key);
    }
    
    return false;
}

void XmlConfig::removeSection(const QString& section)
{
    m_data.remove(section);
}

void XmlConfig::removeKey(const QString& section, const QString& key)
{
    if (m_data.contains(section)) {
        QVariant sectionData = m_data[section];
        if (sectionData.canConvert<QVariantMap>()) {
            QVariantMap sectionMap = sectionData.toMap();
            sectionMap.remove(key);
            m_data[section] = sectionMap;
        }
    }
}

void XmlConfig::clear()
{
    m_data.clear();
    m_doc.clear();
    
    // 重新创建根元素
    QDomElement root = m_doc.createElement("config");
    m_doc.appendChild(root);
}

// 支持多层文件架构，接口实现
QVariant XmlConfig::getValueByPath(const QString& path, const QVariant& defaultValue)
{
    QStringList pathParts = path.split('/', QString::SkipEmptyParts);
    if (pathParts.isEmpty()) {
        return defaultValue;
    }
    
    return getValueFromPath(m_data, pathParts, 0);
}

void XmlConfig::setValueByPath(const QString& path, const QVariant& value)
{
    QStringList pathParts = path.split('/', QString::SkipEmptyParts);
    if (pathParts.isEmpty()) {
        return;
    }
    
    setValueToPath(m_data, pathParts, 0, value);
}

bool XmlConfig::containsPath(const QString& path)
{
    QStringList pathParts = path.split('/', QString::SkipEmptyParts);
    if (pathParts.isEmpty()) {
        return false;
    }
    
    return containsPath(m_data, pathParts, 0);
}

void XmlConfig::removePath(const QString& path)
{
    QStringList pathParts = path.split('/', QString::SkipEmptyParts);
    if (pathParts.isEmpty()) {
        return;
    }
    
    removePath(m_data, pathParts, 0);
}

QVariantMap XmlConfig::getSubTree(const QString& path)
{
    QStringList pathParts = path.split('/', QString::SkipEmptyParts);
    if (pathParts.isEmpty()) {
        return m_data;
    }
    
    return getSubTree(m_data, pathParts, 0);
}

QStringList XmlConfig::getAllPaths(const QString& prefix)
{
    QStringList result;
    getAllPaths(m_data, prefix, result);
    return result;
}

// 递归工具函数实现（与JsonConfig相同）
QVariant XmlConfig::getValueFromPath(const QVariantMap& data, const QStringList& pathParts, int index)
{
    if (index >= pathParts.size()) {
        return QVariant();
    }
    
    QString currentKey = pathParts[index];
    if (!data.contains(currentKey)) {
        return QVariant();
    }
    
    QVariant currentValue = data[currentKey];
    
    if (index == pathParts.size() - 1) {
        // 到达路径末尾，返回当前值
        return currentValue;
    } else {
        // 继续递归遍历
        if (currentValue.canConvert<QVariantMap>()) {
            return getValueFromPath(currentValue.toMap(), pathParts, index + 1);
        } else {
            return QVariant();
        }
    }
}

void XmlConfig::setValueToPath(QVariantMap& data, const QStringList& pathParts, int index, const QVariant& value)
{
    if (index >= pathParts.size()) {
        return;
    }
    
    QString currentKey = pathParts[index];
    
    if (index == pathParts.size() - 1) {
        // 到达路径末尾，设置值
        data[currentKey] = value;
    } else {
        // 继续递归遍历
        if (!data.contains(currentKey) || !data[currentKey].canConvert<QVariantMap>()) {
            // 创建中间节点
            data[currentKey] = QVariantMap();
        }
        
        QVariantMap subData = data[currentKey].toMap();
        setValueToPath(subData, pathParts, index + 1, value);
        data[currentKey] = subData;
    }
}

bool XmlConfig::containsPath(const QVariantMap& data, const QStringList& pathParts, int index)
{
    if (index >= pathParts.size()) {
        return false;
    }
    
    QString currentKey = pathParts[index];
    if (!data.contains(currentKey)) {
        return false;
    }
    
    if (index == pathParts.size() - 1) {
        return true;
    } else {
        QVariant currentValue = data[currentKey];
        if (currentValue.canConvert<QVariantMap>()) {
            return containsPath(currentValue.toMap(), pathParts, index + 1);
        } else {
            return false;
        }
    }
}

void XmlConfig::removePath(QVariantMap& data, const QStringList& pathParts, int index)
{
    if (index >= pathParts.size()) {
        return;
    }
    
    QString currentKey = pathParts[index];
    if (!data.contains(currentKey)) {
        return;
    }
    
    if (index == pathParts.size() - 1) {
        // 到达路径末尾，删除节点
        data.remove(currentKey);
    } else {
        QVariant currentValue = data[currentKey];
        if (currentValue.canConvert<QVariantMap>()) {
            QVariantMap subData = currentValue.toMap();
            removePath(subData, pathParts, index + 1);
            
            if (subData.isEmpty()) {
                data.remove(currentKey);
            } else {
                data[currentKey] = subData;
            }
        }
    }
}

QVariantMap XmlConfig::getSubTree(const QVariantMap& data, const QStringList& pathParts, int index)
{
    if (index >= pathParts.size()) {
        return data;
    }
    
    QString currentKey = pathParts[index];
    if (!data.contains(currentKey)) {
        return QVariantMap();
    }
    
    QVariant currentValue = data[currentKey];
    if (currentValue.canConvert<QVariantMap>()) {
        return getSubTree(currentValue.toMap(), pathParts, index + 1);
    } else {
        return QVariantMap();
    }
}

void XmlConfig::getAllPaths(const QVariantMap& data, const QString& currentPath, QStringList& result)
{
    for (auto it = data.begin(); it != data.end(); ++it) {
        QString newPath = currentPath.isEmpty() ? it.key() : currentPath + "/" + it.key();
        
        QVariant value = it.value();
        if (value.canConvert<QVariantMap>()) {
            // 如果是映射，递归遍历
            getAllPaths(value.toMap(), newPath, result);
        } else {
            // 如果是叶子节点，添加到结果
            result.append(newPath);
        }
    }
}

// XML转换，包括属性(atrribute)和复杂结构比如有很多层/config/serialconfig/cominfo/com,类似这种啥的
QVariantMap XmlConfig::domElementToVariantMap(const QDomElement& element)
{
    QVariantMap result;
    
    // 处理属性
    QDomNamedNodeMap attributes = element.attributes();
    for (int i = 0; i < attributes.count(); ++i) {
        QDomAttr attr = attributes.item(i).toAttr();
        if (!attr.isNull()) {
            QString attrName = "@" + attr.name();
            result[attrName] = attr.value();
        }
    }
    
    // 处理子元素
    QDomNodeList childNodes = element.childNodes();
    
    // 检查是否有多个相同名称的子元素（列表结构）
    QMap<QString, int> childCount;
    for (int i = 0; i < childNodes.count(); ++i) {
        QDomElement childElement = childNodes.at(i).toElement();
        if (!childElement.isNull()) {
            childCount[childElement.tagName()]++;
        }
    }
    
    for (int i = 0; i < childNodes.count(); ++i) {
        QDomNode childNode = childNodes.at(i);
        
        if (childNode.isComment()) {
            continue; // 跳过注释
        }
        
        if (childNode.isText()) {
            // 处理文本内容
            QString text = childNode.toText().data().trimmed();
            if (!text.isEmpty()) {
                result["#text"] = text;
            }
            continue;
        }
        
        QDomElement childElement = childNode.toElement();
        if (childElement.isNull()) {
            continue;
        }
        
        QString tagName = childElement.tagName();
        
        // 检查是否有多个相同名称的子元素
        if (childCount[tagName] > 1) {
            // 处理列表结构
            if (!result.contains(tagName)) {
                result[tagName] = QVariantList();
            }
            
            QVariantList list = result[tagName].toList();
            
            // 检查子元素是否有子元素或属性
            if (childElement.hasChildNodes() || childElement.attributes().count() > 0) {
                list.append(domElementToVariantMap(childElement));
            } else {
                list.append(domElementValueToVariant(childElement));
            }
            
            result[tagName] = list;
        } else {
            // 单个子元素
            if (childElement.hasChildNodes() || childElement.attributes().count() > 0) {
                // 有子元素或属性，递归处理
                result[tagName] = domElementToVariantMap(childElement);
            } else {
                // 没有子元素，作为叶子节点处理
                result[tagName] = domElementValueToVariant(childElement);
            }
        }
    }
    
    return result;
}

//转换variantmap为domelement
void XmlConfig::variantMapToDomElement(const QVariantMap& variantMap, QDomElement& parentElement, QDomDocument& doc)
{
    for (auto it = variantMap.begin(); it != variantMap.end(); ++it) {
        QString key = it.key();
        QVariant value = it.value();
        
        if (key.startsWith("@")) {
            // 处理属性
            QString attrName = key.mid(1);
            parentElement.setAttribute(attrName, value.toString());
        } else if (key == "#text") {
            // 处理文本内容
            QDomText textNode = doc.createTextNode(value.toString());
            parentElement.appendChild(textNode);
        } else {
            // 处理子元素
            if (value.canConvert<QVariantList>()) {
                // 列表结构
                QVariantList list = value.toList();
                for (const QVariant& item : list) {
                    variantToDomElement(item, parentElement, doc, key);
                }
            } else {
                variantToDomElement(value, parentElement, doc, key);
            }
        }
    }
}


QVariant XmlConfig::domElementValueToVariant(const QDomElement& element)
{
    QString text = element.text().trimmed();
    
    // 尝试转换为合适的类型
    if (text.isEmpty()) {
        return QVariant();
    }
    
    bool ok;
    int intValue = text.toInt(&ok);
    if (ok) {
        return intValue;
    }
    
    double doubleValue = text.toDouble(&ok);
    if (ok) {
        return doubleValue;
    }
    
    if (text.toLower() == "true" || text.toLower() == "false") {
        return (text.toLower() == "true");
    }
    
    return text;
}

void XmlConfig::variantToDomElement(const QVariant& variant, QDomElement& parentElement, QDomDocument& doc, const QString& tagName)
{
    QDomElement element = doc.createElement(tagName);
    
    if (variant.canConvert<QVariantMap>()) {
        // 如果是映射，递归处理
        QVariantMap map = variant.toMap();
        variantMapToDomElement(map, element, doc);
    } else {
        // 基本类型，设置文本内容
        QDomText textNode = doc.createTextNode(variant.toString());
        element.appendChild(textNode);
    }
    
    parentElement.appendChild(element);
}

//加载数据从xml文件
void XmlConfig::loadDataFromXml()
{
    m_data.clear();
    
    QDomElement root = m_doc.documentElement();
    if (root.isNull()) {
        return;
    }
    
    m_data = domElementToVariantMap(root);
}

//保存数据到xml文件
void XmlConfig::saveDataToXml()
{
    m_doc.clear();
    
    QDomElement root = m_doc.createElement("config");
    m_doc.appendChild(root);
    
    variantMapToDomElement(m_data, root, m_doc);
}
