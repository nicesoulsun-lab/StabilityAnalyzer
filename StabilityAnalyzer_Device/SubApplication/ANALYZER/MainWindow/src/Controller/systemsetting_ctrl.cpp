#include "inc/Controller/systemsetting_ctrl.h"
#include "SqlOrmManager.h"

#include <QFile>
#include <QDir>
#include <QTimer>
#include <QCoreApplication>
#include <QSharedPointer>
#include <QTimeZone>
#include <QThread>
#include <QProcess>
#include <QCoreApplication>
#include <QRegularExpression>
#include <QtConcurrent>
#include "inc/Common/systemdata.h"
#include "inc/Common/update_apk.h"

systemSettingCtrl::systemSettingCtrl(QObject *parent)
    : QObject(parent), m_appTranslator(new QTranslator(this))
{
    // 1. 获取应用程序所在目录 (即 /opt/ANALYZER/bin-mingw)
    QString appDir = QCoreApplication::applicationDirPath();

    // 2. 拼接配置文件完整路径: /opt/ANALYZER/bin-mingw/sys_config/config.ini
    // QDir::separator() 会自动处理 Linux(/) 和 Windows(\) 的差异
    QString configFilePath = appDir + QDir::separator() + "sys_config" + QDir::separator() + "config.ini";

    // 3. 获取单例实例并加载配置
    m_systemdata = SystemData::globalSystemData();

    // 4. 调用加载函数
    if (m_systemdata->LoadSystemConfig(configFilePath)) qDebug() << "系统配置加载成功:" << configFilePath;
    else qWarning() << "系统配置加载失败，将使用默认值或检查文件权限:" << configFilePath;

    m_currentLanguage = m_systemdata->GetSystemConfig().language;
    m_brightness = m_systemdata->GetSystemConfig().brightness;

    emit loadedSetting_Signal();

    this->loadLanguage(m_currentLanguage);

    this->switchBrightness(m_brightness);

    this->disconnectWifi();

    connect(this, &systemSettingCtrl::upgradeCompleted, m_systemdata, [this](bool succ){
        if(succ){
            QThread::sleep(3);
            m_systemdata->RestartApplication();
        }
    });

    //    code  0   已是最新版本
    //          1   获取版本信息失败
    //          2   下载过程中失败
    //          3   解压缩过程中失败
    //          4   启动脚本过程失败
    connect(update_apk::getInstance(),&update_apk::error_occurred,this,[this](const int code,const QString &errorMessage){
        qDebug()<<"系统升级失败 code:"<<code<<" errorMsg:"<<errorMessage;
        if(code == 0){
            emit beLatestVersion();
        }else{
            emit send_show_msg(tr("更新失败"));
        }
    });
    connect(update_apk::getInstance(),&update_apk::send_new_version,this,[this](QString new_version){
        emit send_show_msg(tr("开始下载"));
        update_apk::getInstance()->download_single_file();
    });

    connect(update_apk::getInstance(),&update_apk::send_msg,this,[this](QString msg){
        emit send_show_msg(msg);
    });

    // 配置监控定时器
    m_monitorTimer = new QTimer(this);
    m_monitorTimer->setInterval(3000); // 3 秒轮询一次
    connect(m_monitorTimer, &QTimer::timeout, this, &systemSettingCtrl::onMonitorTimeout);

    m_verifyProcess = new QProcess(this);
    m_signalProcess = new QProcess(this);
    connect(m_verifyProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &systemSettingCtrl::onVerifyConnectionFinished);

    connect(m_signalProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &systemSettingCtrl::onGetSignalFinished);
}

systemSettingCtrl::~systemSettingCtrl()
{
    QCoreApplication::removeTranslator(m_appTranslator);
}

void systemSettingCtrl::switchLanguage(QString language)
{
    m_currentLanguage = language;
    loadLanguage(m_currentLanguage);

    m_systemdata->UpdateSystemSetting_language(m_currentLanguage);

    emit languageChanged_Signal();
}

void systemSettingCtrl::loadLanguage(QString language)
{
    // 移除旧翻译器
    QCoreApplication::removeTranslator(m_appTranslator);

    QString langFile;
    if (language == "en_US") {
        langFile = ":/translations/app_en_US.qm";
    } else {
        langFile = ":/translations/app_zh_CN.qm";
    }

    // 尝试检查文件是否存在
    QFile resourceFile(langFile);
    bool exists = resourceFile.exists();
    qDebug() << "Resource file exists:" << exists;
    if (exists) {
        // 如果存在，尝试获取大小
        //qint64 size = resourceFile.size();
        //qDebug() << "Resource file size:" << size;
    }

    if (m_appTranslator->load(langFile)) {
        if (QCoreApplication::installTranslator(m_appTranslator)) {
            emit languageChanged();
            qDebug() << "Language switched to:" << m_currentLanguage;
        } else {
            qDebug() << "Failed to install translator:" << langFile;
        }
    } else {
        qDebug() << "Failed to load translator:" << langFile;
    }
}

