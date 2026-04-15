import QtQuick 2.12
import QtQuick.Controls 2.12

Rectangle {
    id: root

    property string title: qsTr("A通道")
    property bool isRunning: true
    property bool hasSample: true
    property bool isCovered: true

    color: "transparent"
    radius: 20
    border.color: "#81BFFE"
    border.width: 1

    gradient: Gradient {
        GradientStop { position: 0.0; color: "#FDFEFF" }
        GradientStop { position: 1.0; color: "#DCEEFE" }
    }

    Column {
        anchors.top: parent.top
        anchors.topMargin: 8
        anchors.horizontalCenter: parent.horizontalCenter
        width: Math.max(0, parent.width - 8)
        spacing: 8

        Rectangle {
            anchors.horizontalCenter: parent.horizontalCenter
            width: Math.max(0, parent.width - 8)
            height: 44
            radius: 12
            color: "#D6EBFE"

            Text {
                anchors.centerIn: parent
                text: root.title
                font.pixelSize: 13
                color: "#1A1A1A"
                font.family: "Microsoft YaHei"
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.NoWrap
            }
        }

        Row {
            anchors.horizontalCenter: parent.horizontalCenter
            spacing: 6

            Rectangle {
                width: 12
                height: 12
                radius: 6
                color: root.isRunning ? "#32CD32" : "#8B8C8F"
                anchors.verticalCenter: parent.verticalCenter
            }

            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: root.isRunning ? qsTr("实验中") : qsTr("空闲")
                font.pixelSize: 11
                color: root.isRunning ? "#32CD32" : "#8B8C8F"
                font.family: "Microsoft YaHei"
            }
        }

        Rectangle {
            anchors.horizontalCenter: parent.horizontalCenter
            width: Math.min(parent.width, 70)
            height: 1
            color: "#9EC7F1"
            opacity: 0.9
        }

        Row {
            anchors.horizontalCenter: parent.horizontalCenter
            spacing: 6

            Rectangle {
                width: 12
                height: 12
                radius: 6
                color: "transparent"
                border.width: 1
                border.color: root.hasSample ? "#2F7DE1" : "#8B8C8F"
                anchors.verticalCenter: parent.verticalCenter

                Text {
                    anchors.centerIn: parent
                    text: root.hasSample ? "✓" : ""
                    font.pixelSize: 10
                    color: root.hasSample ? "#2F7DE1" : "#8B8C8F"
                }
            }

            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: root.hasSample ? qsTr("有样品") : qsTr("无样品")
                font.pixelSize: 11
                color: root.hasSample ? "#2F7DE1" : "#8B8C8F"
                font.family: "Microsoft YaHei"
            }
        }

        Rectangle {
            anchors.horizontalCenter: parent.horizontalCenter
            width: Math.min(parent.width, 70)
            height: 1
            color: "#9EC7F1"
            opacity: 0.9
        }

        Row {
            anchors.horizontalCenter: parent.horizontalCenter
            spacing: 6

            Rectangle {
                width: 12
                height: 12
                radius: 6
                color: "transparent"
                border.width: 1
                border.color: root.isCovered ? "#2F7DE1" : "#E53935"
                anchors.verticalCenter: parent.verticalCenter

                Text {
                    anchors.centerIn: parent
                    text: root.isCovered ? "✓" : "×"
                    font.pixelSize: 10
                    color: root.isCovered ? "#2F7DE1" : "#E53935"
                }
            }

            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: root.isCovered ? qsTr("已关盖") : qsTr("未关盖")
                font.pixelSize: 11
                color: root.isCovered ? "#2F7DE1" : "#E53935"
                font.family: "Microsoft YaHei"
            }
        }
    }
}
