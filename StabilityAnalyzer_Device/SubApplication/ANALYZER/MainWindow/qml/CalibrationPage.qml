import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import "component"

Item {
    id: root

    objectName: "CalibrationPage"

    property var calibrationTypeOptions: [qsTr("透射光校准"), qsTr("背射光校准")]
    property var scanStepModel: ["20", "40", "100", "200"]
    property int scanStepValue: 20
    property int expectedPointCount: 0

    property bool isCalibrating: false
    property int calibrationChannel: -1
    property string calibrationType: ""

    property int samplePointCount: 0
    property double averageIntensity: 0.0
    property string calibStatus: qsTr("未校准")
    property string calibStatusColor: "#999999"

    function updateExpectedPointCount() {
        var rangeStart = parseInt(scanStartInput.text) || 0
        var rangeEnd = parseInt(scanEndInput.text) || 0
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
        calibStatus = qsTr("未校准")
        calibStatusColor = "#999999"
        startButton.enabled = true
        startButton.button_text = qsTr("开始校准")
    }

    function startCalibration() {
        var channel = channelCombo.currentIndex
        if (channel < 0) {
            info_pop.openDialog(qsTr("请选择通道"))
            return
        }

        var isTransmission = calibrationTypeCombo.currentIndex === 0
        var rangeStart = parseInt(scanStartInput.text) || 0
        var rangeEnd = parseInt(scanEndInput.text) || 0
        var step = parseInt(scanStepModel[scanStepCombo.currentIndex]) || 20

        if (rangeStart < 0 || rangeStart > 30) {
            info_pop.openDialog(qsTr("扫描区间起始值需在0~30之间"))
            return
        }
        if (rangeEnd < 30 || rangeEnd > 55) {
            info_pop.openDialog(qsTr("扫描区间结束值需在30~55之间"))
            return
        }
        if (rangeEnd <= rangeStart) {
            info_pop.openDialog(qsTr("扫描区间上限必须大于下限"))
            return
        }
        if (step <= 0) {
            info_pop.openDialog(qsTr("扫描步长必须大于0"))
            return
        }

        scanStepValue = step
        updateExpectedPointCount()

        calibrationChannel = channel
        calibrationType = isTransmission ? "transmission" : "backscatter"
        samplePointCount = 0
        averageIntensity = 0.0
        calibStatus = qsTr("扫描中...")
        calibStatusColor = "#3B87E4"
        isCalibrating = true
        startButton.enabled = false
        startButton.button_text = qsTr("校准中...")

        experiment_ctrl.startCalibrationScan(channel, rangeStart, rangeEnd, step)
    }

    function finishCalibration() {
        isCalibrating = false
        startButton.enabled = true
        startButton.button_text = qsTr("开始校准")
    }

    Connections {
        target: experiment_ctrl
        onCalibrationScanDataReady: {
            if (!isCalibrating) return
            if (channel !== calibrationChannel) return

            if (!rows || rows.length === 0) {
                calibStatus = qsTr("无数据")
                calibStatusColor = "#E05656"
                finishCalibration()
                return
            }

            var sum = 0.0
            var count = rows.length
            var field = calibrationType === "transmission"
                        ? "transmission_intensity"
                        : "backscatter_intensity"
            for (var i = 0; i < count; i++) {
                sum += rows[i][field]
            }
            samplePointCount = count
            averageIntensity = expectedPointCount > 0 ? sum / expectedPointCount : 0

            var transRef = calibrationType === "transmission" ? Math.round(averageIntensity) : 0
            var backRef = calibrationType === "backscatter" ? Math.round(averageIntensity) : 0

            experiment_ctrl.updateCalibration(calibrationChannel, transRef, backRef)

            calibStatus = qsTr("校准完成")
            calibStatusColor = "#2FA36B"

            finishCalibration()
        }
        onOperationFailed: {
            if (isCalibrating) {
                calibStatus = qsTr("校准失败")
                calibStatusColor = "#E05656"
                finishCalibration()
            }
        }
    }

    Column {
        anchors.centerIn: parent
        spacing: 20

        Row {
            spacing: 20

            SettingBlock {
                title: qsTr("校准参数")
                titleBold: true
                titleSize: 24
                blockWidth: 440
                blockHeight: 360

                ColumnLayout {
                    anchors.centerIn: parent
                    anchors.verticalCenterOffset: 30
                    spacing: 16
                    width: parent.width - 40

                    RowLayout {
                        Layout.alignment: Qt.AlignHCenter
                        spacing: 12

                        Text {
                            Layout.preferredWidth: 80
                            text: qsTr("通道选择")
                            font.pixelSize: 16
                            color: "#333333"
                            horizontalAlignment: Text.AlignRight
                        }

                        UiComboBox {
                            id: channelCombo
                            Layout.preferredWidth: 180
                            Layout.preferredHeight: 42
                            model: {
                                var names = []
                                var count = experiment_ctrl ? experiment_ctrl.channelCount : 4
                                for (var i = 0; i < count; ++i) {
                                    names.push(experiment_ctrl ? experiment_ctrl.channelDisplayName(i) : "")
                                }
                                return names
                            }
                        }
                    }

                    RowLayout {
                        Layout.alignment: Qt.AlignHCenter
                        spacing: 12

                        Text {
                            Layout.preferredWidth: 80
                            text: qsTr("校准类型")
                            font.pixelSize: 16
                            color: "#333333"
                            horizontalAlignment: Text.AlignRight
                        }

                        UiComboBox {
                            id: calibrationTypeCombo
                            Layout.preferredWidth: 180
                            Layout.preferredHeight: 42
                            model: root.calibrationTypeOptions
                        }
                    }

                    RowLayout {
                        Layout.alignment: Qt.AlignHCenter
                        spacing: 8

                        Text {
                            Layout.preferredWidth: 80
                            text: qsTr("扫描区间")
                            font.pixelSize: 16
                            color: "#333333"
                            horizontalAlignment: Text.AlignRight
                        }

                        LineEdit {
                            id: scanStartInput
                            Layout.preferredWidth: 70
                            Layout.preferredHeight: 42
                            font.pixelSize: 18
                            m_radius: 4
                            border_color: "#82C1F2"
                            text: "0"
                            inputMethodHints: Qt.ImhDigitsOnly
                            input_rules: RegExpValidator { regExp: /^\d*$/ }
                            onTextChanged: root.updateExpectedPointCount()
                        }

                        Text {
                            text: "mm ~"
                            font.pixelSize: 15
                            color: "#333333"
                        }

                        LineEdit {
                            id: scanEndInput
                            Layout.preferredWidth: 70
                            Layout.preferredHeight: 42
                            font.pixelSize: 18
                            m_radius: 4
                            border_color: "#82C1F2"
                            text: "55"
                            inputMethodHints: Qt.ImhDigitsOnly
                            input_rules: RegExpValidator { regExp: /^\d*$/ }
                            onTextChanged: root.updateExpectedPointCount()
                        }

                        Text {
                            text: "mm"
                            font.pixelSize: 15
                            color: "#333333"
                        }
                    }

                    RowLayout {
                        Layout.alignment: Qt.AlignHCenter
                        spacing: 12

                        Text {
                            Layout.preferredWidth: 80
                            text: qsTr("扫描步长")
                            font.pixelSize: 16
                            color: "#333333"
                            horizontalAlignment: Text.AlignRight
                        }

                        UiComboBox {
                            id: scanStepCombo
                            Layout.preferredWidth: 140
                            Layout.preferredHeight: 42
                            model: root.scanStepModel
                            onCurrentIndexChanged: root.updateExpectedPointCount()
                        }

                        Text {
                            text: "μm"
                            font.pixelSize: 15
                            color: "#333333"
                        }
                    }
                }
            }

            SettingBlock {
                title: qsTr("校准结果")
                titleBold: true
                titleSize: 24
                blockWidth: 440
                blockHeight: 360

                ColumnLayout {
                    anchors.centerIn: parent
                    anchors.verticalCenterOffset: 30
                    spacing: 12
                    width: parent.width - 40

                    RowLayout {
                        Layout.alignment: Qt.AlignHCenter
                        spacing: 12

                        Text {
                            Layout.preferredWidth: 100
                            text: qsTr("采样点数")
                            font.pixelSize: 16
                            color: "#333333"
                            horizontalAlignment: Text.AlignRight
                        }

                        Rectangle {
                            Layout.preferredWidth: 160
                            Layout.preferredHeight: 38
                            radius: 4
                            color: "#FFFFFF"
                            border.color: "#E5EAF1"
                            border.width: 1

                            Text {
                                anchors.centerIn: parent
                                text: samplePointCount > 0 ? samplePointCount.toString() : "-"
                                font.pixelSize: 16
                                color: "#333333"
                            }
                        }
                    }

                    RowLayout {
                        Layout.alignment: Qt.AlignHCenter
                        spacing: 12

                        Text {
                            Layout.preferredWidth: 100
                            text: qsTr("预期点数")
                            font.pixelSize: 16
                            color: "#333333"
                            horizontalAlignment: Text.AlignRight
                        }

                        Rectangle {
                            Layout.preferredWidth: 160
                            Layout.preferredHeight: 38
                            radius: 4
                            color: "#FFFFFF"
                            border.color: "#E5EAF1"
                            border.width: 1

                            Text {
                                anchors.centerIn: parent
                                text: expectedPointCount > 0 ? expectedPointCount.toString() : "-"
                                font.pixelSize: 16
                                color: "#333333"
                            }
                        }
                    }

                    RowLayout {
                        Layout.alignment: Qt.AlignHCenter
                        spacing: 12

                        Text {
                            Layout.preferredWidth: 100
                            text: qsTr("平均光强")
                            font.pixelSize: 16
                            color: "#333333"
                            horizontalAlignment: Text.AlignRight
                        }

                        Rectangle {
                            Layout.preferredWidth: 160
                            Layout.preferredHeight: 38
                            radius: 4
                            color: "#FFFFFF"
                            border.color: "#E5EAF1"
                            border.width: 1

                            Text {
                                anchors.centerIn: parent
                                text: averageIntensity > 0 ? averageIntensity.toFixed(2) : "-"
                                font.pixelSize: 16
                                color: "#333333"
                            }
                        }
                    }

                    RowLayout {
                        Layout.alignment: Qt.AlignHCenter
                        spacing: 12

                        Text {
                            Layout.preferredWidth: 100
                            text: qsTr("状态")
                            font.pixelSize: 16
                            color: "#333333"
                            horizontalAlignment: Text.AlignRight
                        }

                        Rectangle {
                            Layout.preferredWidth: 160
                            Layout.preferredHeight: 38
                            radius: 4
                            color: "#FFFFFF"
                            border.color: "#E5EAF1"
                            border.width: 1

                            Text {
                                anchors.centerIn: parent
                                text: calibStatus
                                font.pixelSize: 16
                                color: calibStatusColor
                            }
                        }
                    }

                    IconButton {
                        id: startButton
                        Layout.alignment: Qt.AlignHCenter
                        Layout.topMargin: 10
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
