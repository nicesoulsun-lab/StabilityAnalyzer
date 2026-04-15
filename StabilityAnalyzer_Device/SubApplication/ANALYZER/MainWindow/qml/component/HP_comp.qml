import QtQuick 2.12
import QtQuick.Layouts 1.12
import QtQuick.Controls 2.12

Item {
    id: root
    width: 212
    height: 330

    property string title: "A通道"
    property bool isRunning: true
    property bool hasSample: true
    property bool isCovered: true
    property string temperature: "25.0"
    property string remainingHours: "20.0"

    Rectangle {
        anchors.fill: parent
        color: "transparent"

        Image {anchors.fill: parent; source: "qrc:/icon/qml/icon/bg_hp_channel.png"}

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

            // 状态行：实验中
            ColumnLayout {
                // 1. 关键：让整个 ColumnLayout 在父容器中水平居中
                Layout.alignment: Qt.AlignHCenter
                spacing: 12 // 设置行间距

                // 运行状态图标
                Image {
                    source: isRunning ? "qrc:/icon/qml/icon/running.png" : "qrc:/icon/qml/icon/idle.png"

                    // 2. 关键：防止图片被拉伸，保持原始大小以便居中
                    Layout.preferredWidth: width
                    Layout.preferredHeight: height

                    Layout.alignment: Qt.AlignHCenter
                    Layout.leftMargin: 6
                }

                // 样品状态图标
                Image {
                    source: hasSample ? "qrc:/icon/qml/icon/sample.png" : "qrc:/icon/qml/icon/no_sample.png"

                    Layout.preferredWidth: width
                    Layout.preferredHeight: height

                    Layout.alignment: Qt.AlignHCenter
                }

                // 盖子状态图标
                Image {
                    source: isCovered ? "qrc:/icon/qml/icon/close.png" : "qrc:/icon/qml/icon/open.png"

                    Layout.preferredWidth: width
                    Layout.preferredHeight: height

                    Layout.alignment: Qt.AlignHCenter
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
                    text: "温度：" + root.temperature + " °C"
                    font.pixelSize: 20
                    font.bold: true

                    // 2. 关键：让文字在 Label 内部居中
                    horizontalAlignment: Text.AlignHCenter

                    // 3. 关键：告诉 Layout 这个 Label 只需要“包裹”文字的宽度，不要拉伸填满
                    Layout.preferredWidth: implicitWidth
                }

                // 剩余时间
                Label {
                    text: "剩余：" + root.remainingHours + "  h"
                    font.pixelSize: 20
                    font.bold: true

                    // 2. 关键：让文字在 Label 内部居中
                    horizontalAlignment: Text.AlignHCenter

                    // 3. 关键：同上，限制宽度为文字实际宽度
                    Layout.preferredWidth: implicitWidth
                }
            }
        }
    }
}

