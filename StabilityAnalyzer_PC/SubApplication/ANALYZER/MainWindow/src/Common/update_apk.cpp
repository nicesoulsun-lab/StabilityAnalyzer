
#include <QDebug>
#include <QUrl>
#include <QTimer>
#include <QProcess>
#include <QApplication>
#include <QVersionNumber>
#include "inc/Common/systemdata.h"

#include "inc/Common/update_apk.h"

#ifdef OS_WINDOW
#include <windows.h>
#else
#include <sys/stat.h>
#endif

// 初始化静态成员
QScopedPointer<update_apk> update_apk::instance;
QMutex update_apk::mutex;

// 获取单例实例
update_apk* update_apk::getInstance(QObject *parent)
{
    if (instance.isNull()) {
        QMutexLocker locker(&mutex);
        if (instance.isNull()) {
            instance.reset(new update_apk(parent));
        }
    }
    return instance.data();
}

update_apk::update_apk(QObject *parent) : QObject(parent)
{
    auto_extract = true; // 默认自动解压
}

// 获取文件列表
void update_apk::get_file_list()
{
    // m_user_name = username;
    // m_password = password;
    QNetworkAccessManager *manager = new QNetworkAccessManager(this);

    // // 构建请求URL
    // QString urlStr = QString("http://manage.hengmeierp.com/api/project/produceApk/appList?"
    //                          "username=%1&password=%2")
    //                      .arg(username)
    //                      .arg(password);


    const QString urlStr = QString("http://manage.hengmeierp.com/api/project/produceApkRela/getByApp?updateAPK=e4fec07c-8917-44ca-99f5-582daa869f02&acCode=%1").arg(c_apk_instrument_no);


    QUrl url(urlStr);
    QNetworkRequest request(url);

    // 设置请求头
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    qDebug() << "正在请求文件列表:" << url.toString();

    QNetworkReply *reply = manager->get(request);

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        this->on_apk_version_received(reply);
    });

    connect(reply, QOverload<QNetworkReply::NetworkError>::of(&QNetworkReply::error),
            this, &update_apk::on_network_error);
}

void update_apk::on_apk_version_received(QNetworkReply *reply)
{
    if (reply->error() != QNetworkReply::NoError) {
        qDebug() << "获取文件列表失败:" << reply->errorString();
        reply->deleteLater();
        return;
    }

    QByteArray responseData = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(responseData);

    if (doc.isNull()) {
        qDebug() << "JSON解析失败";
        emit error_occurred(400, tr("服务器响应格式错误"));
        reply->deleteLater();
        return;
    }

    QJsonObject jsonObj = doc.object();

    // 检查响应码
    if (jsonObj["code"].toInt() != 200) {
        QString msg = jsonObj["msg"].toString();
        qDebug() << "API返回错误:" << msg;
        // 【新增】发射错误信号，以便前端关闭loading并显示提示
        emit error_occurred(jsonObj["code"].toInt(), msg);
        reply->deleteLater();
        return;
    }

    QJsonObject dataObj = jsonObj["data"].toObject();

    QString oss_url = dataObj["apkOssUrl"].toString();
    QString revision = dataObj["apkRevision"].toString();

    QString currentVersion = SystemData::globalSystemData()->GetSystemConfig().version;

    qDebug() << "远程版本:" << revision << "下载地址:" << oss_url;
    qDebug() << "当前版本:" << currentVersion;

    if(is_version_newer(revision,currentVersion)){
        m_version = revision;
        m_oss_url = oss_url;
        emit send_new_version(revision);
    }else{
        emit error_occurred(0,tr("已是最新版本"));
        return;
    }

    reply->deleteLater();
}

