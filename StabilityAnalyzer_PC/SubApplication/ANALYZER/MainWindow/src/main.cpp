#include "MainWindow.h"
#include "CurveItem.h"
#include "Controller/controllerManager.h"
#include <QtWidgets/QApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQmlEngine>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // 注册 CurveItem 类型到 QML
    qmlRegisterType<CurveItem>("CustomComponents", 1, 0, "CurveItem");

    // 创建 QML 引擎
    QQmlApplicationEngine engine;

    // 注册 MainWindow 实例
    MainWindow *mainWindowInstance = new MainWindow();
    CtrllerManager *controllerManager = new CtrllerManager(&engine);
    engine.rootContext()->setContextProperty("mainWindow", mainWindowInstance);
    engine.rootContext()->setContextProperty("dataModel", mainWindowInstance->curveModel());
    engine.rootContext()->setContextProperty("system_ctrl", controllerManager->getSystemSettingCtrl());
    engine.rootContext()->setContextProperty("user_ctrl", controllerManager->getUserCtrl());
    engine.rootContext()->setContextProperty("data_ctrl", controllerManager->getDataCtrl());
    engine.rootContext()->setContextProperty("experiment_ctrl", controllerManager->getExperimentCtrl());
    engine.rootContext()->setContextProperty("user_list_model", controllerManager->getUserListmodel());
    engine.rootContext()->setContextProperty("experiment_list_model", controllerManager->getExperimentListmodel());

    // 加载主 QML 文件
    engine.load(QUrl(QStringLiteral("qrc:/qml/MainWindow.qml")));

    if (engine.rootObjects().isEmpty()) {
        return -1;
    }

    return a.exec();
}
