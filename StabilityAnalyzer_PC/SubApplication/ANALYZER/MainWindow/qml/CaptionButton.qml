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
    color: "transparent" // 默认透明背景

    // 鼠标悬停背景变色
    Rectangle {
        anchors.fill: parent
        // 关闭按钮悬停变红，其他变半透明深色
        color: isCloseButton ? "#e81123" : "#3a7eba"
        opacity: mouseArea.containsMouse ? (isCloseButton ? 1.0 : 0.5) : 0.0
        Behavior on opacity { NumberAnimation { duration: 150 } }
    }

    Text {
        id: btnText
        anchors.centerIn: parent
        color: "#005BAC"
        font.pixelSize: 14
        font.family: "Segoe UI Symbol" // 使用支持特殊符号的字体
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: true
        onClicked: controlBtn.clicked()
        // 防止鼠标事件穿透到下方的拖拽 MouseArea
        preventStealing: true
        propagateComposedEvents: false
    }
}
