#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include "inc/modbustaskscheduler.h"


int main(int argc, char *argv[])
{
    qputenv("QT_IM_MODULE", QByteArray("qtvirtualkeyboard"));

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif

    QGuiApplication app(argc, argv);

    // 创建任务调度器实例
    ModbusTaskScheduler *scheduler = new ModbusTaskScheduler(&app);
    
    // 设置默认配置文件路径（可以根据需要修改）
    QString configFile = "../../../bin-mingw/config.json";
    scheduler->setConfigFile(configFile);

    QQmlApplicationEngine engine;
    
    // 将调度器注册到QML上下文
    engine.rootContext()->setContextProperty("taskScheduler", scheduler);
    
    const QUrl url(QStringLiteral("qrc:/main.qml"));
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                     &app, [url](QObject *obj, const QUrl &objUrl) {
        if (!obj && url == objUrl)
            QCoreApplication::exit(-1);
    }, Qt::QueuedConnection);
    engine.load(url);

    return app.exec();
}
