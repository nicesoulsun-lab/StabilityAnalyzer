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
    // 加载主 QML 文件
    engine.load(QUrl(QStringLiteral("qrc:/qml/MainWindow.qml")));

    if (engine.rootObjects().isEmpty()) {
        return -1;
    }

    return a.exec();
}
