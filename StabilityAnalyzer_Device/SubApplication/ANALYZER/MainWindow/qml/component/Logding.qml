import QtQuick 2.9
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3


Popup {
    id: root
    z: 99999
    width: 500
    height: 150
    modal: true
    anchors.centerIn: Overlay.overlay  // 娉ㄦ剰锛氬簲璇ユ槸 Overlay.overlay锛屼笉鏄?OverlayLayout
    closePolicy: Popup.NoAutoClose
    property string message: "鎻愮ず淇℃伅"
    property bool loading: false

    property int pixelSize: 16
    property color text_color: "#242426"
    //property string family: notoSansSCRegular.name


    background: Rectangle {
        color: "#FFFFFF"
        radius: 8
        border.width: 1
        border.color: "#3498db"
    }

    ColumnLayout {
        // width: 500
        // height: 150
        anchors.fill: parent
        spacing: 20

        Text {
            Layout.fillWidth: true
            text: root.message
            wrapMode: Text.Wrap
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            font.pixelSize: root.pixelSize
            color: root.text_color
            font.family: root.family

        }

        // 浣跨敤灏佽濂界殑杩涘害鏉?
        CustomProgressBar {
            Layout.fillWidth: true
            Layout.preferredHeight: 30 // 缁欐暣涓涓€鐐归珮搴?

            // 缁戝畾灞炴€?
            // indeterminate: root.isIndeterminate
            // value: root.progressValue
        }
    }

    onOpened: {
        loading = true
    }

    onClosed: {
        loading = false
    }

    function openDialog(msg) {
        root.message = msg;
        root.open();
    }
}

