// File: ConfirmCancelButton.qml
import QtQuick 2.12
import QtQuick.Controls 2.12

Button {
    id: root

    property int model: 1 // 1: 取消(左), 2: 确认(右), 3: 特殊色
    property real pressedScale: 0.95
    property real scaleFactor: 1.0

    width: 95
    height: 38

    // 按钮文本
    property string btnText: model === 1 ? qsTr("取消") : (model === 2 ? qsTr("确认") : qsTr("确认"))

    contentItem: Text {
        text: root.btnText
        color: root.model === 3 ? "#0062BE" : "white"
        font.pixelSize: 18
        font.bold: true
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        anchors.centerIn: parent
    }
    background: Rectangle {
        radius: 8
        color: root.model === 1 ? "#52D0D6" : (root.model === 2 ? "#0369CF" : "#ddefff")
        scale: root.scaleFactor
        Behavior on scale { NumberAnimation { duration: 80; easing.type: Easing.InOutQuad } }
    }
    MouseArea {
        id: mouseArea
        anchors.fill: parent
        acceptedButtons: Qt.LeftButton
        onPressed: root.scaleFactor = root.pressedScale
        onReleased: root.scaleFactor = 1.0
        onCanceled: root.scaleFactor = 1.0
        onClicked: root.clicked()
    }
    //设置边框 1px

    signal clicked()

    scale: mouseArea.pressed ? 0.92 : 1.0
    transformOrigin: Item.Center // 关键：以中心为轴缩放

    Behavior on scale {
        NumberAnimation {
            duration: 100
            easing.type: Easing.OutQuad
        }
    }
}
