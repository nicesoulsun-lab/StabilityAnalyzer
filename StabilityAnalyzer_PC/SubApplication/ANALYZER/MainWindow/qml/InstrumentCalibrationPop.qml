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
    closePolicy: Popup.CloseOnEscape
    padding: 0

    // 通道列表（动态获取）
    property var availableChannelOptions: []
    property var availableChannelIndexes: []
    // 校准类型选项
    property var calibrationTypeOptions: [qsTr("透射光校准"), qsTr("背射光校准")]
    // 扫描区间
    property var scanRangeStartModel: []
    property var scanRangeEndModel: []

    // 扫描步长选项（与新建实验一致）
    property var scanStepModel: ["20", "40", "100", "200"]
    // 扫描步长（μm）
    property int scanStepValue: 20
    // 预期采样点数 = (rangeEnd - rangeStart) * 1000 / scanStep
    property int expectedPointCount: 0

    // 校准状态
    property bool isCalibrating: false
    property int calibrationChannel: -1
    property string calibrationType: ""

    // 校准结果
    property int samplePointCount: 0
    property double averageIntensity: 0.0
    property string sendStatus: qsTr("未校准")
    property string sendStatusColor: "#999999"

    function showMessage(message) {
        if (typeof info_pop !== "undefined") {
            info_pop.openDialog(message)
        } else {
            console.log(message)
        }
    }

    // 刷新可用通道列表，过滤掉正在运行实验的通道
    function refreshChannelOptions() {
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
        channelCombo.model = availableChannelOptions
        if (availableChannelIndexes.length > 0) {
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

    // 获取当前选中的通道索引
    function currentChannel() {
        if (channelCombo.currentIndex < 0 || channelCombo.currentIndex >= availableChannelIndexes.length) {
            return -1
        }
        return availableChannelIndexes[channelCombo.currentIndex]
    }

    // 根据扫描区间和步长计算预期采样点数
    function updateExpectedPointCount() {
        var rangeStart = parseInt(scanRangeStartModel[scanStartCombo.currentIndex])
        var rangeEnd = parseInt(scanRangeEndModel[scanEndCombo.currentIndex])
        var step = parseInt(scanStepModel[scanStepCombo.currentIndex]) || 20
        if (step <= 0) step = 20
        scanStepValue = step
        expectedPointCount = (rangeEnd - rangeStart) * 1000 / step
    }

    function resetState() {
        isCalibrating = false
        calibrationChannel = -1
        calibrationType = ""
        samplePointCount = 0
        averageIntensity = 0.0
        expectedPointCount = 0
        sendStatus = qsTr("未校准")
        sendStatusColor = "#999999"
        startButton.enabled = true
        startButton.button_text = qsTr("开始校准")
        if (data_transmit_ctrl) {
            data_transmit_ctrl.streamMessageReceived.disconnect(onStreamData)
        }
    }

    function startCalibration() {
        var channel = currentChannel()
        if (channel < 0) {
            showMessage(qsTr("没有可用通道"))
            return
        }

        var isTransmission = calibrationTypeCombo.currentIndex === 0
        var rangeStart = parseInt(scanRangeStartModel[scanStartCombo.currentIndex])
        var rangeEnd = parseInt(scanRangeEndModel[scanEndCombo.currentIndex])
        var step = parseInt(scanStepModel[scanStepCombo.currentIndex]) || 20

        if (rangeEnd <= rangeStart) {
            showMessage(qsTr("扫描区间上限必须大于下限"))
            return
        }
        if (step <= 0) {
            showMessage(qsTr("扫描步长必须大于0"))
            return
        }

        scanStepValue = step
        updateExpectedPointCount()

        // 保存校准上下文
        calibrationChannel = channel
        calibrationType = isTransmission ? "transmission" : "backscatter"
        samplePointCount = 0
        averageIntensity = 0.0
        sendStatus = qsTr("扫描中...")
        sendStatusColor = "#3B87E4"
        isCalibrating = true
        startButton.enabled = false
        startButton.button_text = qsTr("校准中...")

        // 监听 Stream 通道的校准数据
        if (data_transmit_ctrl) {
            data_transmit_ctrl.streamMessageReceived.connect(onStreamData)
        }

        // 发起校准扫描，步长传给设备端
        var success = experiment_ctrl.startCalibrationScan(channel, rangeStart, rangeEnd, step)
        if (!success) {
            resetState()
            sendStatus = qsTr("启动失败")
            sendStatusColor = "#E05656"
        }
    }

    // Stream 数据回调：接收校准扫描结果
    function onStreamData(message) {
        if (!isCalibrating) return
        if (message.type !== "calibration_scan_data") return
        if (message.channel !== calibrationChannel) return

        var rows = message.rows
        if (!rows || rows.length === 0) {
            sendStatus = qsTr("无数据")
            sendStatusColor = "#E05656"
            finishCalibration()
            return
        }

        // 计算目标光强的总和
        var sum = 0.0
        var count = rows.length
        var field = calibrationType === "transmission"
                    ? "transmission_intensity"
                    : "backscatter_intensity"
        for (var i = 0; i < count; i++) {
            sum += rows[i][field]
        }
        samplePointCount = count

        // 用预期点数作除数，避免丢点导致均值偏高
        averageIntensity = expectedPointCount > 0 ? sum / expectedPointCount : 0

        // 构造校准参数并下发
        var transRef = calibrationType === "transmission" ? Math.round(averageIntensity) : 0
        var backRef = calibrationType === "backscatter" ? Math.round(averageIntensity) : 0

        var calSuccess = experiment_ctrl.sendCalibration(calibrationChannel, transRef, backRef)

        if (calSuccess) {
            sendStatus = qsTr("校准完成")
            sendStatusColor = "#2FA36B"
        } else {
            sendStatus = qsTr("校准失败")
            sendStatusColor = "#E05656"
        }

        finishCalibration()
    }

    function finishCalibration() {
        isCalibrating = false
        startButton.enabled = true
        startButton.button_text = qsTr("开始校准")
        if (data_transmit_ctrl) {
            data_transmit_ctrl.streamMessageReceived.disconnect(onStreamData)
        }
    }

    Component.onCompleted: {
        for (var i = 0; i <= 55; ++i) {
            scanRangeStartModel.push(i)
        }
        for (var j = 55; j >= 0; --j) {
            scanRangeEndModel.push(j)
        }
    }

    onOpened: {
        resetState()
        refreshChannelOptions()
        scanStartCombo.model = scanRangeStartModel
        scanEndCombo.model = scanRangeEndModel
        scanStartCombo.currentIndex = 0
        scanEndCombo.currentIndex = 0
        calibrationTypeCombo.currentIndex = 0
        scanStepCombo.currentIndex = 0
        scanStepValue = 20
        expectedPointCount = 0
    }

    onClosed: resetState()

    // 通道状态变化时刷新可用通道列表
    Connections {
        target: data_transmit_ctrl
        onExperimentChannelsChanged: root.refreshChannelOptions()
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

            // 标题栏
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

            // 校准参数区
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

                        // 第一行：通道选择 + 校准类型
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
                            }
                        }

                        // 第二行：扫描区间 + 扫描步长
                        RowLayout {
                            spacing: 12

                            Text {
                                text: qsTr("扫描区间")
                                font.pixelSize: 16
                                color: "#333333"
                                font.family: "Microsoft YaHei"
                            }

                            UiComboBox {
                                id: scanStartCombo
                                Layout.preferredWidth: 92
                                Layout.preferredHeight: 40
                                pixelSize: 14
                                background: Rectangle {
                                    border.color: "#82C1F2"
                                    radius: 4
                                }
                            }

                            Text {
                                text: "mm ~"
                                font.pixelSize: 15
                                color: "#333333"
                                font.family: "Microsoft YaHei"
                            }

                            UiComboBox {
                                id: scanEndCombo
                                Layout.preferredWidth: 92
                                Layout.preferredHeight: 40
                                pixelSize: 14
                                background: Rectangle {
                                    border.color: "#82C1F2"
                                    radius: 4
                                }
                            }

                            Text {
                                text: "mm"
                                font.pixelSize: 15
                                color: "#333333"
                                font.family: "Microsoft YaHei"
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

                            UiComboBox {
                                id: scanStepCombo
                                Layout.preferredWidth: 120
                                Layout.preferredHeight: 40
                                model: root.scanStepModel
                                onCurrentIndexChanged: root.updateExpectedPointCount()
                                background: Rectangle {
                                    border.color: "#82C1F2"
                                    radius: 4
                                }
                            }

                            Text {
                                text: "μm"
                                font.pixelSize: 15
                                color: "#333333"
                                font.family: "Microsoft YaHei"
                            }
                        }
                    }
                }
            }

            // 校准结果区
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

                        // 采样点数
                        RowLayout {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 54
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
                                Layout.preferredHeight: 36
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
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 1
                            color: "#EEF2F6"
                        }

                        // 预期点数
                        RowLayout {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 54
                            spacing: 12

                            Text {
                                Layout.preferredWidth: 120
                                text: qsTr("预期点数")
                                font.pixelSize: 18
                                color: "#333333"
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                                font.family: "Microsoft YaHei"
                            }

                            Rectangle {
                                Layout.preferredWidth: 170
                                Layout.preferredHeight: 36
                                radius: 4
                                color: "#FFFFFF"
                                border.color: "#E5EAF1"
                                border.width: 1

                                Text {
                                    anchors.centerIn: parent
                                    text: expectedPointCount > 0 ? expectedPointCount.toString() : "-"
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

                        // 平均光强
                        RowLayout {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 54
                            spacing: 12

                            Text {
                                Layout.preferredWidth: 120
                                text: qsTr("平均光强")
                                font.pixelSize: 18
                                color: "#333333"
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                                font.family: "Microsoft YaHei"
                            }

                            Rectangle {
                                Layout.preferredWidth: 170
                                Layout.preferredHeight: 36
                                radius: 4
                                color: "#FFFFFF"
                                border.color: "#E5EAF1"
                                border.width: 1

                                Text {
                                    anchors.centerIn: parent
                                    text: averageIntensity > 0 ? averageIntensity.toFixed(2) : "-"
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

                        // 下发状态
                        RowLayout {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 54
                            spacing: 12

                            Text {
                                Layout.preferredWidth: 120
                                text: qsTr("状态")
                                font.pixelSize: 18
                                color: "#333333"
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                                font.family: "Microsoft YaHei"
                            }

                            Rectangle {
                                Layout.preferredWidth: 170
                                Layout.preferredHeight: 36
                                radius: 4
                                color: "#FFFFFF"
                                border.color: "#E5EAF1"
                                border.width: 1

                                Text {
                                    anchors.centerIn: parent
                                    text: sendStatus
                                    font.pixelSize: 16
                                    color: sendStatusColor
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
