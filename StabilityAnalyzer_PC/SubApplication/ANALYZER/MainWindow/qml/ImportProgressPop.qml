import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12

import "component"

Popup {
    id: root
    width: 420
    height: 140
    modal: true
    dim: true
    focus: true
    padding: 0
    anchors.centerIn: Overlay.overlay
    closePolicy: Popup.NoAutoClose

    background: Rectangle {
        color: "#FFFFFF"
        radius: 8
        border.width: 1
        border.color: "#D6E4F0"
    }

    contentItem: ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 18

        Text {
            Layout.fillWidth: true
            text: qsTr("导入中，请稍候")
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            font.pixelSize: 18
            color: "#2F3A4A"
            font.family: "Microsoft YaHei"
        }

        CustomProgressBar {
            Layout.fillWidth: true
            Layout.preferredHeight: 28
            indeterminate: true
            colorBackground: "#E7EEF7"
            colorFill: "#3B87E4"
            barHeight: 8
        }
    }
}