void systemSettingCtrl::switchBrightness(int brightness)
{
    qDebug()<<"设置亮度："<<brightness;
    // 限制亮度值在 0 到 100 之间
    if (brightness < 0) {
        brightness = 0;
    }/* else if (brightness > 100) {
        brightness = 100;
    }*/

    m_brightness = brightness;

    // 构建命令字符串
    QString command = QString("echo lcd0 > name; echo setbl > command; echo %1 > param; echo 1 > start").arg(brightness);

    // 使用 QProcess 执行命令
    QProcess process;
    process.setWorkingDirectory("/sys/kernel/debug/dispdbg"); // 设置工作目录

    // 执行命令
    process.start("/bin/sh", QStringList() << "-c" << command);
    if (!process.waitForStarted()) {
        qDebug() << "Failed to start brightness setting process.";
        return;
    }

    if (!process.waitForFinished(3000)) { // 等待最多3秒
        qDebug() << "Brightness setting process timed out.";
        return;
    }

    // 检查执行结果
    if (process.exitCode() != 0) {
        qDebug() << "Failed to set brightness. Exit code:" << process.exitCode();
        qDebug() << "Error output:" << process.readAllStandardError();
    } else {
        qDebug() << "Brightness set to" << brightness << "% successfully.";
    }

    m_systemdata->UpdateSystemSetting_brightness(brightness);
}

int systemSettingCtrl::currentBrightness() const
{
    return m_brightness;
}

void systemSettingCtrl::getWifiNameAsync(int mode)
{

#ifdef Q_OS_WIN
    QTimer::singleShot(2000, [=]() {
        QStringList testList;
        testList << tr("测试");
        qDebug() << "刷新";
        m_wifiIntensity = "50";
        emit wifiListReady(mode, testList);
        emit wifiIntensityChanged();
    });
#else
    QtConcurrent::run([=](){
        QStringList newSsidList;
        QProcess process;
        process.start("wifi_list.sh");
        process.waitForFinished();
        QString output = process.readAllStandardOutput();
        QStringList res = output.split('\n', QString::SkipEmptyParts);

        if (res.size() > 1) {
            res.removeFirst();
            for (const auto& line : qAsConst(res)) {
                QStringList spl = line.split(" ", QString::SkipEmptyParts);
                if (spl[0] == "*") {
                    newSsidList.append(spl[1]);
                    qDebug() << "wifi_信号" << spl[1] << spl[6];
                    
                    // 更新已连接的 SSID 和信号强度
                    m_connectedSsid = spl[1];
                    m_wifiIntensity = spl[6];
                    
                    emit wifiConnectedChanged();
                    emit wifiIntensityChanged();
                } else {
                    newSsidList.append(spl[0]);
                }
            }
        } else {
            // 添加错误处理
            newSsidList << tr("未找到 WiFi 网络");
        }
        emit wifiListReady(mode, newSsidList);
    });
#endif
}

void systemSettingCtrl::connectWifi(QString ssid, QString password)
{
#ifdef Q_OS_WIN
    // Windows下模拟：2秒后返回假结果
    QTimer::singleShot(2000, [this, ssid, password]() {
        QString msg = QStringLiteral("WiFi连接成功（模拟）\nSSID: %1\nPassword: %2").arg(ssid, password);
        QStringList list;
        list << msg;
        emit wifiListReady(2, list);
    });
#else
    // 先清除所有同名配置（防止自动连接旧 WiFi）
    qDebug() << "连接 WIFI：";
    qDebug() << "SSID:" << ssid << "Password:" << password;

    // 删除旧的连接配置
    QProcess::execute("nmcli", QStringList() << "connection" << "delete" << "id" << ssid);
    QProcess::execute("nmcli", QStringList() << "connection" << "delete" << "id" << "wifi");

    // 使用 wifi_set.sh 脚本连接 WiFi
    QTimer::singleShot(0, this, [this, ssid, password]() {
        QProcess *process = new QProcess(this);
        
        // 连接完成信号
        connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                this, [this, process, ssid](int exitCode, QProcess::ExitStatus exitStatus) {

            if (exitCode == 0 && exitStatus == QProcess::NormalExit) {
                // 脚本执行成功，验证 WiFi 是否真的连接成功
                qDebug() << "WiFi 连接脚本执行成功，开始验证连接...";
                verifyWifiConnection(ssid);
            } else {
                // 连接失败
                QString stdErr = process->readAllStandardError();
                qDebug() << "WiFi 连接失败，退出码:" << exitCode << "错误信息:" << stdErr;
                QString msg = tr("WiFi 连接失败");
                QStringList list;
                list << msg;
                emit wifiListReady(2, list);
            }

            process->deleteLater();
        });
        
        // 使用绝对路径调用脚本
        process->start("/usr/bin/wifi_set.sh", QStringList() << ssid << password);
    });
