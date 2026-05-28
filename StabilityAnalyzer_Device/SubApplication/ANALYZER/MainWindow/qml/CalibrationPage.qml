import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import "component"

Item {
    id: root

    objectName: "CalibrationPage"

    property bool isCalibrating: false
    property int calibrationChannel: -1
    property int calibrationRound: 0
    property int calibrationTotalRounds: 3
    property string calibrationType: "transmission"
    property var calibrationTypeOptions: [qsTr("透射光校准"), qsTr("背射光校准")]

    property int samplePointCount: 0
    property double lightAvg: 0.0
    property double lightMax: 0.0
    property double lightMin: 0.0
    property string calibratedLightType: ""
    property string lastTransCalTime: ""
    property string lastBackCalTime: ""
    property string calibStatus: qsTr("未校准")
    property string calibStatusColor: "#999999"

    property string currentCalibrationTime: calibrationTypeCombo.currentIndex === 0 ? lastTransCalTime : lastBackCalTime

    function refreshCalibrationTime() {
        var ch = channelCombo.currentIndex
        if (ch < 0 || !experiment_ctrl) return
        lastTransCalTime = experiment_ctrl.getLastCalibrationTime(ch, "transmission") || ""
        lastBackCalTime = experiment_ctrl.getLastCalibrationTime(ch, "backscatter") || ""
    }

    function resetState() {
        isCalibrating = false
        calibrationChannel = -1
        calibrationRound = 0
        samplePointCount = 0
        lightAvg = 0.0
        lightMax = 0.0
        lightMin = 0.0
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

        calibrationChannel = channel
        calibrationRound = 0
        samplePointCount = 0
        lightAvg = 0.0
        lightMax = 0.0
        lightMin = 0.0
        calibStatus = qsTr("第 1/3 次扫描...")
        calibStatusColor = "#3B87E4"
        isCalibrating = true
        startButton.enabled = false
        startButton.button_text = qsTr("校准中...")

        var calType = calibrationTypeCombo.currentIndex === 0 ? "transmission" : "backscatter"
        experiment_ctrl.startCalibration(channel, calType)
    }

    function finishCalibration() {
        isCalibrating = false
        startButton.enabled = true
        startButton.button_text = qsTr("开始校准")
    }

    Connections {
        target: experiment_ctrl
        onCalibrationProgress: {
            if (channel !== calibrationChannel) return
            calibrationRound = currentRound
            calibStatus = qsTr("第 %1/%2 次扫描...").arg(currentRound).arg(totalRounds)
        }
        onCalibrationCompleted: {
            if (channel !== calibrationChannel) return

            samplePointCount = summary.total_points || 0
            calibratedLightType = summary.calibration_type || ""
            if (calibratedLightType === "transmission") {
                lastTransCalTime = summary.calibrated_at || ""
                lightAvg = summary.overall_avg_transmission || 0.0
                lightMax = summary.max_transmission || 0.0
                lightMin = summary.min_transmission || 0.0
            } else {
                lastBackCalTime = summary.calibrated_at || ""
                lightAvg = summary.overall_avg_backscatter || 0.0
                lightMax = summary.max_backscatter || 0.0
                lightMin = summary.min_backscatter || 0.0
            }

            calibStatus = qsTr("校准完成")
            calibStatusColor = "#2FA36B"
            finishCalibration()
        }
        onCalibrationFailed: {
            if (channel !== calibrationChannel) return
            calibStatus = qsTr("校准失败: %1").arg(reason)
            calibStatusColor = "#E05656"
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

    Component.onCompleted: refreshCalibrationTime()

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

                            onCurrentIndexChanged: {
                                if (currentIndex === 0)
                                    calibratedLightType = "transmission"
                                else
                                    calibratedLightType = "backscatter"
                            }
                        }
                    }

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
                                    if (!experiment_ctrl || !experiment_ctrl.isExperimentRunning(i)) {
                                        names.push(experiment_ctrl ? experiment_ctrl.channelDisplayName(i) : "")
                                    }
                                }
                                return names
                            }
                            onCurrentIndexChanged: root.refreshCalibrationTime()
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

                        Text {
                            Layout.preferredWidth: 180
                            font.pixelSize: 16
                            color: "#666666"
                            text: "0 mm ~ 55 mm"
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

                        Text {
                            Layout.preferredWidth: 180
                            Layout.preferredHeight: 42
                            text: "20 μm"
                            font.pixelSize: 16
                            color: "#666666"
                            verticalAlignment: Text.AlignVCenter
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
                    anchors.verticalCenterOffset: 20
                    spacing: 8
                    width: parent.width - 40

                    RowLayout {
                        Layout.alignment: Qt.AlignHCenter
                        spacing: 12

                        Text {
                            Layout.preferredWidth: 100
                            text: qsTr("采样点数")
                            font.pixelSize: 16
                            color: "#333333"
                            horizontalAlignment: Text.AlignHCenter
                        }

                        Rectangle {
                            Layout.preferredWidth: 160
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
                            horizontalAlignment: Text.AlignHCenter
                        }

                        Rectangle {
                            Layout.preferredWidth: 160
                            Layout.preferredHeight: 32
                            radius: 4
                            color: "#FFFFFF"
                            border.color: "#E5EAF1"
                            border.width: 1

                            Text {
                                anchors.centerIn: parent
                                text: calibStatus
                                font.pixelSize: 12
                                color: calibStatusColor
                            }
                        }
                    }

                    RowLayout {
                        Layout.alignment: Qt.AlignHCenter
                        spacing: 12

                        Text {
                            Layout.preferredWidth: 100
                            text: calibratedLightType === "transmission" ? qsTr("透射光强均值") : qsTr("背射光强均值")
                            font.pixelSize: 16
                            color: "#333333"
                            horizontalAlignment: Text.AlignRight
                        }

                        Rectangle {
                            Layout.preferredWidth: 160
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
                            }
                        }
                    }

                    RowLayout {
                        Layout.alignment: Qt.AlignHCenter
                        spacing: 12

                        Text {
                            Layout.preferredWidth: 100
                            text: calibratedLightType === "transmission" ? qsTr("透射光强范围") : qsTr("背射光强范围")
                            font.pixelSize: 16
                            color: "#333333"
                            horizontalAlignment: Text.AlignRight
                        }

                        Rectangle {
                            Layout.preferredWidth: 160
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
                            }
                        }
                    }

                    RowLayout {
                        Layout.alignment: Qt.AlignHCenter
                        spacing: 12

                        Text {
                            Layout.preferredWidth: 100
                            text: qsTr("最近校准时间")
                            font.pixelSize: 16
                            color: "#333333"
                            horizontalAlignment: Text.AlignRight
                        }

                        Rectangle {
                            Layout.preferredWidth: 160
                            Layout.preferredHeight: 32
                            radius: 4
                            color: "#FFFFFF"
                            border.color: "#E5EAF1"
                            border.width: 1

                            Text {
                                anchors.centerIn: parent
                                text: currentCalibrationTime !== "" ? currentCalibrationTime : "-"
                                font.pixelSize: 13
                                color: "#333333"
                            }
                        }
                    }

                    IconButton {
                        id: startButton
                        Layout.alignment: Qt.AlignHCenter
                        Layout.topMargin: 6
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
