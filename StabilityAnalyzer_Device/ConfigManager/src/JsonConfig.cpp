#include "inc/JsonConfig.h"
#include <QFile>
#include <QTextStream>
#include <QJsonArray>
#include <QDebug>
#include <QStringList>
/**
 * 读取json文件
 * 支持多层文件架构
**/
JsonConfig::JsonConfig()
{
    m_doc.setObject(QJsonObject());
}

//加载json文件
bool JsonConfig::load(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Failed to open JSON file:" << filePath;
        return false;
    }
    
    //读取文件内容
    QTextStream in(&file);
    in.setCodec("UTF-8");
    QString content = in.readAll();
    file.close();
    
    //转换为json
    QJsonParseError parseError;
    m_doc = QJsonDocument::fromJson(content.toUtf8(), &parseError);
    
    if (parseError.error != QJsonParseError::NoError) {
        qWarning() << "Failed to parse JSON file:" << filePath
                  << "Error:" << parseError.errorString();
        return false;
    }
    
    if (m_doc.isNull() || !m_doc.isObject()) {
        qWarning() << "Invalid JSON format in file:" << filePath;
        return false;
    }
    
    m_currentFilePath = filePath;
    loadDataFromJson();
    return true;
}

//保存
bool JsonConfig::save(const QString& filePath)
{
    QString savePath = filePath.isEmpty() ? m_currentFilePath : filePath;
    
    if (savePath.isEmpty()) {
        qWarning() << "No file path specified for saving";
        return false;
    }
    
    saveDataToJson();
    
    QFile file(savePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "Failed to open JSON file for writing:" << savePath;
        return false;
    }
    
    QTextStream out(&file);
    out.setCodec("UTF-8");
    out << m_doc.toJson(QJsonDocument::Indented);
    file.close();
    
    return true;
}

QVariant JsonConfig::getValue(const QString& section, const QString& key, 
                             const QVariant& defaultValue)
{
    // 将section/key作为两级路径处理
    QString path = section + "/" + key;
    return getValueByPath(path, defaultValue);
}

void JsonConfig::setValue(const QString& section, const QString& key, 
                         const QVariant& value)
{
    // 将section/key作为两级路径处理
    QString path = section + "/" + key;
    setValueByPath(path, value);
}

QStringList JsonConfig::getSections()
{
    // 返回第一层键作为sections
    QStringList sections;
    for (auto it = m_data.begin(); it != m_data.end(); ++it) {
        sections.append(it.key());
    }
    return sections;
}

QStringList JsonConfig::getKeys(const QString& section)
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

bool JsonConfig::containsSection(const QString& section)
{
    return m_data.contains(section);
}

bool JsonConfig::containsKey(const QString& section, const QString& key)
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

void JsonConfig::removeSection(const QString& section)
{
    m_data.remove(section);
}

void JsonConfig::removeKey(const QString& section, const QString& key)
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

void JsonConfig::clear()
{
    m_data.clear();
    m_doc.setObject(QJsonObject());
}

// 根据路径获取键值
QVariant JsonConfig::getValueByPath(const QString& path, const QVariant& defaultValue)
{
    QStringList pathParts = path.split('/', QString::SkipEmptyParts);
    if (pathParts.isEmpty()) {
        return defaultValue;
    }
    
    return getValueFromPath(m_data, pathParts, 0);
}

//通过路径设置值
void JsonConfig::setValueByPath(const QString& path, const QVariant& value)
{
    QStringList pathParts = path.split('/', QString::SkipEmptyParts);
    if (pathParts.isEmpty()) {
        return;
    }
    
    setValueToPath(m_data, pathParts, 0, value);
}

bool JsonConfig::containsPath(const QString& path)
{
    QStringList pathParts = path.split('/', QString::SkipEmptyParts);
    if (pathParts.isEmpty()) {
        return false;
    }
    
    return containsPath(m_data, pathParts, 0);
}

