#ifndef UPDATE_APK_H
#define UPDATE_APK_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDir>
#include <QList>
#include <QStringList>
#include <QProcess>
#include <QMutex>
#include <QScopedPointer>

const QString c_apk_version = "V1.0.7";
const QString c_apk_instrument_no = "00036";

class update_apk : public QObject
{
    Q_OBJECT

public:
    // 获取单例实例
    static update_apk* getInstance(QObject *parent = nullptr);

    // 删除拷贝构造函数和赋值运算符
    update_apk(const update_apk&) = delete;
    update_apk& operator=(const update_apk&) = delete;

    // 获取文件列表
    void get_file_list();

    // 下载单个文件
    void download_single_file();



signals:
    void send_new_version(QString new_version);
    void error_occurred(const int code,const QString &errorMessage);
    void send_msg(QString msg);



private slots:
    void on_apk_version_received(QNetworkReply *reply);
    void on_apk_list_received(QNetworkReply *reply);
    void on_network_error(QNetworkReply::NetworkError code);

private:
    // 私有构造函数
    explicit update_apk(QObject *parent = nullptr);

    //获取历史版本
    // void get_history_list();

    // 解压缩相关方法
    bool extract_zip(const QString &zipFilePath, const QString &extractDir);
    bool execute_update_script(const QString &extractDir);
    QString find_script_file(const QString &extractDir) const;
    bool set_script_permissions(const QString &scriptPath);
    bool is_windows() const;

    // 处理下载完成的文件
    void process_downloaded_file(const QString &filePath);

    bool is_version_newer(const QString &v1, const QString &v2);



    // 单例实例指针
    static QScopedPointer<update_apk> instance;
    static QMutex mutex;

    QString m_version = ""; // 存储revision
    QString m_oss_url = ""; // 存储ossUrl
    QString m_user_name;
    QString m_password;
    bool auto_extract;


};

#endif // UPDATE_APK_H
