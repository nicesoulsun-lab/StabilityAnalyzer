import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12

import "component"

Popup {
    id: root
    property int designWidth: 1180
    property int designHeight: 680
    property real popupScale: 0.8

    width: designWidth * popupScale
    height: designHeight * popupScale
    modal: true
    dim: true
    focus: true
    anchors.centerIn: Overlay.overlay
    closePolicy: Popup.NoAutoClose
    padding: 0

    property var availableChannelOptions: []
    property var availableChannelIndexes: []

    property bool isCalibrating: false
    property int calibrationChannel: -1
    property int calibrationRound: 0
    property int calibrationTotalRounds: 3
    property var calibrationTypeOptions: [qsTr("透射光校准"), qsTr("背射光校准")]

    property int samplePointCount: 0
    property double lightAvg: 0.0
    property double lightMax: 0.0
    property double lightMin: 0.0
    property string calibratedLightType: ""
    property string lastCalibrationTime: ""
    property string sendStatus: qsTr("未校准")
    property string sendStatusColor: "#999999"

    function showMessage(message) {
        if (typeof info_pop !== "undefined") {
            info_pop.openDialog(message)
        } else {
            console.log(message)
        }
    }

    function refreshChannelOptions() {
        var prevChannel = currentChannel()
        var nextAvailable = []
        var nextIndexes = []
        var count = experiment_ctrl ? experiment_ctrl.channelCount : 0
        for (var i = 0; i < count; ++i) {
            var name = experiment_ctrl ? experiment_ctrl.channelDisplayName(i) : ""
            if (!isChannelRunning(i)) {
                nextAvailable.push(name)
                nextIndexes.push(i)
            }
        }
        availableChannelOptions = nextAvailable
        availableChannelIndexes = nextIndexes
        var newIndex = nextIndexes.indexOf(prevChannel)
        if (newIndex >= 0) {
            channelCombo.currentIndex = newIndex
        } else if (nextIndexes.length > 0) {
            channelCombo.currentIndex = 0
        }
    }

    function isChannelRunning(channelIndex) {
        if (!data_transmit_ctrl || !data_transmit_ctrl.experimentChannels
                || channelIndex < 0 || channelIndex >= data_transmit_ctrl.experimentChannels.length) {
            return false
        }
        var info = data_transmit_ctrl.experimentChannels[channelIndex]
        return !!(info && info.running)
    }

    function currentChannel() {
        if (channelCombo.currentIndex < 0 || channelCombo.currentIndex >= availableChannelIndexes.length) {
            return -1
        }
        return availableChannelIndexes[channelCombo.currentIndex]
    }

    function resetState() {
        isCalibrating = false
        calibrationChannel = -1
        calibrationRound = 0
        samplePointCount = 0
        lightAvg = 0.0
        lightMax = 0.0
        lightMin = 0.0
        sendStatus = qsTr("未校准")
        sendStatusColor = "#999999"
        startButton.enabled = true
        startButton.button_text = qsTr("开始校准")
        root.closePolicy = Popup.NoAutoClose
        closeButton.enabled = true
    }

    function startCalibration() {
        var channel = currentChannel()
        if (channel < 0) {
            showMessage(qsTr("没有可用通道"))
            return
        }

        calibrationChannel = channel
        calibrationRound = 0
        samplePointCount = 0
        lightAvg = 0.0
        lightMax = 0.0
        lightMin = 0.0
        sendStatus = qsTr("第 1/3 次扫描...")
        sendStatusColor = "#3B87E4"
        isCalibrating = true
        startButton.enabled = false
        startButton.button_text = qsTr("校准中...")
        root.closePolicy = Popup.NoAutoClose
        closeButton.enabled = false

        var calType = calibrationTypeCombo.currentIndex === 0 ? "transmission" : "backscatter"
        var success = experiment_ctrl.startCalibration(channel, calType)
        if (!success) {
            resetState()
            sendStatus = qsTr("启动失败")
            sendStatusColor = "#E05656"
        }
    }

    function finishCalibration() {
        isCalibrating = false
        startButton.enabled = true
        startButton.button_text = qsTr("开始校准")
        root.closePolicy = Popup.CloseOnEscape
        closeButton.enabled = true
    }

    Component.onCompleted: {} // 保留声明，避免重定义冲突

    onOpened: {
        resetState()
        refreshChannelOptions()
    }

    onClosed: resetState()

    Connections {
        target: data_transmit_ctrl
        onExperimentChannelsChanged: root.refreshChannelOptions()
    }

    Connections {
        target: experiment_ctrl
        onCalibrationProgress: {
            if (channel !== calibrationChannel) return
            calibrationRound = currentRound
            sendStatus = qsTr("第 %1/%2 次扫描...").arg(currentRound).arg(totalRounds)
        }
        onCalibrationCompleted: {
            if (channel !== calibrationChannel) return

            samplePointCount = summary.total_points || 0
            calibratedLightType = summary.calibration_type || ""
            lastCalibrationTime = summary.calibrated_at || ""
            if (calibratedLightType === "transmission") {
                lightAvg = summary.overall_avg_transmission || 0.0
                lightMax = summary.max_transmission || 0.0
                lightMin = summary.min_transmission || 0.0
            } else {
                lightAvg = summary.overall_avg_backscatter || 0.0
                lightMax = summary.max_backscatter || 0.0
                lightMin = summary.min_backscatter || 0.0
            }

            sendStatus = qsTr("校准完成")
            sendStatusColor = "#2FA36B"
            finishCalibration()
        }
        onCalibrationFailed: {
            if (channel !== calibrationChannel) return
            sendStatus = qsTr("校准失败: %1").arg(reason)
            sendStatusColor = "#E05656"
            finishCalibration()
        }
        onOperationFailed: {
            if (isCalibrating) {
                sendStatus = qsTr("校准失败")
                sendStatusColor = "#E05656"
                finishCalibration()
            }
        }
    }

    background: Rectangle {
        color: "transparent"
    }

    Rectangle {
        width: root.designWidth
        height: root.designHeight
        anchors.centerIn: parent
        scale: root.popupScale
        radius: 6
        color: "#FFFFFF"
        border.color: "#DCE3EC"
        border.width: 1

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 18
            spacing: 18

            Item {
                Layout.fillWidth: true
                Layout.preferredHeight: 28

                Text {
                    text: qsTr("仪器校准")
                    anchors.verticalCenter: parent.verticalCenter
                    font.pixelSize: 24
                    color: "#333333"
                    font.family: "Microsoft YaHei"
                }

                Button {
                    id: closeButton
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    width: 28
                    height: 28
                    onClicked: root.close()

                    contentItem: Text {
                        text: "×"
                        font.pixelSize: 24
                        color: closeButton.down ? "#2B2B2B" : "#5D6775"
                        font.family: "Microsoft YaHei"
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }

                    background: Rectangle {
                        radius: 14
                        color: closeButton.hovered ? "#F2F4F8" : "transparent"
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 182
                color: "#FFFFFF"
                radius: 2
                border.color: "#E7ECF2"
                border.width: 1

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 0

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 42
                        color: "#F3F5F7"

                        Text {
                            anchors.centerIn: parent
                            text: qsTr("校准参数")
                            font.pixelSize: 18
                            color: "#4A4A4A"
                            font.family: "Microsoft YaHei"
                        }
                    }

                    GridLayout {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        Layout.margins: 20
                        columns: 2
                        rowSpacing: 18
                        columnSpacing: 60

                        RowLayout {
                            spacing: 12

                            Text {
                                text: qsTr("通道选择")
                                font.pixelSize: 16
                                color: "#333333"
                                font.family: "Microsoft YaHei"
                            }

                            UiComboBox {
                                id: channelCombo
                                Layout.preferredWidth: 220
                                Layout.preferredHeight: 40
                                model: root.availableChannelOptions
                                background: Rectangle {
                                    border.color: "#82C1F2"
                                    radius: 4
                                }
                            }
                        }

                        RowLayout {
                            spacing: 12

                            Text {
                                text: qsTr("扫描区间")
                                font.pixelSize: 16
                                color: "#333333"
                                font.family: "Microsoft YaHei"
                            }

                            Text {
                                Layout.preferredWidth: 220
                                Layout.preferredHeight: 40
                                text: "0 mm ~ 55 mm"
                                font.pixelSize: 16
                                color: "#666666"
                                font.family: "Microsoft YaHei"
                                verticalAlignment: Text.AlignVCenter
                            }
                        }

                        RowLayout {
                            spacing: 12

                            Text {
                                text: qsTr("扫描步长")
                                font.pixelSize: 16
                                color: "#333333"
                                font.family: "Microsoft YaHei"
                            }

                            Text {
                                Layout.preferredWidth: 220
                                Layout.preferredHeight: 40
                                text: "20 μm"
                                font.pixelSize: 16
                                color: "#666666"
                                font.family: "Microsoft YaHei"
                                verticalAlignment: Text.AlignVCenter
                            }
                        }

                        RowLayout {
                            spacing: 12

                            Text {
                                text: qsTr("校准类型")
                                font.pixelSize: 16
                                color: "#333333"
                                font.family: "Microsoft YaHei"
                            }

                            UiComboBox {
                                id: calibrationTypeCombo
                                Layout.preferredWidth: 220
                                Layout.preferredHeight: 40
                                model: root.calibrationTypeOptions
                                background: Rectangle {
                                    border.color: "#82C1F2"
                                    radius: 4
                                }

                                onCurrentIndexChanged: {
                                    if(currentIndex === 0)
                                        calibratedLightType = "transmission"
                                    else{
                                        calibratedLightType = ""
                                    }
                                }
                            }
                        }
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: "#FFFFFF"
                radius: 2
                border.color: "#E7ECF2"
                border.width: 1

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 0

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 44
                        color: "#F3F5F7"

                        Text {
                            anchors.centerIn: parent
                            text: qsTr("校准结果")
                            font.pixelSize: 18
                            color: "#4A4A4A"
                            font.family: "Microsoft YaHei"
                        }
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        Layout.margins: 20
                        spacing: 0

                        RowLayout {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 40
                            spacing: 12

                            Text {
                                Layout.preferredWidth: 120
                                text: qsTr("采样点数")
                                font.pixelSize: 18
                                color: "#333333"
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                                font.family: "Microsoft YaHei"
                            }

                            Rectangle {
                                Layout.preferredWidth: 170
                                Layout.preferredHeight: 32
                                radius: 4
                                color: "#FFFFFF"
                                border.color: "#E5EAF1"
                                border.width: 1

                                Text {
                                    anchors.centerIn: parent
                                    text: samplePointCount > 0 ? samplePointCount.toString() : "-"
                                    font.pixelSize: 16
                                    color: "#333333"
                                    font.family: "Microsoft YaHei"
                                }
                            }

                            Item { Layout.preferredWidth: 28 }

                            Text {
                                Layout.preferredWidth: 30
                                text: qsTr("状态")
                                font.pixelSize: 18
                                color: "#333333"
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                                font.family: "Microsoft YaHei"
                            }

                            Rectangle {
                                Layout.preferredWidth: 150
                                Layout.preferredHeight: 32
                                radius: 4
                                color: "#FFFFFF"
                                border.color: "#E5EAF1"
                                border.width: 1

                                Text {
                                    anchors.centerIn: parent
                                    text: sendStatus
                                    font.pixelSize: 14
                                    color: sendStatusColor
                                    font.family: "Microsoft YaHei"
                                }
                            }
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 1
                            color: "#EEF2F6"
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 40
                            spacing: 12

                            Text {
                                Layout.preferredWidth: 120
                                text: calibratedLightType === "transmission" ? qsTr("透射光强均值") : qsTr("背射光强均值")
                                font.pixelSize: 18
                                color: "#333333"
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                                font.family: "Microsoft YaHei"
                            }

                            Rectangle {
                                Layout.preferredWidth: 170
                                Layout.preferredHeight: 32
                                radius: 4
                                color: "#FFFFFF"
                                border.color: "#E5EAF1"
                                border.width: 1

                                Text {
                                    anchors.centerIn: parent
                                    text: lightAvg > 0 ? lightAvg.toFixed(2) : "-"
                                    font.pixelSize: 16
                                    color: "#333333"
                                    font.family: "Microsoft YaHei"
                                }
                            }
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 1
                            color: "#EEF2F6"
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 40
                            spacing: 12

                            Text {
                                Layout.preferredWidth: 120
                                text: calibratedLightType === "transmission" ? qsTr("透射光强范围") : qsTr("背射光强范围")
                                font.pixelSize: 18
                                color: "#333333"
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                                font.family: "Microsoft YaHei"
                            }

                            Rectangle {
                                Layout.preferredWidth: 170
                                Layout.preferredHeight: 32
                                radius: 4
                                color: "#FFFFFF"
                                border.color: "#E5EAF1"
                                border.width: 1

                                Text {
                                    anchors.centerIn: parent
                                    text: (lightMax > 0 || lightMin > 0)
                                          ? lightMin.toFixed(1) + " ~ " + lightMax.toFixed(1)
                                          : "-"
                                    font.pixelSize: 14
                                    color: "#333333"
                                    font.family: "Microsoft YaHei"
                                }
                            }
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 1
                            color: "#EEF2F6"
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 40
                            spacing: 12

                            Text {
                                Layout.preferredWidth: 120
                                text: qsTr("最近校准时间")
                                font.pixelSize: 18
                                color: "#333333"
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                                font.family: "Microsoft YaHei"
                            }

                            Rectangle {
                                Layout.preferredWidth: 220
                                Layout.preferredHeight: 32
                                radius: 4
                                color: "#FFFFFF"
                                border.color: "#E5EAF1"
                                border.width: 1

                                Text {
                                    anchors.centerIn: parent
                                    text: lastCalibrationTime !== "" ? lastCalibrationTime : "-"
                                    font.pixelSize: 13
                                    color: "#333333"
                                    font.family: "Microsoft YaHei"
                                }
                            }
                        }

                        Item { Layout.fillHeight: true }

                        IconButton {
                            id: startButton
                            Layout.alignment: Qt.AlignHCenter
                            Layout.bottomMargin: 8
                            Layout.preferredWidth: 190
                            Layout.preferredHeight: 48
                            button_text: qsTr("开始校准")
                            button_color: "#3B87E4"
                            text_color: "#FFFFFF"
                            enabled: true
                            onClicked: root.startCalibration()
                        }
                    }
                }
            }
        }
    }
}
