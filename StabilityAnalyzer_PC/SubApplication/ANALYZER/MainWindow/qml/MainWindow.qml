import QtQuick 2.12
import QtQuick.Window 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import "component"

Item {
    id: window

    objectName: "MainWindow"

    property alias mainStackView: stackView
    property alias rootRectangle: root
    property bool main_loading: false
    property int user_level: -1
    property string user_name: ""

    Timer {
        interval: 1500
        running: true
        repeat: false
        onTriggered: {
            main_loading = true
        }
    }

    Rectangle {
        id: root
        width: window.width
        height: window.height

        Image {
            id: backgroundImage
            anchors.fill: parent
            source: "qrc:/icon/qml/icon/background.png"
            fillMode: Image.PreserveAspectCrop
        }

        // 启动画面 - 在加载完成前显示
        Item {
            width: parent.width
            height: parent.height
            visible: !main_loading

            Item {
                anchors {
                    top: parent.top
                    topMargin: parent.height * 0.15
                    horizontalCenter: parent.horizontalCenter
                }
                width: 160
                height: 36
                Image {
                    anchors.fill: parent
                    source: "qrc:/icon/qml/icon/logo.png"
                    fillMode: Image.PreserveAspectFit
                }
            }

            UiText {
                id: titleText
                text: qsTr("稳定性分析仪")
                anchors.centerIn: parent
                anchors.verticalCenterOffset: -40
                pixelSize: 48
            }
        }

        StackView {
            id: stackView
            visible: main_loading
            anchors.fill: parent

            function backToLoginPage() {
                clear()
                push(Qt.resolvedUrl("qrc:/qml/LoginPage.qml"))
            }
        }
    }

    Component.onCompleted: {
        stackView.backToLoginPage()
    }

    Connections {
        target: data_ctrl
        onOperationInfo: {
            info_pop.openDialog(message)
        }
    }
}