#endif
}

void systemSettingCtrl::verifyWifiConnection(const QString &targetSsid)
{
    // 等待3秒让WiFi连接生效
    QTimer::singleShot(3000, this, [this, targetSsid]() {
        QProcess *verifyProcess = new QProcess(this);

        connect(verifyProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                this, [this, verifyProcess, targetSsid](int exitCode, QProcess::ExitStatus exitStatus) {

            bool connected = false;
            if (exitCode == 0 && exitStatus == QProcess::NormalExit) {
                QString output = verifyProcess->readAllStandardOutput().trimmed();
                // 检查输出中是否包含目标SSID
                connected = output.contains(targetSsid, Qt::CaseInsensitive);
            }

            QString msg;
            if (connected) {
                // msg = QStringLiteral("WiFi连接成功\nSSID: %1").arg(targetSsid);
                msg = tr("WiFi连接成功\nSSID: %1").arg(targetSsid);
                getWifiNameAsync(); // 连接成功才获取WiFi名称
            } else {
                // msg = QStringLiteral("WiFi连接失败：未连接到 %1").arg(targetSsid);
                msg = tr("WiFi连接失败：未连接到 %1").arg(targetSsid);

            }

            QStringList list;
            list << msg;
            emit wifiListReady(2, list);

            verifyProcess->deleteLater();
        });

        // 使用iwgetid命令检查当前连接的WiFi名称
        verifyProcess->start("iwgetid -r");
        // 或者使用: nmcli -t -f name,device connection show --active | grep wifi | cut -d: -f1
    });
}


void systemSettingCtrl::disconnectWifi()
{
#ifdef Q_OS_WIN
    QTimer::singleShot(2000, [this]() {
        QStringList list;
        list << tr("WiFi断开成功（模拟）");
        emit wifiListReady(2, list);
    });
#else
    QTimer::singleShot(0, [this]() {
        QString program = "/usr/bin/wifi_del.sh";
        QProcess *process = new QProcess(this);
        connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                this, [this, process](int exitCode, QProcess::ExitStatus exitStatus) {
            QString stdOut = process->readAllStandardOutput();
            QString stdErr = process->readAllStandardError();
            bool success = (exitCode == 0 && exitStatus == QProcess::NormalExit);
            QString msg = success ? tr("WiFi 断开成功") : tr("WiFi 断开失败");
            QStringList list;
            list << msg;
            emit wifiListReady(2, list);
            process->deleteLater();
            
            // 清空连接状态
            m_connectedSsid.clear();
            m_wifiIntensity = "0";
            emit wifiConnectedChanged();
            emit wifiIntensityChanged();
        });
        process->start(program);
    });
#endif
}

void systemSettingCtrl::onVerifyConnectionFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    if (exitCode != 0 || exitStatus != QProcess::NormalExit) {
        // 命令执行失败，视为断开
        handleWifiStatusUpdate("", "0");
        return;
    }

    QString currentSsid = m_verifyProcess->readAllStandardOutput().trimmed();

    if (currentSsid.isEmpty()) {
        // 未连接
        handleWifiStatusUpdate("", "0");
    } else {
        // 已连接，记录 SSID 并继续获取信号强度
        m_tempSsidForSignalCheck = currentSsid;

        // 步骤 2: 获取信号强度 (假设网卡为 wlan0，请根据实际设备修改)
        // 使用 grep 提取数字部分
        m_signalProcess->start("sh", QStringList() << "-c" << "iwconfig wlan0 | grep -oP 'Signal level=\\K-?\\d+'");
        // 后续逻辑在 onGetSignalFinished 中继续
    }
}

void systemSettingCtrl::onGetSignalFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    QString signal = "0";
    if (exitCode == 0 && exitStatus == QProcess::NormalExit) {
        QString out = m_signalProcess->readAllStandardOutput().trimmed();
        if (!out.isEmpty()) {
            signal = out;
        }
    }

    // 使用之前暂存的 SSID 进行最终状态更新
    handleWifiStatusUpdate(m_tempSsidForSignalCheck, signal);
}

void systemSettingCtrl::handleWifiStatusUpdate(const QString &ssid, const QString &signal)
{
    // 检查 SSID 变化
    if (m_connectedSsid != ssid) {
        m_connectedSsid = ssid;
        // 如果状态改变，通知外部（例如用于自动刷新列表中的星号）
        emit wifiConnectedChanged();
    }

    // 检查信号变化
    if (m_wifiIntensity != signal) {
        m_wifiIntensity = signal;
        emit wifiIntensityChanged();
    }

    m_isMonitoring = false;
}