void update_apk::on_apk_list_received(QNetworkReply *reply)
{
    if (reply->error() != QNetworkReply::NoError) {
        qDebug() << "获取文件列表失败:" << reply->errorString();
        reply->deleteLater();
        return;
    }

    QByteArray responseData = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(responseData);

    if (doc.isNull()) {
        qDebug() << "JSON解析失败";
        emit error_occurred(400, tr("服务器响应格式错误"));
        reply->deleteLater();
        return;
    }

    QJsonObject jsonObj = doc.object();

    // 检查响应码
    if (jsonObj["code"].toInt() != 200) {
        QString msg = jsonObj["msg"].toString();
        qDebug() << "API返回错误:" << msg;
        // 【新增】发射错误信号
        emit error_occurred(jsonObj["code"].toInt(), msg);
        reply->deleteLater();
        return;
    }

    // 解析文件列表
    QJsonArray rows = jsonObj["rows"].toArray();
    qDebug() << "找到" << rows.size() << "个文件";

    QStringList now_revision_list =  c_apk_version.split(".");
    // 将ossUrl和revision存储到QList<QStringList>中
    for (const QJsonValue &value : rows) {
        QJsonObject fileObj = value.toObject();
        QString oss_url = fileObj["ossUrl"].toString();
        QString revision = fileObj["revision"].toString();

        if (!oss_url.isEmpty()) {
            QStringList new_revision_list =  revision.split(".");

            if(new_revision_list[0]== now_revision_list[0]){
                if(revision>c_apk_version){
                    m_version = revision;
                    m_oss_url = oss_url;
                    emit send_new_version(revision);
                }else{
                    emit error_occurred(0,tr("已是最新版本"));
                }
                return;
            }
        }
    }
    emit error_occurred(1,tr("没有历史版本"));
    reply->deleteLater();
}

void update_apk::download_single_file()
{
    // [Debug 1] 确认函数被调用，且 URL 不为空
    qDebug() << "=============================================";
    qDebug() << "准备开始下载...";
    qDebug() << "下载地址 (URL):" << m_oss_url;

    if (m_oss_url.isEmpty()) {
        qDebug() << "错误: URL 为空，无法下载！";
        return;
    }

    QNetworkAccessManager *manager = new QNetworkAccessManager(this);
    QNetworkRequest request(m_oss_url);
    QUrl url(m_oss_url);

    QNetworkReply *reply = manager->get(request);

    // [Debug 2] 关键！监听下载进度信号
    connect(reply, &QNetworkReply::downloadProgress, this,
            [](qint64 bytesReceived, qint64 bytesTotal) {
        if (bytesTotal > 0) {
            // double percent = (double)bytesReceived / bytesTotal * 100.0;
        } else {
            qDebug() << "下载中 (未知总大小):" << bytesReceived;
        }
    });

    // 监听 SSL 错误
    connect(reply, &QNetworkReply::sslErrors, this, [](const QList<QSslError> &errors){
        for(const auto &err : errors) {
            qDebug() << "SSL 错误:" << err.errorString();
        }
    });

    QObject::connect(reply, &QNetworkReply::finished, [this, reply, url]() {
        qDebug() << "网络请求结束信号触发 (Finished)";

        bool success = false;
        QString file_path;
        QString file_name;

        // [Debug 3] 区分是网络错，还是文件保存错
        if (reply->error() == QNetworkReply::NoError) {
            qDebug() << "网络下载成功，准备写入磁盘...";

            // 获取文件名
            file_name = QFileInfo(url.path()).fileName();
            if (file_name.isEmpty()) file_name = "update_package.zip"; // 防止空文件名

            file_path = "./" + file_name;
            qDebug() << "目标保存路径:" << file_path;

            QFile file(file_path);

            if (file.open(QIODevice::WriteOnly)) {
                qint64 bytesWritten = file.write(reply->readAll());
                file.close();
                qDebug() << "文件写入完成，写入字节数:" << bytesWritten;

                if (bytesWritten > 0) {
                    success = true;
                } else {
                    qDebug() << "警告: 写入了 0 字节文件！";
                }

                // 处理下载的文件（如果是ZIP则解压）
                if (success && auto_extract && file_name.endsWith(".zip", Qt::CaseInsensitive)) {
                    qDebug() << "检测到 ZIP 文件，准备解压...";
                    process_downloaded_file(file_path);
                } else {
                    qDebug() << "无需解压或非 ZIP 文件。AutoExtract:" << auto_extract;
                }

            } else {
                qDebug() << "严重错误: 无法打开文件进行写入!" << file_path << "错误信息:" << file.errorString();
                emit error_occurred(2, tr("无法保存文件: ") + file.errorString());
            }
        } else {
            qDebug() << "网络请求发生错误:" << reply->errorString();
            qDebug() << "错误代码:" << reply->error();
            emit error_occurred(2, reply->errorString());
        }

        reply->deleteLater();
        if(reply->manager()) {
            reply->manager()->deleteLater();
        }
    });
}


