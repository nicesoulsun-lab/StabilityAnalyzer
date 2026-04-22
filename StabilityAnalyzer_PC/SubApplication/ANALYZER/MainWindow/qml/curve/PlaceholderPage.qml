import QtQuick 2.12

Rectangle {
    property var detailPage

    // 预留页统一走这个占位组件，避免主页面继续堆临时文本。
    color: "#FFFFFF"

    Rectangle {
        anchors.fill: parent
        anchors.margins: 18
        color: "#F8FBFF"
        border.color: "#D8E4F0"
        border.width: 1
        radius: 6

        Text {
            anchors.centerIn: parent
            text: qsTr("该页面暂未实现")
            font.pixelSize: 16
            font.family: "Microsoft YaHei"
            color: "#7A8CA5"
        }
    }
}
