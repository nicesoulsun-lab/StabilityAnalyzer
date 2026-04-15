#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include "datatransmitcontroller.h"

int main(int argc, char *argv[])
{
    qputenv("QT_IM_MODULE", QByteArray("qtvirtualkeyboard"));

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif

    QGuiApplication app(argc, argv);

    // 创建数据传递控制器
    DataTransmitController *dataController = new DataTransmitController();

    QQmlApplicationEngine engine;
    
    // 将数据控制器暴露给QML
//    engine.rootContext()->setContextProperty("dataController", dataController);
    
    const QUrl url(QStringLiteral("qrc:/main.qml"));
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                     &app, [url](QObject *obj, const QUrl &objUrl) {
        if (!obj && url == objUrl)
            QCoreApplication::exit(-1);
    }, Qt::QueuedConnection);
    engine.load(url);

    // 应用程序退出时清理资源
    QObject::connect(&app, &QGuiApplication::aboutToQuit, [dataController]() {
        dataController->deleteLater();
    });

    return app.exec();
}
