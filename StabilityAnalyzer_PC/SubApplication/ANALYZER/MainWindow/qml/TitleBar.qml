import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import QtGraphicalEffects 1.12
import QtQuick.Window 2.12

Rectangle {
    id: titleBar

    property Window targetWindow
    property bool showNavigation: false
    readonly property string deviceUiState: data_transmit_ctrl ? data_transmit_ctrl.deviceUiConnectionStateText : "Disconnected"
    readonly property string deviceStatusText: {
        if (!data_transmit_ctrl) {
            return qsTr("设备未连接")
        }
        if (deviceUiState === "Connected") {
            return qsTr("设备已连接")
        }
        if (deviceUiState === "Connecting") {
            return qsTr("设备连接中")
        }
        return qsTr("设备未连接")
    }
    readonly property color deviceStatusColor: deviceUiState === "Connected"
                                               ? "#2FA36B"
                                               : (deviceUiState === "Connecting" ? "#E3A008" : "#8B8C8F")

    color: "#d6e8f7"

    Image {
        id: background
        anchors.fill: parent
        source: "qrc:/icon/qml/icon/bar-background.png"
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0
        z: 1

        Item {
            Layout.fillWidth: true
            Layout.preferredHeight: 30

            MouseArea {
                anchors.fill: parent
                acceptedButtons: Qt.LeftButton

                property point clickPos: "0,0"
                property bool isDragging: false
                property int dragThreshold: 4
                property bool ignoreDrag: false

                onPressed: {
                    clickPos = Qt.point(mouseX, mouseY)
                    ignoreDrag = false
                    isDragging = false
                }
                onDoubleClicked: {
                    ignoreDrag = true
                    if (targetWindow) {
                        if (targetWindow.visibility === Window.Maximized) targetWindow.showNormal()
                        else targetWindow.showMaximized()
                    }
                }
                onReleased: {
                    isDragging = false
                }
                onPositionChanged: {
                    if (!pressed || !targetWindow || ignoreDrag) return
                    if (!isDragging) {
                        if (Math.abs(mouseX - clickPos.x) > dragThreshold || Math.abs(mouseY - clickPos.y) > dragThreshold)
                            isDragging = true
                        else
                            return
                    }

                    if (targetWindow.visibility === Window.Maximized) {
                        var xRatio = mouseX / targetWindow.width
                        targetWindow.showNormal()
                        var newClickX = targetWindow.width * xRatio
                        targetWindow.x = (targetWindow.x + mouseX) - newClickX
                        clickPos = Qt.point(newClickX, mouseY)
                        return
                    }
                    targetWindow.x += mouseX - clickPos.x
                    targetWindow.y += mouseY - clickPos.y
                }
            }

            RowLayout {
                anchors.fill: parent
                spacing: 0

                Row {
                    Layout.fillWidth: true
                    Layout.leftMargin: 25
                    Layout.alignment: Qt.AlignVCenter
                    spacing: 8
                    height: parent.height

                    Image {
                        source: "qrc:/icon/qml/icon/huoerde-log.png"
                        width: 70
                        height: 20
                        anchors.verticalCenter: parent.verticalCenter
                    }

                    Text {
                        text: qsTr("  |  稳定性分析仪")
                        color: "#005BAC"
                        font.pixelSize: 13
                        font.family: "Microsoft YaHei"
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }

                Row {
                    spacing: 24
                    height: parent.height
                    layoutDirection: Qt.LeftToRight
                    Layout.alignment: Qt.AlignVCenter

                    Row {
                        spacing: 8
                        anchors.verticalCenter: parent.verticalCenter

                        Rectangle {
                            width: 10
                            height: 10
                            radius: 5
                            color: titleBar.deviceStatusColor
                            anchors.verticalCenter: parent.verticalCenter
                        }

                        Text {
                            text: titleBar.deviceStatusText
                            color: titleBar.deviceStatusColor
                            font.pixelSize: 12
                            font.family: "Microsoft YaHei"
                            anchors.verticalCenter: parent.verticalCenter
                        }
                    }

                    Text {
                        id: timeText
                        text: ""
                        color: "#005BAC"
                        font.pixelSize: 12
                        font.family: "Microsoft YaHei"
                        anchors.verticalCenter: parent.verticalCenter
                    }

                    Timer {
                        interval: 1000
                        running: true
                        repeat: true
                        triggeredOnStart: true
                        onTriggered: {
                            var date = new Date()
                            var weekDays = [qsTr("周日"), qsTr("周一"), qsTr("周二"), qsTr("周三"), qsTr("周四"), qsTr("周五"), qsTr("周六")]
                            timeText.text = Qt.formatDateTime(date, "yyyy/MM/dd hh:mm ") + weekDays[date.getDay()]
                        }
                    }

                    CaptionButton {
                        width: 36
                        height: 36
                        z: 2
                        text: "\u2014"
                        onClicked: if (targetWindow) targetWindow.showMinimized()
                    }

                    CaptionButton {
                        width: 36
                        height: 36
                        z: 2
                        text: targetWindow && targetWindow.visibility === Window.Maximized ? "\u2750" : "\u25A1"
                        onClicked: {
                            if (targetWindow) {
                                targetWindow.visibility === Window.Maximized ? targetWindow.showNormal() : targetWindow.showMaximized()
                            }
                        }
                    }

                    CaptionButton {
                        width: 36
                        height: 36
                        z: 2
                        text: "\u2715"
                        isCloseButton: true
                        onClicked: if (targetWindow) targetWindow.close()
                    }
                }
            }
        }
    }
}
