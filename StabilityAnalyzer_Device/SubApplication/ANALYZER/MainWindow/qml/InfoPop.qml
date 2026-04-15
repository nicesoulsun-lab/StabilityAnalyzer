import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
Popup {
    id: root
    z: 9999999
    width: 500
    height: 150
    modal: true
    anchors.centerIn: Overlay.overlay
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    property string message: "提示信息"

    property int pixelSize : 16
    property string text_color : "#000000"
    //property string family : notoSansSCRegular.name

    function openDialog(msg) {
        root.message = msg;
        root.open();
    }

    onOpened: {
        //keyboard.visible = false;
    }
    background: Rectangle {
        color: "#FFFFFF"
        radius: 8
        border.width: 0
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 10

        Text {
            Layout.fillWidth: true
            Layout.fillHeight: true
            text: root.message
            font.pixelSize: pixelSize
            color: text_color
            //font.family: family
            wrapMode: Text.Wrap
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }
    }
}
