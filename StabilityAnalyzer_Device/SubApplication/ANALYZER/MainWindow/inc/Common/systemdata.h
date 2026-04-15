#ifndef SYSTEMDATA_H
#define SYSTEMDATA_H

#include <QObject>
#include <QVector>
#include <QMap>
#include <QMutex>
#include <QDebug>
#include <QDateTime>

// 系统设置结构体
struct SystemSetting{
    QString language = "zh_CN"  ;
    int brightness   = 100      ;

    //VERSION
    QString version = "1.0.0";
};


// 用于管理系统数据
class SystemData : public QObject
{
    Q_OBJECT
public:
    explicit SystemData(QObject *parent = nullptr);
    ~SystemData();

    static SystemData* globalSystemData();

    // 加载系统配置信息
    bool LoadSystemConfig(const QString& filePath);

    //保存系统配置到文件
    bool SaveSystemConfig(const QString& filePath);

    //恢复默认配置
    void resetFactory();

    // 更新系统配置
    void UpdateSystemConfig(const SystemSetting& config);

    //获取系统配置信息
    const SystemSetting& GetSystemConfig(){return m_systemConfig; }

    QString currentLanguage();

    // 获取时间函数
    static QString getCurrentTimeInTimezone();

signals:


public slots:
    void UpdateSystemSetting_language(QString);
    void UpdateSystemSetting_brightness(int);
    void UpdateSystemSetting_version(QString);

    bool RestartApplication();

private:
    SystemData(const SystemData &) = delete ;
    const SystemData &operator=(const SystemData &) = delete ;

    void UpdateSysSettingConfigRow(const QString& paraname,const QVariant &paravalue);

    QMutex m_mutex;

    SystemSetting m_systemConfig;        //系统配置信息
    QString m_systemSetPath;

};

#endif // SYSTEMDATA_H
