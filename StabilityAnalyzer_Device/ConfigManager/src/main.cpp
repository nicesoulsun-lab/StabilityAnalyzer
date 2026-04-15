#include <QCoreApplication>
#include "inc/ConfigManager.h"
#include <QDebug>
#include <QFile>
void testIniConfig()
{
    qDebug() << "=== Testing INI Config ===";

    ConfigManager manager;
    QString filePath = "D:/test/config.ini";

    // 测试加载和保存
    if (manager.load(filePath, ConfigManager::INI)) {
        qDebug() << "INI config loaded successfully";

        // 设置一些测试值
        manager.setValue("database1", "host", "localhost");
        manager.setValue("database", "port", 3306);
        manager.setValue("database", "enabled", true);

        manager.setValue("application", "name", "TestApp");
        manager.setValue("application", "version", "1.0.0");

        // 保存配置
        if (manager.save()) {
            qDebug() << "INI config saved successfully";
        }

        // 读取配置值
        qDebug() << "Database host:" << manager.getValue("database", "host").toString();
        qDebug() << "Database port:" << manager.getValue("database", "port").toInt();
        qDebug() << "Application name:" << manager.getValue("application", "name").toString();

        // 测试获取所有节和键
        qDebug() << "Sections:" << manager.getSections();
        qDebug() << "Database keys:" << manager.getKeys("database");

        // 测试删除功能
        manager.removeKey("database", "enabled");
        qDebug() << "After removing 'enabled' key:" << manager.getKeys("database");

        if (manager.save()) {
            qDebug() << "INI config saved successfully";
        }
        // 清理测试文件
//        QFile::remove(filePath);
    } else {
        qDebug() << "Failed to load INI config";
    }
}

void testXmlConfig()
{
    qDebug() << "\n=== Testing XML Config ===";

    ConfigManager manager;
    QString filePath = "D:/test/softconfig.xml";

    if (manager.load(filePath, ConfigManager::XML)) {
        qDebug() << "XML config loaded successfully";

        // 设置测试值
        manager.setValue("network1", "ip", "192.168.1.1");
        manager.setValue("network", "port", 8080);
        manager.setValue("network", "timeout", 30.5);

        manager.setValue("security", "ssl_enabled", true);
        manager.setValue("security", "cert_path", "/path/to/cert.pem");

        // 保存配置
        if (manager.save()) {
            qDebug() << "XML config saved successfully";
        }

        // 读取配置值
        qDebug() << "Network IP:" << manager.getValue("network", "ip").toString();
        qDebug() << "Network port:" << manager.getValue("network", "port").toInt();
        qDebug() << "Security SSL enabled:" << manager.getValue("security", "ssl_enabled").toBool();

        // 测试包含检查
        qDebug() << "Contains 'network' section:" << manager.containsSection("network");
        qDebug() << "Contains 'security/ssl_enabled':" << manager.containsKey("security", "ssl_enabled");

        // 清理测试文件
//        QFile::remove(filePath);
    } else {
        qDebug() << "Failed to load XML config";
    }
}

void testJsonConfig()
{
    qDebug() << "\n=== Testing JSON Config ===";

    ConfigManager manager;
    QString filePath = "D:/test/config1.json";

    if (manager.load(filePath, ConfigManager::JSON)) {
        qDebug() << "JSON config loaded successfully";

        // 设置测试值
        manager.setValue("user1", "name", "John Doe");
        manager.setValue("user", "age", 30);
        manager.setValue("user", "active", true);

        manager.setValue("preferences", "theme", "dark");
        manager.setValue("preferences", "language", "zh-CN");
        manager.setValue("preferences", "notifications", true);

        // 保存配置
        if (manager.save()) {
            qDebug() << "JSON config saved successfully";
        }

        // 读取配置值
        qDebug() << "User name:" << manager.getValue("user", "name").toString();
        qDebug() << "User age:" << manager.getValue("user", "age").toInt();
        qDebug() << "Preferences theme:" << manager.getValue("preferences", "theme").toString();

        // 测试格式检测
        qDebug() << "Detected format:" << manager.getCurrentFormat();
        qDebug() << "Current file path:" << manager.getCurrentFilePath();

        // 清理测试文件
//        QFile::remove(filePath);
    } else {
        qDebug() << "Failed to load JSON config";
    }
}

int main(int argc, char *argv[])
{
  QCoreApplication ca(argc, argv);
//  CONFIGMANAGER->load("D:/test/config.ini", ConfigManager::ConfigFormat::INI);
//  QVariant result;
//  CONFIGMANAGER->getValue("Communication","RetryCount", result);
//  qDebug()<<"获取到ini配置文件的键值：" << result;
//  CONFIGMANAGER->load("D:/test/config1.json", ConfigManager::ConfigFormat::JSON);
//  CONFIGMANAGER->getValue("serialConfig","portName",result);
//  qDebug()<<"获取到json配置文件的键值：" << result;
//  CONFIGMANAGER->load("D:/test/softconfig.xml", ConfigManager::ConfigFormat::XML);
//  CONFIGMANAGER->getValue("File_Config", "Site", result);
//  qDebug()<<"读取到xml文件的键值："<<result;
  testIniConfig();
  testJsonConfig();
  testXmlConfig();
  return ca.exec();
}