void update_apk::on_network_error(QNetworkReply::NetworkError code)
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    if (reply) {
        emit error_occurred(1, reply->errorString());
    }
}

// 处理下载完成的文件
void update_apk::process_downloaded_file(const QString &filePath)
{
    qDebug() << "开始处理下载的文件:" << filePath;

    // 提取目录与文件同名（不带扩展名）
    QFileInfo fileInfo(filePath);
    QString extractDir = fileInfo.path() + "/" + fileInfo.completeBaseName();

    // 解压 ZIP 文件
    if (extract_zip(filePath, extractDir)) {
        qDebug() << "解压成功，正在查找 H3 文件夹...";

        QDir dir(extractDir);
        QString h1Path;

        // 1. 先查找 H3 文件夹
        QFileInfoList subDirs = dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
        for (const QFileInfo &subDir : subDirs) {
            if (subDir.fileName() == "H3") {
                h1Path = subDir.absoluteFilePath();
                break;
            }
        }

        if (!h1Path.isEmpty()) {
            qDebug() << "找到 H3 文件夹:" << h1Path;

            // 2. 删除除 H3 以外的所有文件和文件夹
            qDebug() << "正在清理非必要文件...";

            // 获取所有条目 (包括文件和文件夹)
            QFileInfoList allEntries = dir.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot);

            for (const QFileInfo &entry : allEntries) {
                if (entry.fileName() != "H3") {
                    bool removed = false;
                    if (entry.isDir()) {
                        // 递归删除文件夹
                        removed = QDir(entry.absoluteFilePath()).removeRecursively();
                    } else {
                        // 删除文件
                        removed = QFile::remove(entry.absoluteFilePath());
                    }

                    if (removed) {
                        qDebug() << "已删除:" << entry.absoluteFilePath();
                    } else {
                        qWarning() << "删除失败:" << entry.absoluteFilePath();
                    }
                }
            }

            qDebug() << "清理完成，开始执行更新脚本...";
            execute_update_script(h1Path);
        } else {
            qDebug() << "错误: 未在解压目录中找到 H3 文件夹";
            emit error_occurred(3, tr("未找到 H3 文件夹，更新无法继续"));

            // 如果没找到 H3，通常意味着包结构错误，可以选择清空整个解压目录
            if (dir.removeRecursively()) {
                qDebug() << "已清空无效的解压目录:" << extractDir;
            }
        }
    } else {
        qDebug() << "解压失败，无法继续处理";
    }
}

bool update_apk::is_version_newer(const QString &v1, const QString &v2)
{
    // 移除前缀v/V，移除空格
    QString cleanedV1 = v1.trimmed().toLower();
    if (cleanedV1.startsWith('v')) {
        cleanedV1 = cleanedV1.mid(1);
    }
    QString cleanedV2 = v2.trimmed().toLower();
    if (cleanedV2.startsWith('v')) {
        cleanedV2 = cleanedV2.mid(1);
    }
    QVersionNumber version1 = QVersionNumber::fromString(cleanedV1);
    QVersionNumber version2 = QVersionNumber::fromString(cleanedV2);
    return version1 > version2;
}

// 解压ZIP文件
bool update_apk::extract_zip(const QString &zipFilePath, const QString &extractDir)
{
    QFileInfo zipFileInfo(zipFilePath);
    if (!zipFileInfo.exists()) {
        emit error_occurred(3, tr("zip文件不存在"));
        return false;
    }

    // 创建解压目录
    QDir dir(extractDir);
    if (!dir.exists()) {
        if (!dir.mkpath(".")) {
            emit error_occurred(3, tr("无法创建解压目录"));
            return false;
        }
    }

    bool success = false;

    if (is_windows()) {
        // Windows: 使用PowerShell解压
        QProcess process;
        QStringList args;
        args << "-Command"
             << QString("Expand-Archive -Path '%1' -DestinationPath '%2' -Force")
                .arg(zipFilePath).arg(extractDir);

        process.start("powershell", args);
        if (process.waitForFinished(30000)) { // 30秒超时
            success = (process.exitCode() == 0);
            if (!success) {
                emit error_occurred(3, tr("解压失败"));
            }
        } else {
            emit error_occurred(3, tr("解压安装包超时"));
        }
    } else {
        QProcess process;
        QStringList args;
        args << "-o" << zipFilePath << "-d" << extractDir;

        process.start("unzip", args);
        if (process.waitForFinished(30000)) { // 30秒超时
            success = (process.exitCode() == 0);
            if (!success) {
                emit error_occurred(3, tr("解压失败"));
            }
        } else {
            emit error_occurred(3, tr("解压安装包超时"));
        }
    }

    return success;
}

