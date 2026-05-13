import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12


Popup {
    id: root
    z: 99999
    width: 500
    height: 150
    modal: true
    anchors.centerIn: Overlay.overlay  // 注意：应该是 Overlay.overlay，不是 OverlayLayout
    closePolicy: Popup.NoAutoClose
    property string message: "提示信息"
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

        // 使用封装好的进度条
        CustomProgressBar {
            Layout.fillWidth: true
            Layout.preferredHeight: 30 // 给整个行一点高度

            // 绑定属性
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
