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

    qmlRegisterType<CurveItem>("CustomComponents", 1, 0, "CurveItem");

    CtrllerManager *ctrl_manager = new CtrllerManager(&a);
    ctrl_manager->getDataTransmitCtrl()->startConnection();
    QObject::connect(ctrl_manager->getExperimentCtrl(), &ExperimentCtrl::experimentStopped,
                     ctrl_manager->getDataCtrl(), &dataCtrl::clearExperimentRuntimeResources);

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("data_transmit_ctrl", ctrl_manager->getDataTransmitCtrl());
    engine.rootContext()->setContextProperty("system_ctrl", ctrl_manager->getSystemSettingCtrl());
    engine.rootContext()->setContextProperty("user_ctrl", ctrl_manager->getUserCtrl());
    engine.rootContext()->setContextProperty("data_ctrl", ctrl_manager->getDataCtrl());
    engine.rootContext()->setContextProperty("experiment_ctrl", ctrl_manager->getExperimentCtrl());
    engine.rootContext()->setContextProperty("user_list_model", ctrl_manager->getUserListmodel());
    engine.rootContext()->setContextProperty("experiment_list_model", ctrl_manager->getExperimentListmodel());
    engine.rootContext()->setContextProperty("recycle_experiment_list_model", ctrl_manager->getRecycleExperimentListmodel());
    engine.load(QUrl(QStringLiteral("qrc:/qml/MainWindow.qml")));

    if (engine.rootObjects().isEmpty()) {
        return -1;
    }

    return a.exec();
}