void systemSettingCtrl::updateDateTime(QString datetime)
{
    // 解析输入的时间字符串
    QDateTime displayTime = QDateTime::fromString(datetime, "yyyy-MM-dd hh:mm:ss");
    if (!displayTime.isValid()) {
        qWarning() << "Invalid datetime format:" << datetime;
        return;
    }

    // 转换为系统时间字符串格式（注意：这里不减8小时，直接使用输入的时间）
    QString systemTimeStr = displayTime.toString("yyyy-MM-dd hh:mm:ss");

    // 1. 设置系统时间
    QProcess process;
    process.setProgram("date");
    process.setArguments({"-s", systemTimeStr});
    process.start();
    process.waitForFinished();

    if (process.exitCode() == 0) {
        qInfo() << "System time set successfully to:" << systemTimeStr;

        // 2. 将系统时间同步到硬件时钟
        process.setProgram("hwclock");
        process.setArguments({"--systohc"});
        process.start();
        process.waitForFinished();

        if (process.exitCode() == 0) {
            qInfo() << "Hardware clock updated successfully.";
        } else {
            qWarning() << "Failed to update hardware clock. Error:" <<
                          QString::fromLocal8Bit(process.readAllStandardError());
        }
    } else {
        qWarning() << "Failed to set system time. Error:" <<
                      QString::fromLocal8Bit(process.readAllStandardError());
    }
}

QString systemSettingCtrl::getDateTime()
{
    return QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
}

QString systemSettingCtrl::currentLanguage() const
{
    return m_currentLanguage;
}

bool systemSettingCtrl::isChinese() const
{
    return m_currentLanguage.compare("zh_CN") == 0;
}

QString systemSettingCtrl::getCurrentTimeInTimezone()
{
    QDateTime utcTime = QDateTime::currentDateTimeUtc();

    // 手动添加8小时偏移（东八区）
    QDateTime beijingTime = utcTime.addSecs(8 * 3600);

    // 格式化输出
    return beijingTime.toString("yyyy-MM-dd hh:mm:ss");
}

void systemSettingCtrl::syncSystemTime()
{
    QProcess process;
    //hwclock--hctosys
    process.setProgram("hwclock");
    process.setArguments({"--hctosys"});
    process.start();
}

/************************************
 * @brief 获取序列号
 * @param
 * @return 返回32位序列号
 *************************************/
QString systemSettingCtrl::getSerialNumber()
{
    QFile file("/sys/class/sunxi_info/sys_info");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Failed to open sys_info file";
        return QString(32, '0'); // 返回32个'0'的字符串
    }

    QTextStream in(&file);
    QString sn(32, '0'); // 初始化为32个'0'
    QRegularExpression re("sunxi_chipid.*([0-9a-fA-F]{32})");

    while (!in.atEnd()) {
        QString line = in.readLine();
        if (line.contains("sunxi_chipid")) {
            QRegularExpressionMatch match = re.match(line);
            if (match.hasMatch())
            {
                sn = match.captured(1);
                break;
            }
        }
    }
    file.close();


    qDebug()<<"序列号："<<sn;
    return sn;
}

void systemSettingCtrl::startMonitoring()
{
    m_monitorTimer->start();

    // 立即执行一次检查，避免等待第一个周期
    onMonitorTimeout();

#ifdef Q_OS_WIN
    m_winSimTimer->start(2000);
#endif

    qDebug() << "WiFi Monitor Started";
}

void systemSettingCtrl::stopMonitoring()
{
    m_monitorTimer->stop();

#ifdef Q_OS_WIN
    m_winSimTimer->stop();
#endif

    // 取消正在进行的进程以防止回调干扰
    m_verifyProcess->kill();
    m_signalProcess->kill();

    qDebug() << "WiFi Monitor Stopped";
}

void systemSettingCtrl::onMonitorTimeout()
{
    if(m_isMonitoring) {
        qDebug() << "监测进程未结束";
        return;
    }
#ifdef Q_OS_WIN
    return;
#else
    // 步骤 1: 快速获取当前连接的 SSID (非阻塞)
    m_verifyProcess->start("iwgetid", QStringList() << "-r");
    // 后续逻辑在 onVerifyConnectionFinished 中继续
#endif
}

void systemSettingCtrl::update_system()
{
    QVariantMap logData;
    logData["username"] = "";
    logData["user_id"] = -1;
    logData["operation"] = "执行了系统升级";
    SqlOrmManager::instance()->addOperationLog(logData);

    update_apk::getInstance()->get_file_list();
}

