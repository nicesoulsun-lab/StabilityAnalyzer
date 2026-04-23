import QtQuick 2.12

Rectangle {
    id: root

    property string title: ""
    property string background_color: "#F3F5F7"
    property string font_color: "#333333"

    color: "#FFFFFF"
    border.color: "#E7ECF2"
    border.width: 1

    Rectangle {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        height: 30
        color: background_color

        Text {
            anchors.centerIn: parent
            text: root.title
            font.pixelSize: 16
            color: font_color
            //font.bold: true
            font.family: "Microsoft YaHei"
        }
    }
}