// 执行更新脚本 - 使用系统命令而不是QProcess
bool update_apk::execute_update_script(const QString &extractDir)
{
    QString scriptPath = find_script_file(extractDir);
    if (scriptPath.isEmpty()) {
        emit error_occurred(4, tr("未找到更新脚本文件"));
        return false;
    }


    bool success = false;
    if (is_windows()) {
        // 正确方式：用 powershell 执行脚本（不阻塞当前进程）
        QString cmd = QString("start \"UpdateScript\" powershell -ExecutionPolicy Bypass -NoProfile -File \"%1\"")
                .arg(scriptPath);

        int result = system(cmd.toUtf8().constData());
        success = (result == 0);

        if (success) {
            QTimer::singleShot(100, [](){
                qDebug() << "Main program exiting for update...";
                QApplication::quit();
            });
            qDebug() << "已启动更新脚本:" << scriptPath;
        } else {
            emit error_occurred(4, tr("启动 PowerShell 脚本失败"));
        }
    }else{
        // 在Linux下设置执行权限
        if (!set_script_permissions(scriptPath)) {
            emit error_occurred(4, tr("无法设置脚本执行权限"));
            return false;
        }

        // 使用系统命令执行脚本，不等待完成
        QString command = QString("nohup %1 > /dev/null 2>&1 &").arg(scriptPath);
        int result = system(command.toUtf8().constData());
        success = (result == 0);

        if (success) {
            qDebug() << "✓ 已启动脚本执行:" << scriptPath;

            SystemData::globalSystemData()->UpdateSystemSetting_version(m_version);

            emit send_msg(tr("下载完成，即将重启"));
            // QTimer::singleShot(100, [](){
            //     qDebug() << "Main program exiting for update...";
            //     QApplication::quit();
            // });
        } else {
            emit error_occurred(4, tr("启动脚本失败"));
        }
    }
    return success;
}

// 查找脚本文件
QString update_apk::find_script_file(const QString &extractDir) const
{
    QDir dir(extractDir);

    // 查找脚本文件
    QString scriptName;
    if (is_windows()) {
        scriptName = "update.ps1";//vb
    } else {
        scriptName = "update.sh";
    }

    if (scriptName.isEmpty()) {
        return QString();
    }

    // 在解压目录中查找脚本文件
    QStringList files = dir.entryList(QStringList() << scriptName, QDir::Files);
    if (!files.isEmpty()) {
        return dir.absoluteFilePath(files.first());
    }

    // 如果在根目录没找到，递归搜索子目录
    QStringList allFiles = dir.entryList(QDir::Files | QDir::NoDotAndDotDot);
    for (const QString &file : allFiles) {
        if (file == scriptName) {
            return dir.absoluteFilePath(file);
        }
    }

    // 搜索子目录
    QStringList subDirs = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QString &subDir : subDirs) {
        QString foundScript = find_script_file(dir.absoluteFilePath(subDir));
        if (!foundScript.isEmpty()) {
            return foundScript;
        }
    }

    return QString();
}

// 在Linux下设置脚本执行权限
bool update_apk::set_script_permissions(const QString &scriptPath)
{
#ifndef OS_WINDOW
    // 在Unix/Linux系统下设置执行权限
    struct stat st;
    if (stat(scriptPath.toUtf8().constData(), &st) == 0) {
        mode_t new_mode = st.st_mode | S_IXUSR | S_IXGRP | S_IXOTH;
        if (chmod(scriptPath.toUtf8().constData(), new_mode) == 0) {
            qDebug() << "已设置脚本执行权限:" << scriptPath;
            return true;
        }
    }
    return false;
#else
    // Windows不需要设置执行权限
    return true;
#endif
}

// 检查是否为Windows系统
bool update_apk::is_windows() const
{
#ifdef OS_WINDOW
    return true;
#else
    return false;
#endif
}
