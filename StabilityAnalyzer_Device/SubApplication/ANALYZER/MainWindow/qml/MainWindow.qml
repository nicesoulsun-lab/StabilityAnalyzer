import QtQuick 2.9
import QtQuick.Window 2.2
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3
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
    property int user_level: -1  // 1琛ㄧず鏅€氱敤鎴凤紝2琛ㄧず楂樼骇鐢ㄦ埛 3琛ㄧず绠＄悊鍛?    property string user_name: ""

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

            // 椤堕儴Logo (淇濇寔鍘熶綅缃笉鍙橈紝鎴栬€呬綘鍙互鏍规嵁闇€瑕佽皟鏁?
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
                text: qsTr("绋冲畾鎬у垎鏋愪华")
                anchors.centerIn: parent
                //鍚戜笂鍋忕Щ50鍍忕礌
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

