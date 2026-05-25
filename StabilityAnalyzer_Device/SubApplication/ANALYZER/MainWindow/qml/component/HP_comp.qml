import QtQuick 2.12
import QtQuick.Layouts 1.12
import QtQuick.Controls 2.12

Item {
    id: root
    width: 212
    height: 330

    property string title: qsTr("A通道")
    property bool isRunning: true
    property bool hasSample: true
    property bool isCovered: true
    property string temperature: "25.0"
    property string remainingHours: "20.0"

    signal clicked()

    Rectangle {
        anchors.fill: parent
        color: "transparent"

        Image { anchors.fill: parent; source: "qrc:/icon/qml/icon/bg_hp_channel.png" }

        ColumnLayout {
            anchors.fill: parent
            spacing: 8

            // 标题
            Rectangle {
                Layout.alignment: Qt.AlignHCenter
                Layout.preferredWidth: 186
                Layout.preferredHeight: 53
                Layout.topMargin: 8
                radius: 15
                color: "#E9F4FF"

                Label {
                    text: root.title
                    font.pixelSize: 22
                    font.bold: true
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter // 顺便垂直居中
                    anchors.fill: parent
                }
            }

            // 状态行
            ColumnLayout {
                Layout.alignment: Qt.AlignHCenter
                spacing: 12

                // 运行状态
                Row {
                    Layout.alignment: Qt.AlignHCenter
                    spacing: 8

                    Rectangle {
                        width: 16
                        height: 16
                        radius: 8
                        color: root.isRunning ? "#32CD32" : "#8B8C8F"
                        anchors.verticalCenter: parent.verticalCenter
                    }

                    Label {
                        text: root.isRunning ? qsTr("实验中") : qsTr("空闲")
                        font.pixelSize: 18
                        font.bold: true
                        color: root.isRunning ? "#32CD32" : "#8B8C8F"
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }

                // 样品状态
                Row {
                    Layout.alignment: Qt.AlignHCenter
                    spacing: 8

                    Rectangle {
                        width: 16
                        height: 16
                        radius: 8
                        color: "transparent"
                        border.width: 1
                        border.color: root.hasSample ? "#2F7DE1" : "#8B8C8F"
                        anchors.verticalCenter: parent.verticalCenter

                        Label {
                            anchors.centerIn: parent
                            text: root.hasSample ? "✓" : "-"
                            font.pixelSize: 12
                            color: root.hasSample ? "#2F7DE1" : "#8B8C8F"
                        }
                    }

                    Label {
                        text: root.hasSample ? qsTr("有样品") : qsTr("无样品")
                        font.pixelSize: 18
                        font.bold: true
                        color: root.hasSample ? "#2F7DE1" : "#8B8C8F"
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }

                // 盖子状态
                Row {
                    Layout.alignment: Qt.AlignHCenter
                    spacing: 8

                    Rectangle {
                        width: 16
                        height: 16
                        radius: 8
                        color: "transparent"
                        border.width: 1
                        border.color: root.isCovered ? "#2F7DE1" : "#E53935"
                        anchors.verticalCenter: parent.verticalCenter

                        Label {
                            anchors.centerIn: parent
                            text: root.isCovered ? "✓" : "×"
                            font.pixelSize: 12
                            color: root.isCovered ? "#2F7DE1" : "#E53935"
                        }
                    }

                    Label {
                        text: root.isCovered ? qsTr("已关盖") : qsTr("未关盖")
                        font.pixelSize: 18
                        font.bold: true
                        color: root.isCovered ? "#2F7DE1" : "#E53935"
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }
            }

            // 分隔线
            Rectangle {
                Layout.preferredHeight: 1
                Layout.preferredWidth: 168
                Layout.alignment: Qt.AlignHCenter
                color: "#C0E0FE"
            }

            // 温度 & 剩余时间
            ColumnLayout {
                spacing: 8

                // 1. 关键：让整个 ColumnLayout 在父容器中水平居中
                Layout.alignment: Qt.AlignHCenter

                Layout.bottomMargin: 20

                // 温度
                Label {
                    text: qsTr("温度：") + root.temperature + " °C"
                    font.pixelSize: 20
                    font.bold: true

                    // 2. 关键：让文字在 Label 内部居中
                    horizontalAlignment: Text.AlignHCenter

                    // 3. 关键：告诉 Layout 这个 Label 只需要“包裹”文字的宽度，不要拉伸填满
                    Layout.preferredWidth: implicitWidth
                }

                // 剩余时间
                Label {
                    text: qsTr("剩余：") + root.remainingHours + "  h"
                    font.pixelSize: 20
                    font.bold: true

                    // 2. 关键：让文字在 Label 内部居中
                    horizontalAlignment: Text.AlignHCenter

                    // 3. 关键：同上，限制宽度为文字实际宽度
                    Layout.preferredWidth: implicitWidth
                }
            }
        }

        MouseArea {
            anchors.fill: parent
            onClicked: root.clicked()
        }
    }
}

