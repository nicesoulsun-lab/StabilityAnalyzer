#include <QQmlApplicationEngine>
#include <QQmlEngine>
#include <QQmlContext>
#include <QSharedPointer>
#include <QQuickWindow>

#include "logmanager.h"
#include "qtsingleapplication.h"
//#include "Initwork.h" //初始化操作，暂时注释掉，后续如果场景需要再放开
//#include "JlCompress.h"
#include "MainWindow.h"
#include "CurveItem.h"
#include "Controller/controllerManager.h"
#include "Controller/experiment_ctrl.h"

#ifdef Q_OS_WIN
// win 端 生成 dump 前置在 main 中 保证应用启动即能检测
#include <windows.h>
#include "DbgHelp.h"
LONG CreateCrashHandler(EXCEPTION_POINTERS *pException){
    //创建 Dump 文件
    QDateTime CurDTime = QDateTime::currentDateTime();
    QString current_date = CurDTime.toString("yyyy_MM_dd_hh_mm_ss");
    //dmp 文件的命名
    QString dumpText = "Dump_"+current_date+".dmp";
    //    EXCEPTION_RECORD *record = pException->ExceptionRecord;
    //    QString errCode(QString::number(record->ExceptionCode, 16));
    //    QString errAddr(QString::number((uint)record->ExceptionAddress, 16));
    //    QString errFlag(QString::number(record->ExceptionFlags, 16));
    //    QString errPara(QString::number(record->NumberParameters, 16));
    HANDLE DumpHandle = CreateFile((LPCWSTR)dumpText.utf16(),
                                   GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if(DumpHandle != INVALID_HANDLE_VALUE) {
        MINIDUMP_EXCEPTION_INFORMATION dumpInfo;
        dumpInfo.ExceptionPointers = pException;
        dumpInfo.ThreadId = GetCurrentThreadId();
        dumpInfo.ClientPointers = TRUE;
        //将 dump 信息写入 dmp 文件
        MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(),DumpHandle, MiniDumpNormal, &dumpInfo, NULL, NULL);
        CloseHandle(DumpHandle);
    }
    return EXCEPTION_EXECUTE_HANDLER;
}
#endif

int main(int argc, char *argv[])
{
    /// 添加 organizationName 解决 qml fileDialog QSettings 告警
    QCoreApplication::setOrganizationName("DetectionSystem");
    QTextCodec *codec = QTextCodec::codecForName("UTF-8");//或者"GBK,UTF-8",不分大小写
    //QTextCodec::setCodecForLocale(codec);

    QCoreApplication::setAttribute(Qt::AA_DisableHighDpiScaling);
#ifdef Q_OS_MAC
    QDir appBin(QCoreApplication::applicationDirPath());
    appBin.cdUp();    /* Fix this on Mac because of the .app folder, */
    appBin.cdUp();    /* which means that the actual executable is   */
    appBin.cdUp();
    QDir::setCurrent(appBin.absolutePath());
#endif
#ifdef Q_OS_WIN
    //注冊异常捕获函数
    SetUnhandledExceptionFilter((LPTOP_LEVEL_EXCEPTION_FILTER)CreateCrashHandler);
#endif

    //初始化日志模块
    LogManager log;
    QDateTime CurDTime = QDateTime::currentDateTime();
    QString current_date = "idsp"+CurDTime.toString("yyyy_MM_dd");
    log.InitLog("./log",current_date,true);
    LOG_INFO("**************************应用程序启动信息**************************");
    //QGuiApplication app(argc, argv);
    QtSingleApplication app(argc,argv);
    if(app.sendMessage("isRunning"))
    {
        LOG_WARNING(QObject::tr("重复启动程序"));
        return 0;
    }
    LOG_INFO()<<"程序启动主线程:"<<QThread::currentThread();

    // 注册 CurveItem 类型到 QML
    //qmlRegisterType<CurveItem>("CustomComponents", 1, 0, "CurveItem");
    
    // 注册 ExperimentCtrl 类到 QML，这样可以访问 Channel 枚举
    qmlRegisterUncreatableType<ExperimentCtrl>("StabilityAnalyzer", 1, 0, "ExperimentCtrl", "Cannot create ExperimentCtrl in QML");

    QQmlApplicationEngine engine;

    // 注册 MainWindow 实例
//    MainWindow *mainWindowInstance = new MainWindow(&engine);
//    engine.rootContext()->setContextProperty("mainWindow", mainWindowInstance);
//    engine.rootContext()->setContextProperty("dataModel", mainWindowInstance->curveModel());

    CtrllerManager *ctrl_manager = new CtrllerManager(&app);
    engine.rootContext()->setContextProperty("system_ctrl", ctrl_manager->getSystemSettingCtrl());
    engine.rootContext()->setContextProperty("user_ctrl", ctrl_manager->getUserCtrl());
    engine.rootContext()->setContextProperty("data_ctrl", ctrl_manager->getDataCtrl());
    engine.rootContext()->setContextProperty("experiment_ctrl", ctrl_manager->getExperimentCtrl());

    engine.rootContext()->setContextProperty("user_list_model", ctrl_manager->getUserListmodel());
    engine.rootContext()->setContextProperty("experiment_list_model", ctrl_manager->getExperimentListmodel());

    // 直接加载 QML 界面
    engine.load(QUrl(QStringLiteral("qrc:/qml/Application.qml")));

    if (engine.rootObjects().isEmpty()) {
        LOG_ERROR("QML 界面加载失败");
        return -1;
    }

    LOG_INFO("QML 界面加载成功");

    return app.exec();
}
