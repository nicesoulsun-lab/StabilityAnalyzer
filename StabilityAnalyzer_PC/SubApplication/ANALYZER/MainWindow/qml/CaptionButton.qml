import QtQuick 2.12
import QtQuick.Controls 1.4
import QtQuick.Layouts 1.12

Rectangle {
    id: controlBtn
    property alias text: btnText.text
    property bool isCloseButton: false
    signal clicked()

    Layout.preferredWidth: 45
    Layout.fillHeight: true
    color: "transparent"

    Rectangle {
        anchors.fill: parent
        color: isCloseButton ? "#e81123" : "#3a7eba"
        opacity: mouseArea.containsMouse ? (isCloseButton ? 1.0 : 0.5) : 0.0
        Behavior on opacity { NumberAnimation { duration: 150 } }
    }

    Text {
        id: btnText
        anchors.centerIn: parent
        color: "#005BAC"
        font.pixelSize: 14
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: true
        onClicked: controlBtn.clicked()
        preventStealing: true
        propagateComposedEvents: false
    }
}