void JsonConfig::removePath(const QString& path)
{
    QStringList pathParts = path.split('/', QString::SkipEmptyParts);
    if (pathParts.isEmpty()) {
        return;
    }
    
    removePath(m_data, pathParts, 0);
}

QVariantMap JsonConfig::getSubTree(const QString& path)
{
    QStringList pathParts = path.split('/', QString::SkipEmptyParts);
    if (pathParts.isEmpty()) {
        return m_data;
    }
    
    return getSubTree(m_data, pathParts, 0);
}

QStringList JsonConfig::getAllPaths(const QString& prefix)
{
    QStringList result;
    getAllPaths(m_data, prefix, result);
    return result;
}

// 递归，根据路径获取值
QVariant JsonConfig::getValueFromPath(const QVariantMap& data, const QStringList& pathParts, int index)
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

//根据路径设置值，一直遍历找到对应路径再设置，比如/user/name
void JsonConfig::setValueToPath(QVariantMap& data, const QStringList& pathParts, int index, const QVariant& value)
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

bool JsonConfig::containsPath(const QVariantMap& data, const QStringList& pathParts, int index)
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

void JsonConfig::removePath(QVariantMap& data, const QStringList& pathParts, int index)
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

QVariantMap JsonConfig::getSubTree(const QVariantMap& data, const QStringList& pathParts, int index)
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

void JsonConfig::getAllPaths(const QVariantMap& data, const QString& currentPath, QStringList& result)
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

// JSON转换
QVariant JsonConfig::jsonValueToVariant(const QJsonValue& jsonValue)
{
    switch (jsonValue.type()) {
    case QJsonValue::Bool:
        return jsonValue.toBool();
    case QJsonValue::Double:
        return jsonValue.toDouble();
    case QJsonValue::String:
        return jsonValue.toString();
    case QJsonValue::Array:
        return jsonValue.toArray().toVariantList();
    case QJsonValue::Object:
        return jsonObjectToVariantMap(jsonValue.toObject());
    case QJsonValue::Null:
        return QVariant();
    default:
        return jsonValue.toVariant();
    }
}

QJsonValue JsonConfig::variantToJsonValue(const QVariant& variant)
{
    switch (variant.type()) {
    case QVariant::Bool:
        return QJsonValue(variant.toBool());
    case QVariant::Int:
    case QVariant::UInt:
    case QVariant::LongLong:
    case QVariant::ULongLong:
        return QJsonValue(variant.toLongLong());
    case QVariant::Double:
        return QJsonValue(variant.toDouble());
    case QVariant::String:
        return QJsonValue(variant.toString());
    case QVariant::List:
        return QJsonValue(QJsonArray::fromVariantList(variant.toList()));
    case QVariant::Map:
        return QJsonValue(variantMapToJsonObject(variant.toMap()));
    default:
        return QJsonValue(variant.toString());
    }
}
//转换json为qvariantmap
QVariantMap JsonConfig::jsonObjectToVariantMap(const QJsonObject& jsonObject)
{
    QVariantMap result;
    for (auto it = jsonObject.begin(); it != jsonObject.end(); ++it) {
        result[it.key()] = jsonValueToVariant(it.value());
    }
    return result;
}
//转换variantmap为json
QJsonObject JsonConfig::variantMapToJsonObject(const QVariantMap& variantMap)
{
    QJsonObject result;
    for (auto it = variantMap.begin(); it != variantMap.end(); ++it) {
        result[it.key()] = variantToJsonValue(it.value());
    }
    return result;
}

//加载数据从json文件
void JsonConfig::loadDataFromJson()
{
    m_data.clear();
    
    //加载文件内容，转为map
    QJsonObject rootObj = m_doc.object();
    m_data = jsonObjectToVariantMap(rootObj);
}

//保存数据到json文件
void JsonConfig::saveDataToJson()
{
    QJsonObject rootObj = variantMapToJsonObject(m_data);
    m_doc.setObject(rootObj);
}

