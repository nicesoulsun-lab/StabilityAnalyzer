import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12

import "component"

Popup {
    id: selectionPopup
    width: 557
    height: 366
    anchors.centerIn: Overlay.overlay
    dim: true
    modal: true
    closePolicy: Popup.CloseOnEscape

    property var channelStates: []

    function channelTitle(index) {
        if (experiment_ctrl && experiment_ctrl.channelDisplayName)
            return experiment_ctrl.channelDisplayName(index)
        return qsTr("通道") + " " + (index + 1)
    }

    function channelPage(index) {
        var suffix = experiment_ctrl && experiment_ctrl.channelName
                ? experiment_ctrl.channelName(index)
                : String.fromCharCode(65 + index)
        return "qrc:/qml/ParaSetting_" + suffix + ".qml"
    }

    function hasSelectedChannel() {
        for (var i = 0; i < channelStates.length; ++i) {
            if (channelStates[i])
                return true
        }
        return false
    }

    onOpened: {
        var count = experiment_ctrl ? experiment_ctrl.channelCount : 4
        channelStates = []
        for (var i = 0; i < count; ++i)
            channelStates.push(false)
    }

    background: Rectangle {
        color: "white"
        radius: 5
        border.color: "#e0e0e0"
        border.width: 1
    }

    contentItem: ColumnLayout {
        anchors.fill: parent
        spacing: 50

        GridLayout {
            Layout.topMargin: 80
            rows: Math.ceil((experiment_ctrl ? experiment_ctrl.channelCount : 4) / 2)
            columns: 2
            rowSpacing: 50
            columnSpacing: 30
            Layout.preferredHeight: 128
            Layout.alignment: Qt.AlignHCenter

            Repeater {
                model: experiment_ctrl ? experiment_ctrl.channelCount : 4

                delegate: RowLayout {
                    Layout.row: Math.floor(index / 2)
                    Layout.column: index % 2
                    Layout.preferredHeight: 51

                    CheckBox {
                        checked: !!selectionPopup.channelStates[index]
                        text: selectionPopup.channelTitle(index)
                        font.pixelSize: 24
                        font.bold: true

                        onCheckedChanged: {
                            var states = selectionPopup.channelStates.slice()
                            states[index] = checked
                            selectionPopup.channelStates = states
                        }
                    }

                    IconButton {
                        button_text: qsTr("设置参数")
                        Layout.preferredWidth: 128
                        Layout.preferredHeight: 42
                        button_color: "#3B87E4"
                        text_color: "#FFFFFF"
                        onClicked: {
                            console.log("设置参数：", selectionPopup.channelTitle(index))
                            mainStackView.push(Qt.resolvedUrl(selectionPopup.channelPage(index)))
                            close()
                        }
                    }
                }
            }
        }

        RowLayout {
            Layout.alignment: Qt.AlignHCenter
            Layout.bottomMargin: 40
            spacing: 64

            IconButton {
                button_text: qsTr("取消")
                Layout.preferredWidth: 120
                Layout.preferredHeight: 42
                button_color: "#EDEEF0"
                text_color: "#333333"
                onClicked: {
                    console.log("取消开始实验")
                    close()
                }
            }

            IconButton {
                button_text: qsTr("确定")
                Layout.preferredWidth: 120
                Layout.preferredHeight: 42
                button_color: "#3B87E4"
                text_color: "#FFFFFF"
                enabled: selectionPopup.hasSelectedChannel()
                onClicked: {
                    console.log("开始实验")
                }
            }
        }
    }
}
