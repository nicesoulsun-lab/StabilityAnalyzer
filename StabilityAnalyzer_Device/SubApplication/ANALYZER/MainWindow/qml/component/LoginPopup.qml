import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12

Popup {
    id: root
    z: 99
    width: 500
    height: 300
    modal: true
    // anchors.centerIn: Overlay.overlay
    closePolicy: Popup.NoAutoClose
    anchors.centerIn: parent

    parent: rootRectangle

    property string confirmText: qsTr("确定")
    property string cancelText: qsTr("取消")
    property int buttonWidth: 120
    property int buttonHeight: 40
    property int buttonSpacing: 60

    property alias usernameInput: usernameInput
    property alias passwordInput: passwordInput

    signal confirmed()
    signal canceled()

    onOpened: {
        popup.open()
    }

    background: Rectangle {
        color: "white"
        radius: 8
        border.color: "transparent"
    }

    Popup{
        id:popup
        width: parent.width
        height: parent.height
        modal: false
        // anchors.centerIn: Overlay.overlay
        anchors.centerIn: parent

        closePolicy: Popup.NoAutoClose
        z:root.z + 1

        background: Rectangle {
            color: "white"
            radius: 8
            border.color: "transparent"
        }
        ColumnLayout {
            anchors.fill: parent
            spacing: 24

            LineEdit {
                id: usernameInput
                placeholderText: qsTr("请输入账号")
                Layout.fillWidth: true
                Layout.preferredHeight: 60
            }

            LineEdit {
                id: passwordInput
                placeholderText: qsTr("请输入密码")
                Layout.fillWidth: true
                Layout.preferredHeight: 60
            }

            RowLayout {
                Layout.alignment: Qt.AlignHCenter
                Layout.topMargin: 20
                spacing: root.buttonSpacing

                IconButton {
                    text: root.cancelText
                    Layout.preferredWidth: 95
                    Layout.preferredHeight: 40
                    button_text: qsTr("取消")
                    button_color: "#FFFFFF"
                    text_color: "#19191A"
                    pixelSize: 16
                    border_color: "#E5E5E5"
                    border_width: 1

                    onClicked: {
                        root.canceled()
                        root.close()
                        popup.close()
                    }
                }

                IconButton {
                    text: root.confirmText
                    Layout.preferredWidth: 95
                    Layout.preferredHeight: 40
                    button_text: qsTr("确认")
                    button_color: "#3B87E4"
                    text_color: "#FFFFFF"
                    pixelSize: 16
                    onClicked: {
                        root.confirmed()
                        root.close()
                        popup.close()
                    }
                }
            }
        }
    }
}
