import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import QtQuick.Window 2.12

ApplicationWindow {
    id: window

    width: 1280
    height: 768
    minimumWidth: 1280
    minimumHeight: 768
    title: "稳定性分析仪"
    visible: true
    flags: Qt.FramelessWindowHint | Qt.Window

    property var mainStackView: null
    property bool showNavigation: false

    property alias rootRectangle: root

    Loader {
        id: framelessBorderLoader
        anchors.fill: parent
        source: "qrc:/qml/FramelessBorder.qml"
        onLoaded: item.targetWindow = window
    }

    Rectangle {
        id: root
        width: parent.width
        height: parent.height
        color: "#f0f4fa"

        Column {
            anchors.fill: parent
            spacing: 0

            Loader {
                id: titleBarLoader
                width: parent.width
                height: 36
                source: "qrc:/qml/TitleBar.qml"
                onLoaded: {
                    item.targetWindow = window
                    item.showNavigation = Qt.binding(function() { return window.showNavigation })
                }

                Behavior on height { NumberAnimation { duration: 200; easing.type: Easing.OutCubic } }
            }

            Item {
                id: contentArea
                width: parent.width
                height: parent.height - titleBarLoader.height

                Loader {
                    id: main_loader
                    anchors.fill: parent
                    source: "qrc:/qml/MainWindow.qml"

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
        }
    }

    // 监听 StackView 当前页面变化
    Connections {
        target: mainStackView
        enabled: mainStackView !== null
        onCurrentItemChanged: {
            if (mainStackView && mainStackView.currentItem) {
                showNavigation = (mainStackView.currentItem.objectName === "HomePage")
            } else {
                showNavigation = false
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
}
