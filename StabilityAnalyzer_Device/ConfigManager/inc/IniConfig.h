#ifndef INICONFIG_H
#define INICONFIG_H

#include "IConfigInterface.h"
#include <QSettings>
#include <QMap>
/**
 * ini配置文件
 * */
class CONFIGMANAGER_EXPORT IniConfig : public IConfigInterface
{
public:
    IniConfig();
    ~IniConfig() override;
    
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

private:
    QSettings* m_settings;
    QString m_currentFilePath;
    QMap<QString, QMap<QString, QVariant>> m_data;
    
    void loadDataFromSettings();
    void saveDataToSettings();
};

#endif // INICONFIG_H
