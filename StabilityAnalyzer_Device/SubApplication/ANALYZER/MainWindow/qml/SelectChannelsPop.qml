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

    width: 557
    height: 366
    anchors.centerIn: Overlay.overlay
    dim: true
    modal: true
    closePolicy: Popup.CloseOnEscape

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
                        checked: !!selectionPopup.channelSelectedStates[index]
                        enabled: !Boolean(selectionPopup.channelRunningStates[index])
                        text: selectionPopup.channelTitle(index)
                        font.pixelSize: 24
                        font.bold: true

                        onCheckedChanged: {
                            var selectedStates = selectionPopup.channelSelectedStates.slice()
                            selectedStates[index] = checked
                            selectionPopup.channelSelectedStates = selectedStates
                        }
                    }

                    IconButton {
                        button_text: qsTr("设置参数")
                        Layout.preferredWidth: 128
                        Layout.preferredHeight: 42
                        button_color: "#3B87E4"
                        text_color: "#FFFFFF"
                        enabled: !Boolean(selectionPopup.channelRunningStates[index])
                        onClicked: {
                            console.log("Open params", selectionPopup.channelTitle(index))
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
                    console.log("Cancel start experiment")
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
                    if (!selectionPopup.hasSelectedChannel()) {
                        info_pop.openDialog(qsTr("请选择通道"))
                        return
                    }

                    console.log("Start experiment")
                    var creatorId = user_ctrl && user_ctrl.currentUserId > 0 ? user_ctrl.currentUserId : -1
                    close()

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
