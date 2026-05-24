#include <QQmlApplicationEngine>
#include <QQmlEngine>
#include <QQmlContext>
#include <QDateTime>
#include <QDir>
#include <QSharedPointer>
#include <QTextCodec>
#include <QThread>
#include <QQuickWindow>

#include "logmanager.h"
#include "qtsingleapplication.h"
#include "MainWindow.h"
#include "CurveItem.h"
#include "Controller/controllerManager.h"
#include "Controller/experiment_ctrl.h"

#ifdef Q_OS_WIN
#include <windows.h>
#include "DbgHelp.h"
LONG CreateCrashHandler(EXCEPTION_POINTERS *pException){
    QDateTime CurDTime = QDateTime::currentDateTime();
    QString current_date = CurDTime.toString("yyyy_MM_dd_hh_mm_ss");
    QString dumpText = "Dump_"+current_date+".dmp";
    HANDLE DumpHandle = CreateFile((LPCWSTR)dumpText.utf16(),
                                   GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if(DumpHandle != INVALID_HANDLE_VALUE) {
        MINIDUMP_EXCEPTION_INFORMATION dumpInfo;
        dumpInfo.ExceptionPointers = pException;
        dumpInfo.ThreadId = GetCurrentThreadId();
        dumpInfo.ClientPointers = TRUE;
        MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(),DumpHandle, MiniDumpNormal, &dumpInfo, NULL, NULL);
        CloseHandle(DumpHandle);
    }
    return EXCEPTION_EXECUTE_HANDLER;
}
#endif

int main(int argc, char *argv[])
{
    QCoreApplication::setOrganizationName("DetectionSystem");
    QTextCodec *codec = QTextCodec::codecForName("UTF-8");
    Q_UNUSED(codec)

    QCoreApplication::setAttribute(Qt::AA_DisableHighDpiScaling);
#ifdef Q_OS_MAC
    QDir appBin(QCoreApplication::applicationDirPath());
    appBin.cdUp();
    appBin.cdUp();
    appBin.cdUp();
    QDir::setCurrent(appBin.absolutePath());
#endif
#ifdef Q_OS_WIN
    SetUnhandledExceptionFilter((LPTOP_LEVEL_EXCEPTION_FILTER)CreateCrashHandler);
#endif

    LogManager log;
    QDateTime CurDTime = QDateTime::currentDateTime();
    QString current_date = "idsp"+CurDTime.toString("yyyy_MM_dd");
    log.InitLog("./log",current_date,true);
    LOG_INFO("**************************应用程序启动信息**************************");

    QtSingleApplication app(argc,argv);
    if(app.sendMessage("isRunning"))
    {
        LOG_WARNING(QObject::tr("重复启动程序"));
        return 0;
    }
    LOG_INFO()<<"程序启动主线程"<<QThread::currentThread();

    qmlRegisterUncreatableType<ExperimentCtrl>("StabilityAnalyzer", 1, 0, "ExperimentCtrl", "Cannot create ExperimentCtrl in QML");
    qmlRegisterType<CurveItem>("CustomComponents", 1, 0, "CurveItem");

    QQmlApplicationEngine engine;

    CtrllerManager *ctrl_manager = new CtrllerManager(&app);

    engine.rootContext()->setContextProperty("data_transmit_ctrl", ctrl_manager->getDataTransmitCtrl());
    engine.rootContext()->setContextProperty("system_ctrl", ctrl_manager->getSystemSettingCtrl());
    engine.rootContext()->setContextProperty("user_ctrl", ctrl_manager->getUserCtrl());
    engine.rootContext()->setContextProperty("data_ctrl", ctrl_manager->getDataCtrl());
    engine.rootContext()->setContextProperty("experiment_ctrl", ctrl_manager->getExperimentCtrl());
    engine.rootContext()->setContextProperty("realtime_curve_ctrl", ctrl_manager->getRealtimeCurveCtrl());
    engine.rootContext()->setContextProperty("user_list_model", ctrl_manager->getUserListmodel());
    engine.rootContext()->setContextProperty("experiment_list_model", ctrl_manager->getExperimentListmodel());

    engine.load(QUrl(QStringLiteral("qrc:/qml/Application.qml")));

    if (engine.rootObjects().isEmpty()) {
        LOG_ERROR("QML 界面加载失败");
        return -1;
    }

    LOG_INFO("QML 界面加载成功");

    QObject *rootObject = engine.rootObjects().first();
    if (rootObject) {
        QQuickWindow *window = qobject_cast<QQuickWindow*>(rootObject);
        if (window) {
            window->setWidth(1024);
            window->setHeight(600);
        }
    }

    return app.exec();
}
