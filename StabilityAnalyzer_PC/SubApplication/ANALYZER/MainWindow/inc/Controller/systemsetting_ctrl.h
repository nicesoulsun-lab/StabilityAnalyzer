#ifndef SYSTEMSETTING_CTRL_H
#define SYSTEMSETTING_CTRL_H

#include <QObject>
#include <QTranslator>
#include <QQmlEngine>
#include <QThread>
#include <QTimer>
#include <QProcess>
#include "mainwindow_global.h"
class SystemData;
class MAINWINDOW_EXPORT systemSettingCtrl : public QObject
{
    Q_OBJECT

    // QML 绑定的属性：当前信号强度字符串 (如 "-65")
    Q_PROPERTY(QString wifiIntensity READ wifiIntensity NOTIFY wifiIntensityChanged)
    Q_PROPERTY(QString wifiConnected READ connectedSsid NOTIFY wifiConnectedChanged)

public:
    explicit systemSettingCtrl(QObject *parent = nullptr);
    ~systemSettingCtrl();

    // 语言
    Q_INVOKABLE void switchLanguage(QString language);
    // 供 QML 读取当前语言
    Q_INVOKABLE QString currentLanguage() const;
    bool isChinese() const;

    // 亮度
    Q_INVOKABLE void switchBrightness(int brightness);
    Q_INVOKABLE int currentBrightness() const;

    // WiFi
    Q_INVOKABLE void getWifiNameAsync(int mode = 0); //0为初始化获取 1为刷新获取
    Q_INVOKABLE void connectWifi(QString ssid, QString password); //连接wifi
    Q_INVOKABLE void verifyWifiConnection(const QString &targetSsid);
    Q_INVOKABLE void disconnectWifi(); //断开wifi

    Q_INVOKABLE void startMonitoring();
    Q_INVOKABLE void stopMonitoring();

    QString wifiIntensity() const { return m_wifiIntensity; }
    QString connectedSsid() const { return m_connectedSsid; }

    // 日期时间
    Q_INVOKABLE QString getCurrentTimeInTimezone();
    void syncSystemTime();
    // 更新日期时间
    Q_INVOKABLE void updateDateTime(QString);
    Q_INVOKABLE QString getDateTime();

    // 系统升级
    Q_INVOKABLE void update_system();

    Q_INVOKABLE QString getSerialNumber();
signals:
    // 语言切换完成信号
    void languageChanged_Signal();

    // 设置加载完成信号
    void loadedSetting_Signal();

    // 发送电池电量信息
    // \param iaCharging 是否正在充电
    // \param level 电池电量百分比
    void sendBatteryLevel_Signal(bool iaCharging, int level);

    // 发送更新信息
    // \param info 更新信息字符串
    void updateInfo(QString);

    // 发送位置状态
    // \param enabled 位置服务是否启用
    void sendLocationStatus_Signal(bool);

    // 已是最新版本信号
    void beLatestVersion();

    // 语言切换信号，触发 QML 界面自动更新
    void languageChanged();

    // WiFi 扫描完成信号
    // \param networkList WiFi 网络列表
    void scanCompleted(const QVariantList &networkList);

    // 系统升级完成信号
    // \param success 是否成功
    void upgradeCompleted(bool success);

    // 发送显示消息信号
    // \param msg 要显示的消息
    void send_show_msg(const QString &msg);

    // WiFi 列表就绪信号
    // \param mode 模式（0-初始化，1-刷新）
    // \param list WiFi 列表
    void wifiListReady(int mode, const QStringList& list);

    // WiFi 信号强度变化信号
    void wifiIntensityChanged();

    // WiFi 连接状态变化信号
    void wifiConnectedChanged();

private slots:
    void onMonitorTimeout();

    // 验证连接 (iwgetid) 完成回调
    void onVerifyConnectionFinished(int exitCode, QProcess::ExitStatus exitStatus);

    // 获取信号强度完成回调
    void onGetSignalFinished(int exitCode, QProcess::ExitStatus exitStatus);

    void handleWifiStatusUpdate(const QString &ssid, const QString &signal);

private:
    void loadLanguage(QString language);    // 设置语言

private:
    QQmlEngine m_engine;
    QTranslator *m_appTranslator;
    QString m_currentLanguage; // 存储当前语言

    QString m_version;

    int m_brightness;   //背光亮度

    SystemData *m_systemdata;

    QString m_wifiIntensity;
    QString m_tempSsidForSignalCheck;
    QString m_connectedSsid;

    // WiFi监测
    bool m_isMonitoring = false;
    QTimer *m_monitorTimer;
    QProcess *m_verifyProcess;
    QProcess *m_signalProcess;

    QTimer *m_winSimTimer;
};

#endif // SYSTEMSETTING_CTRL_H
