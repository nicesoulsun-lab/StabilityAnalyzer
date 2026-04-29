#include <QQmlApplicationEngine>
#include <QQmlEngine>
#include <QQmlContext>
#include <QSharedPointer>
#include <QQuickWindow>
#include <QDateTime>
#include <QTextCodec>
#include <QThread>

#include "logmanager.h"
#include "qtsingleapplication.h"
#include "MainWindow.h"
#include "CurveItem.h"
#include "Controller/controllerManager.h"
#include "Controller/experiment_ctrl.h"
#include "datatransmitcontroller.h"

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
    LOG_INFO("**************************Application Startup**************************");

    QtSingleApplication app(argc,argv);
    if(app.sendMessage("isRunning"))
    {
        LOG_WARNING(QObject::tr("Repeated application startup"));
        return 0;
    }
    LOG_INFO() << "Application main thread" << QThread::currentThread();

    qmlRegisterType<CurveItem>("CustomComponents", 1, 0, "CurveItem");
    qmlRegisterUncreatableType<ExperimentCtrl>("StabilityAnalyzer", 1, 0, "ExperimentCtrl", "Cannot create ExperimentCtrl in QML");

    QQmlApplicationEngine engine;
    CtrllerManager *ctrl_manager = new CtrllerManager(&app);

    QObject::connect(ctrl_manager->getDataTransmitCtrl(), &DataTransmitController::logMessage,
                     [](const QString &message) {
        LOG_INFO() << "[DataTransmit]" << message;
    });
    QObject::connect(ctrl_manager->getDataTransmitCtrl(), &DataTransmitController::lastErrorChanged,
                     [ctrl_manager]() {
        const QString errorText = ctrl_manager->getDataTransmitCtrl()->lastError();
        if (!errorText.isEmpty()) {
            LOG_ERROR() << "[DataTransmit]" << errorText;
        }
    });

    ctrl_manager->getDataTransmitCtrl()->startConnection();

    ctrl_manager->bindToQmlContext(engine.rootContext());

    engine.load(QUrl(QStringLiteral("qrc:/qml/Application.qml")));

    if (engine.rootObjects().isEmpty()) {
        LOG_ERROR("QML load failed");
        return -1;
    }

    LOG_INFO("QML load success");

    return app.exec();
}
