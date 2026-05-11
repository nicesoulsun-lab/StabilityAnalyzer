// File: ConfirmCancelButton.qml
import QtQuick 2.9
import QtQuick.Controls 2.2

Button {
    id: root

    property int model: 1 // 1: 鍙栨秷(宸?, 2: 纭(鍙?, 3: 鐗规畩鑹?
    property real pressedScale: 0.95
    property real scaleFactor: 1.0

    width: 95
    height: 38

    // 鎸夐挳鏂囨湰
    property string btnText: model === 1 ? qsTr("鍙栨秷") : (model === 2 ? qsTr("纭") : qsTr("纭"))

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
    //璁剧疆杈规 1px

    signal clicked()

    scale: mouseArea.pressed ? 0.92 : 1.0
    transformOrigin: Item.Center // 鍏抽敭锛氫互涓績涓鸿酱缂╂斁

    Behavior on scale {
        NumberAnimation {
            duration: 100
            easing.type: Easing.OutQuad
        }
    }
}

