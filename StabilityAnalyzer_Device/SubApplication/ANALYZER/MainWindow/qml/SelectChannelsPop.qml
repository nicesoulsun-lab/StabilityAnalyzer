import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12

import "component"

Popup {
    id: selectionPopup

    property var channelRunningStates: []
    property var channelSelectedStates: []

    function channelTitle(index) {
        if (experiment_ctrl && experiment_ctrl.channelDisplayName)
            return experiment_ctrl.channelDisplayName(index)
        return "Channel " + (index + 1)
    }

    function channelPage(index) {
        var suffix = experiment_ctrl && experiment_ctrl.channelName
                ? experiment_ctrl.channelName(index)
                : String.fromCharCode(65 + index)
        return "qrc:/qml/ParaSetting_" + suffix + ".qml"
    }

    function syncChannelRunningState(channel, status) {
        var runningStates = channelRunningStates.slice()
        var selectedStates = channelSelectedStates.slice()
        runningStates[channel] = Boolean(status && status.running)
        if (runningStates[channel]) {
            selectedStates[channel] = false
        }
        channelRunningStates = runningStates
        channelSelectedStates = selectedStates
    }

    function refreshChannelRunningState() {
        var count = experiment_ctrl ? experiment_ctrl.channelCount : 4
        for (var i = 0; i < count; ++i) {
            syncChannelRunningState(i, experiment_ctrl.getChannelStatus(i))
        }
    }

    function hasSelectedChannel() {
        for (var i = 0; i < channelSelectedStates.length; ++i) {
            if (channelSelectedStates[i]) {
                return true
            }
        }
        return false
    }

    width: 620
    height: 480
    anchors.centerIn: Overlay.overlay
    dim: true
    modal: true
    closePolicy: Popup.CloseOnEscape
    padding: 0

    onOpened: {
        var count = experiment_ctrl ? experiment_ctrl.channelCount : 4
        channelRunningStates = []
        channelSelectedStates = []
        for (var i = 0; i < count; ++i) {
            channelRunningStates.push(false)
            channelSelectedStates.push(false)
        }
        refreshChannelRunningState()
    }

    Connections {
        target: experiment_ctrl
        onChannelStatusUpdated: {
            selectionPopup.syncChannelRunningState(channel, status)
        }
    }

    background: Rectangle {
        color: "#FFFFFF"
        radius: 8
        border.color: "#DCE3EC"
        border.width: 1
    }

    contentItem: Item {
        anchors.fill: parent

        Rectangle {
            id: titleBar
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            height: 48
            radius: 8
            color: "#F3F5F7"

            Rectangle {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                height: parent.radius
                color: "#F3F5F7"
            }

            Text {
                anchors.left: parent.left
                anchors.leftMargin: 20
                anchors.verticalCenter: parent.verticalCenter
                text: qsTr("选择通道")
                font.pixelSize: 20
                font.bold: true
                color: "#333333"
            }
        }

        GridLayout {
            id: channelGrid
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: titleBar.bottom
            anchors.bottom: calibrationBtn.top
            anchors.margins: 20
            rows: Math.ceil((experiment_ctrl ? experiment_ctrl.channelCount : 4) / 2)
            columns: 2
            rowSpacing: 14
            columnSpacing: 14

            Repeater {
                model: experiment_ctrl ? experiment_ctrl.channelCount : 4

                delegate: Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 80
                    radius: 8
                    color: {
                        if (Boolean(selectionPopup.channelRunningStates[index]))
                            return "#F5F5F5"
                        return channelCheckBox.checked ? "#EDF4FF" : "#FAFBFC"
                    }
                    border.color: {
                        if (Boolean(selectionPopup.channelRunningStates[index]))
                            return "#E0E0E0"
                        return channelCheckBox.checked ? "#82C1F2" : "#E7ECF2"
                    }
                    border.width: 2

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 16
                        anchors.rightMargin: 16
                        spacing: 12

                        CheckBox {
                            id: channelCheckBox
                            checked: !!selectionPopup.channelSelectedStates[index]
                            enabled: !Boolean(selectionPopup.channelRunningStates[index])
                            text: selectionPopup.channelTitle(index)
                            font.pixelSize: 20
                            font.bold: true
                            opacity: enabled ? 1.0 : 0.5
                            Layout.fillWidth: true

                            indicator: Rectangle {
                                implicitWidth: 24
                                implicitHeight: 24
                                radius: 4
                                border.color: channelCheckBox.checked ? "#3B87E4" : "#C0C8D4"
                                border.width: 2
                                color: channelCheckBox.checked ? "#3B87E4" : "transparent"
                                y: parent.height / 2 - height / 2

                                Text {
                                    anchors.centerIn: parent
                                    text: "✓"
                                    font.pixelSize: 16
                                    font.bold: true
                                    color: "#FFFFFF"
                                    visible: channelCheckBox.checked
                                }
                            }

                            onCheckedChanged: {
                                var selectedStates = selectionPopup.channelSelectedStates.slice()
                                selectedStates[index] = checked
                                selectionPopup.channelSelectedStates = selectedStates
                            }
                        }

                        IconButton {
                            button_text: qsTr("设置参数")
                            Layout.preferredWidth: 100
                            Layout.preferredHeight: 36
                            button_color: "#3B87E4"
                            text_color: "#FFFFFF"
                            button_radius: 6
                            pixelSize: 13
                            enabled: !Boolean(selectionPopup.channelRunningStates[index])
                            onClicked: {
                                console.log("Open params", selectionPopup.channelTitle(index))
                                mainStackView.push(Qt.resolvedUrl(selectionPopup.channelPage(index)))
                                selectionPopup.close()
                            }
                        }
                    }

                    Rectangle {
                        visible: Boolean(selectionPopup.channelRunningStates[index])
                        anchors.fill: parent
                        radius: 8
                        color: "transparent"

                        Text {
                            anchors.centerIn: parent
                            text: qsTr("实验中")
                            font.pixelSize: 14
                            color: "#999999"
                        }
                    }
                }
            }
        }

        IconButton {
            id: calibrationBtn
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.bottom: buttonRow.top
            anchors.bottomMargin: 16
            button_text: qsTr("设备校准")
            Layout.preferredWidth: 500
            Layout.preferredHeight: 42
            text_color: "#2F7CF6"
            button_color: "#EEF5FF"
            border_color: "#8EA0BC"
            border_width: 2
            button_radius: 8
            pixelSize: 14
            width: 500
            height: 42
            onClicked: {
                selectionPopup.close()
                mainStackView.push("qrc:/qml/CalibrationPage.qml")
            }
        }

        RowLayout {
            id: buttonRow
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 24
            spacing: 48

            IconButton {
                button_text: qsTr("取消")
                Layout.preferredWidth: 130
                Layout.preferredHeight: 44
                button_color: "#EDEEF0"
                text_color: "#333333"
                button_radius: 8
                onClicked: {
                    console.log("Cancel start experiment")
                    selectionPopup.close()
                }
            }

            IconButton {
                button_text: qsTr("确定")
                Layout.preferredWidth: 130
                Layout.preferredHeight: 44
                button_color: enabled ? "#3B87E4" : "#8EA0BC"
                text_color: "#FFFFFF"
                button_radius: 8
                enabled: selectionPopup.hasSelectedChannel()
                onClicked: {
                    if (!selectionPopup.hasSelectedChannel()) {
                        info_pop.openDialog(qsTr("请选择通道"))
                        return
                    }

                    console.log("Start experiment")
                    var creatorId = user_ctrl && user_ctrl.currentUserId > 0 ? user_ctrl.currentUserId : -1
                    selectionPopup.close()

                    for (var i = 0; i < selectionPopup.channelSelectedStates.length; ++i) {
                        if (!selectionPopup.channelSelectedStates[i]) {
                            continue
                        }

                        var ok = experiment_ctrl.startExperiment(i, creatorId)
                        console.log("Start channel:", selectionPopup.channelTitle(i),
                                    "channel=", i,
                                    "result=", ok,
                                    "creatorId=", creatorId)
                    }
                }
            }
        }
    }
}
