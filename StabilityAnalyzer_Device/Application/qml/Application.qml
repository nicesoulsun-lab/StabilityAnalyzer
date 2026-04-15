import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12

ApplicationWindow {
    id: mainApplicationWindow

    width: 1024
    height: 600
    minimumWidth: 1024
    minimumHeight: 600
    maximumWidth: 1024
    maximumHeight: 600
    title: "稳定性分析仪"
    visible: true

    property var mainStackView: null

    property alias rootRectangle: root

    Rectangle{
        id:root
        width: parent.width; height: parent.height
        color: "transparent"

        // 嵌套子项目的界面
        Loader {
            id: main_loader
            anchors.fill: parent
            source: "qrc:/qml/MainWindow.qml"

            // 2. 关键：在加载完成后，将内部的对象赋值给外部变量
            onLoaded: {
                console.log("子项目界面加载完成")
                if (item) {
                    mainStackView = item.mainStackView;
                    console.log("MainStackView 已绑定:", mainStackView);
                }
            }

            onStatusChanged: {
                if (status === Loader.Error) {
                    console.log("加载子项目界面失败:", source)
                }
            }
        }
    }

    InfoPop{
        id: info_pop
    }

    NewProjectPop{
        id: new_project_pop
    }

    SelectChannelsPop{
        id:select_channel_pop
    }

    CustomPop{
        id: custom_pop
    }

//    Keyboard{
//        id: keyboard
//        window: mainApplicationWindow
//        root: root
//    }
}
