import QtQuick 2.9

Rectangle {
    property alias title: titleLabel.text
    property int blockWidth: 315
    property int blockHeight: 200
    property alias content: container.children // 鍏佽澶栭儴鎻掑叆鍐呭

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
        // 杩欓噷鐣欑粰澶栭儴鏀惧叿浣撶殑鎸夐挳鎴栨粦鍔ㄦ潯
    }
}

