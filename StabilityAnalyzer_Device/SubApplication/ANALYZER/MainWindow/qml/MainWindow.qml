import QtQuick 2.12
import QtQuick.Window 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import "component"

Item  {
    id: window
    width: 1024
    height: 600

    objectName: "MainWindow"

    property alias mainStackView: stackView
    property alias rootRectangle: root
    property bool main_loading:false
    property bool home_show: stackView.depth > 2
    property int user_level: -1  // 1表示普通用户，2表示高级用户 3表示管理员
    property string user_name: ""

    Timer{
        interval: 2000
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

        Item{
            width: parent.width
            height: parent.height
            visible: !main_loading

            // 顶部Logo (保持原位置不变，或者你可以根据需要调整)
            Item {

                anchors{
                    top: parent.top
                    topMargin: 63
                    horizontalCenter: parent.horizontalCenter
                }

                width: 162
                height: 30
                Image{
                    anchors.fill: parent
                    source: "qrc:/icon/qml/icon/logo.png"
                    fillMode: Image.PreserveAspectFit
                }
            }

            UiText{
                id: titleText
                text: qsTr("稳定性分析仪")
                anchors.centerIn: parent
                //向上偏移50像素
                pixelSize: 55
                anchors.verticalCenterOffset: -50
            }
        }

        Statusbar{
            id: statusbar
            visible: main_loading
        }

        StackView{
            id:stackView
            visible: main_loading
            anchors{
                left: parent.left
                leftMargin: 27
                top: parent.top
                topMargin: 69
            }
            width: 972
            height: 501
        }
    }

    Component.onCompleted: {

        stackView.push(Qt.resolvedUrl("qrc:/qml/LoginPage.qml"))

    }

    Connections {
        target: data_ctrl
        onOperationInfo: {
            info_pop.openDialog(message)
        }
    }
}
