import QtQuick 2.12

Rectangle {
    property alias title: titleLabel.text
    property int blockWidth: 315
    property int blockHeight: 200
    property alias content: container.children // 允许外部插入内容

    width: blockWidth
    height: blockHeight
    color: "white"
    radius: 6

    UiText {
        id: titleLabel
        anchors.top: parent.top
        anchors.topMargin: 20
        anchors.horizontalCenter: parent.horizontalCenter
        pixelSize: 18
    }

    Item {
        id: container
        anchors.fill: parent
        // 这里留给外部放具体的按钮或滑动条
    }
}
