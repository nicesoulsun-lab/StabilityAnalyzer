#ifndef APPLICATION_H
#define APPLICATION_H

#include <QObject>
#include <QVariantMap>
#include <QString>

class QQmlApplicationEngine;
class QQuickWindow;
class MainWindow;

namespace Ui {
class Application;
}

class Application : public QObject
{
    Q_OBJECT

public:
    explicit Application(QObject *parent = nullptr);
    ~Application();

    // 属性访问器
    QString appName() const;
    void setAppName(const QString &name);
    QString appVersion() const;
    void setAppVersion(const QString &version);
    bool isInitialized() const;

    // 应用程序管理
    void initializeApplication();
    void shutdownApplication();
    QVariantMap getSystemInfo();

    // 设置管理
    void saveSettings(const QVariantMap &settings);
    QVariantMap loadSettings();

    // 工具函数
    void showMessage(const QString &message, int duration = 0);
    bool checkPermission(const QString &permission);

signals:
    void appNameChanged();
    void appVersionChanged();
    void isInitializedChanged();
    void applicationStarted();
    void applicationStopped();
    void settingsChanged();
    void systemMessage(const QString &message, int type);

private slots:
//    void onModulesInitialized();

private:
    void initUI();
    void initializeModules();
    void cleanupModules();
    void loadConfiguration();
    void saveConfiguration();
    MainWindow* mainWindow();

private:
    QString m_appName;
    QString m_appVersion;
    bool m_isInitialized;
    QQmlApplicationEngine *m_engine;
    QQuickWindow *m_mainWindow;
    QVariantMap m_settings;
};

#endif // APPLICATION_H
