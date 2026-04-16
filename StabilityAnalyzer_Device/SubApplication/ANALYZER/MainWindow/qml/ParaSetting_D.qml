import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import "component"

Item {
    id: homepage
    width: 972
    height: 501

    objectName: "ParaSetting_D"

    // 项目名称列表
    property var nameList: data_ctrl.getProjectName()
    // 通道标识：3-通道D
    property int channel: 3
    // 单次扫描所需时间（秒），便于后续修改
    property int scanTime: 20
    // 防止循环计算的标志
    property bool isCalculating: false

    // 获取持续时间总秒数
    function getDurationSeconds() {
        var days = parseInt(continue_time_day_edit.text) || 0
        var hours = parseInt(continue_time_hour_edit.text) || 0
        var minutes = parseInt(continue_time_min_edit.text) || 0
        var seconds = parseInt(continue_time_sec_edit.text) || 0
        return days * 86400 + hours * 3600 + minutes * 60 + seconds
    }

    // 获取间隔时间总秒数
    function getIntervalSeconds() {
        var hours = parseInt(interval_time_hour_edit.text) || 0
        var minutes = parseInt(interval_time_min_edit.text) || 0
        var seconds = parseInt(interval_time_sec_edit.text) || 0
        return hours * 3600 + minutes * 60 + seconds
    }

    // 检查是否填写了所有必填字段
    function hasAllRequiredFields() {
        // 样品名称不能为空
        if (sample_name_edit.text === "") return false
        // 测试者不能为空
        if (sampler_edit.text === "") return false
        // 持续时间必须大于0
        if (getDurationSeconds() <= 0) return false
        // 间隔时间必须大于0
        if (getIntervalSeconds() <= 0) return false
        return true
    }

    // 根据持续时间和间隔时间自动计算扫描次数
    function calculateScanCount() {
        if (isCalculating) return
        isCalculating = true

        var duration = getDurationSeconds()
        var interval = getIntervalSeconds()

        // 只有当持续时间和间隔时间都大于0时才计算
        if (duration > 0 && interval > 0) {
            var scanCount = Math.floor(duration / interval) + 1
            // 确保至少扫描1次
            if (scanCount < 1) {
                scanCount = 1
            }
            _count_edit.text = scanCount.toString()
        }

        isCalculating = false
    }

    // 扫描区间下拉框数据模型（bottom：0~55）
    property var scanRangeBottomModel: []
    // 扫描区间下拉框数据模型（top：55~0）
    property var scanRangeTopModel: []

    // 初始化扫描区间数据模型
    function initScanRangeModels() {
        // bottom模型：0~55
        scanRangeBottomModel = []
        for (var i = 0; i <= 55; i++) {
            scanRangeBottomModel.push(i)
        }
        region_bottom_combo.model = scanRangeBottomModel

        // top模型：55~0
        scanRangeTopModel = []
        for (var j = 55; j >= 0; j--) {
            scanRangeTopModel.push(j)
        }
        region_top_combo.model = scanRangeTopModel
    }

    // 组件加载完成时，加载保存的参数
    Component.onCompleted: {
        var params = experiment_ctrl.loadParams(channel)
        console.log("加载通道D参数:", params)

        //        if (params.projectId > 0) {
        //            project_combo.currentIndex = params.projectId - 1
        //        }
        project_combo.currentIndex = -1
        sample_name_edit.text = params.sampleName || ""
        sampler_edit.text = params.operatorName || ""
        note_edit.text = params.description || ""

        continue_time_day_edit.text = params.durationDays || 0
        continue_time_hour_edit.text = params.durationHours || 0
        continue_time_min_edit.text = params.durationMinutes || 0
        continue_time_sec_edit.text = params.durationSeconds || 0

        interval_time_hour_edit.text = params.intervalHours || 0
        interval_time_min_edit.text = params.intervalMinutes || 0
        interval_time_sec_edit.text = params.intervalSeconds || 0

        _count_edit.text = params.scanCount || 0

        // 控温值可能来自 QSettings/Variant，显式归一化为布尔值，避免 "false" 被当成真。
        var temperatureControlEnabled = (params.temperatureControl === true
                                         || params.temperatureControl === 1
                                         || params.temperatureControl === "true")
        temp_yes.checked = temperatureControlEnabled
        temp_no.checked = !temperatureControlEnabled
        // 控温为否时，目标温度输入框不可用
        target_temp_edit.enabled = temp_yes.checked
        target_temp_edit.text = params.targetTemperature || 0

        // 设置扫描区间，默认为bottom=0，top=55
        region_bottom_combo.currentIndex = params.scanRangeStart !== undefined ? params.scanRangeStart : 0
        // top下拉框是倒序的，所以需要计算正确的索引
        var topValue = params.scanRangeEnd !== undefined ? params.scanRangeEnd : 55
        region_top_combo.currentIndex = 55 - topValue

        if(params.scanStep === 20) var stepIndex = 0
        else if(params.scanStep === 40) stepIndex = 1
        else if(params.scanStep === 100) stepIndex = 2
        else if(params.scanStep === 200) stepIndex = 3
        else stepIndex = 0
        scan_step_combo.currentIndex = stepIndex
    }

    ColumnLayout {
        anchors.fill: parent
        Layout.topMargin: 10

        // 初始化扫描区间模型（立即初始化，避免下拉框点不开）
        Component.onCompleted: {
            initScanRangeModels()
        }

        Item {
            Layout.fillWidth: true
            Layout.preferredHeight: 426

            Rectangle {
                anchors.fill: parent
                color: "white"
            }

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 0
                spacing: 14

                // 通道标题
                Text {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 20
                    Layout.leftMargin: 50
                    Layout.topMargin: 14
                    text: qsTr("D通道")
                    font.pixelSize: 18
                    font.bold: true
                    color: "#005BAC"
                    verticalAlignment: Text.AlignVCenter
                }

                // 分隔线
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 1
                    color: "#EDEEF0"
                }

                RowLayout {
                    Layout.fillWidth: true; Layout.fillHeight: true
                    Layout.leftMargin: 15; Layout.rightMargin: 15; Layout.bottomMargin: 20
                    spacing: 18

                    // 信息设置区域
                    Rectangle {
                        Layout.fillHeight: true
                        Layout.preferredWidth: 285
                        color: "white"
                        border.color: "#DEDFE0"

                        ColumnLayout {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            spacing: 20

                            // 信息设置标题
                            Label {
                                Layout.preferredWidth: 285
                                Layout.preferredHeight: 36

                                text: qsTr("信息设置")
                                font.pixelSize: 16
                                font.bold: true
                                color: "#555557"
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                                background: Rectangle {
                                    color: "#DEDFE0"
                                }
                            }

                            // 选择工程
                            RowLayout{
                                Layout.preferredWidth: 285
                                Layout.preferredHeight: 36
                                spacing: 8
                                Layout.leftMargin: 14

                                Label {
                                    Layout.preferredWidth: 72
                                    Layout.preferredHeight: 42
                                    text: qsTr("选择工程")
                                    font.pixelSize: 18
                                    font.bold: true
                                    color: "black"
                                    horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
                                }

                                ComboBox {
                                    id: project_combo
                                    Layout.preferredWidth: 172
                                    Layout.preferredHeight: 42
                                    font.pixelSize: 18
                                    model: nameList
                                    background: Rectangle {
                                        border.color: "#82C1F2"
                                        radius: 4
                                    }
                                }
                                Item { Layout.fillWidth: true }
                            }

                            // 样品名称（必填）
                            RowLayout{
                                Layout.preferredWidth: 285
                                Layout.preferredHeight: 36
                                spacing: 8
                                Layout.leftMargin: 14

                                Label {
                                    Layout.preferredWidth: 72
                                    Layout.preferredHeight: 42
                                    text: qsTr("样品名称")
                                    font.pixelSize: 18
                                    font.bold: true
                                    color: "black"
                                    horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
                                }

                                LineEdit {
                                    id: sample_name_edit
                                    Layout.preferredWidth: 172
                                    Layout.preferredHeight: 42
                                    font.pixelSize: 18
                                    m_radius: 4
                                    border_color: "#82C1F2"
                                }
                                Item { Layout.fillWidth: true }
                            }

                            // 测试者（必填）
                            RowLayout{
                                Layout.preferredWidth: 285
                                Layout.preferredHeight: 36
                                spacing: 8
                                Layout.leftMargin: 14

                                Label {
                                    Layout.preferredWidth: 72
                                    Layout.preferredHeight: 42
                                    text: qsTr("测试者")
                                    font.pixelSize: 18
                                    font.bold: true
                                    color: "black"
                                    horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
                                }

                                LineEdit {
                                    id: sampler_edit
                                    Layout.preferredWidth: 172
                                    Layout.preferredHeight: 42
                                    font.pixelSize: 18
                                    m_radius: 4
                                    border_color: "#82C1F2"
                                }
                                Item { Layout.fillWidth: true }
                            }

                            // 备注（选填）
                            RowLayout{
                                Layout.preferredWidth: 285
                                Layout.preferredHeight: 88
                                spacing: 8
                                Layout.leftMargin: 14

                                Label {
                                    Layout.preferredWidth: 72
                                    Layout.preferredHeight: 42
                                    text: qsTr("备注")
                                    font.pixelSize: 18
                                    font.bold: true
                                    color: "black"
                                    horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
                                }

                                LineEdit_wrap {
                                    id: note_edit
                                    Layout.preferredWidth: 172
                                    Layout.preferredHeight: 88
                                    font.pixelSize: 18
                                    m_radius: 4
                                    border_color: "#82C1F2"
                                    multiLine: true
                                    maxLines: 3
                                }
                                Item { Layout.fillWidth: true }
                            }
                        }
                    }

                    // 时间设置区域
                    Rectangle {
                        Layout.fillHeight: true
                        Layout.preferredWidth: 300
                        color: "white"
                        border.color: "#DEDFE0"

                        ColumnLayout {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            spacing: 16

                            // 时间设置标题
                            Label {
                                Layout.preferredWidth: 300
                                Layout.preferredHeight: 36
                                text: qsTr("时间设置")
                                font.pixelSize: 16
                                font.bold: true
                                color: "#555557"
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                                background: Rectangle {
                                    color: "#DEDFE0"
                                }
                            }

                            // 持续时间（必填）
                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 8
                                Layout.leftMargin: 14

                                Label {
                                    Layout.preferredWidth: 72
                                    Layout.preferredHeight: 42
                                    text: qsTr("持续时间")
                                    font.pixelSize: 18
                                    font.bold: true
                                    color: "black"
                                    horizontalAlignment: Text.AlignRight
                                    verticalAlignment: Text.AlignVCenter
                                }

                                LineEdit {
                                    id: continue_time_day_edit
                                    Layout.preferredWidth: 50
                                    Layout.preferredHeight: 42
                                    font.pixelSize: 18
                                    m_radius: 4
                                    border_color: "#82C1F2"
                                    horizontalAlignment: TextInput.AlignHCenter
                                    validator: IntValidator { bottom: 0; top: 999 }
                                    onTextChanged: {
                                        calculateScanCount()
                                    }
                                    inputMethodHints:Qt.ImhDigitsOnly
                                }

                                Label {
                                    Layout.preferredWidth: 35
                                    Layout.preferredHeight: 42
                                    text: "day"
                                    font.pixelSize: 16
                                    font.bold: true
                                    color: "black"
                                    verticalAlignment: Text.AlignVCenter
                                    horizontalAlignment: Text.AlignLeft
                                }

                                LineEdit {
                                    id: continue_time_hour_edit
                                    Layout.preferredWidth: 50
                                    Layout.preferredHeight: 42
                                    font.pixelSize: 18
                                    m_radius: 4
                                    border_color: "#82C1F2"
                                    horizontalAlignment: TextInput.AlignHCenter
                                    validator: IntValidator { bottom: 0; top: 23 }
                                    onTextChanged: {
                                        calculateScanCount()
                                    }
                                    inputMethodHints:Qt.ImhDigitsOnly
                                }

                                Label {
                                    Layout.preferredWidth: 15
                                    Layout.preferredHeight: 42
                                    text: "h"
                                    font.pixelSize: 16
                                    font.bold: true
                                    color: "black"
                                    verticalAlignment: Text.AlignVCenter
                                    horizontalAlignment: Text.AlignLeft
                                }

                                Item { Layout.fillWidth: true }
                            }

                            // 持续时间（分钟和秒）
                            RowLayout {
                                Layout.fillWidth: true
                                Layout.leftMargin: 14
                                spacing: 8

                                Item {
                                    Layout.preferredWidth: 72
                                    Layout.preferredHeight: 42
                                }

                                LineEdit {
                                    id: continue_time_min_edit
                                    Layout.preferredWidth: 50
                                    Layout.preferredHeight: 42
                                    font.pixelSize: 18
                                    m_radius: 4
                                    border_color: "#82C1F2"
                                    horizontalAlignment: TextInput.AlignHCenter
                                    validator: IntValidator { bottom: 0; top: 59 }
                                    onTextChanged: {
                                        calculateScanCount()
                                    }
                                    inputMethodHints:Qt.ImhDigitsOnly
                                }

                                Label {
                                    Layout.preferredWidth: 35
                                    Layout.preferredHeight: 42
                                    text: "min"
                                    font.pixelSize: 16
                                    font.bold: true
                                    color: "black"
                                    verticalAlignment: Text.AlignVCenter
                                    horizontalAlignment: Text.AlignLeft
                                }

                                LineEdit {
                                    id: continue_time_sec_edit
                                    Layout.preferredWidth: 50
                                    Layout.preferredHeight: 42
                                    font.pixelSize: 18
                                    m_radius: 4
                                    border_color: "#82C1F2"
                                    horizontalAlignment: TextInput.AlignHCenter
                                    validator: IntValidator { bottom: 0; top: 59 }
                                    onTextChanged: {
                                        calculateScanCount()
                                    }
                                    inputMethodHints:Qt.ImhDigitsOnly
                                }

                                Label {
                                    Layout.preferredWidth: 15
                                    Layout.preferredHeight: 42
                                    text: "s"
                                    font.pixelSize: 16
                                    font.bold: true
                                    color: "black"
                                    verticalAlignment: Text.AlignVCenter
                                    horizontalAlignment: Text.AlignLeft
                                }

                                Item { Layout.fillWidth: true }
                            }

                            // 间隔时间（必填）
                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 8
                                Layout.leftMargin: 14

                                Label {
                                    Layout.preferredWidth: 72
                                    Layout.preferredHeight: 42
                                    text: qsTr("间隔时间")
                                    font.pixelSize: 18
                                    font.bold: true
                                    color: "black"
                                    horizontalAlignment: Text.AlignRight
                                    verticalAlignment: Text.AlignVCenter
                                }

                                LineEdit {
                                    id: interval_time_hour_edit
                                    Layout.preferredWidth: 50
                                    Layout.preferredHeight: 42
                                    font.pixelSize: 18
                                    m_radius: 4
                                    border_color: "#82C1F2"
                                    horizontalAlignment: TextInput.AlignHCenter
                                    validator: IntValidator { bottom: 0; top: 999 }
                                    onTextChanged: {
                                        calculateScanCount()
                                    }
                                    inputMethodHints:Qt.ImhDigitsOnly
                                }

                                Label {
                                    Layout.preferredWidth: 15
                                    Layout.preferredHeight: 42
                                    text: "h"
                                    font.pixelSize: 16
                                    font.bold: true
                                    color: "black"
                                    verticalAlignment: Text.AlignVCenter
                                    horizontalAlignment: Text.AlignLeft
                                }

                                LineEdit {
                                    id: interval_time_min_edit
                                    Layout.preferredWidth: 50
                                    Layout.preferredHeight: 42
                                    font.pixelSize: 18
                                    m_radius: 4
                                    border_color: "#82C1F2"
                                    horizontalAlignment: TextInput.AlignHCenter
                                    validator: IntValidator { bottom: 0; top: 23 }
                                    onTextChanged: {
                                        calculateScanCount()
                                    }
                                    inputMethodHints:Qt.ImhDigitsOnly
                                }

                                Label {
                                    Layout.preferredWidth: 35
                                    Layout.preferredHeight: 42
                                    text: "min"
                                    font.pixelSize: 16
                                    font.bold: true
                                    color: "black"
                                    verticalAlignment: Text.AlignVCenter
                                    horizontalAlignment: Text.AlignLeft
                                }

                                Item { Layout.fillWidth: true }
                            }

                            // 间隔时间（秒）
                            RowLayout {
                                Layout.fillWidth: true
                                Layout.leftMargin: 14
                                spacing: 8

                                Item {
                                    Layout.preferredWidth: 72
                                    Layout.preferredHeight: 42
                                }

                                LineEdit {
                                    id: interval_time_sec_edit
                                    Layout.preferredWidth: 50
                                    Layout.preferredHeight: 42
                                    font.pixelSize: 18
                                    m_radius: 4
                                    border_color: "#82C1F2"
                                    horizontalAlignment: TextInput.AlignHCenter
                                    validator: IntValidator { bottom: 0; top: 59 }
                                    onTextChanged: {
                                        calculateScanCount()
                                    }
                                    inputMethodHints:Qt.ImhDigitsOnly
                                }

                                Label {
                                    Layout.preferredWidth: 15
                                    Layout.preferredHeight: 42
                                    text: "s"
                                    font.pixelSize: 16
                                    font.bold: true
                                    color: "black"
                                    verticalAlignment: Text.AlignVCenter
                                    horizontalAlignment: Text.AlignLeft
                                }

                                Item { Layout.fillWidth: true }
                            }

                            // 扫描次数（自动计算）
                            RowLayout{
                                Layout.preferredWidth: 285
                                Layout.preferredHeight: 36
                                spacing: 8
                                Layout.leftMargin: 14

                                Label {
                                    Layout.preferredWidth: 72
                                    Layout.preferredHeight: 42
                                    text: qsTr("扫描次数")
                                    font.pixelSize: 18
                                    font.bold: true
                                    color: "black"
                                    horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
                                }

                                LineEdit {
                                    id: _count_edit
                                    Layout.preferredWidth: 138
                                    Layout.preferredHeight: 42
                                    font.pixelSize: 18
                                    m_radius: 4
                                    border_color: "#82C1F2"
                                    readOnly: true
                                }
                                Item { Layout.fillWidth: true }
                            }
                        }
                    }

                    // 参数设置区域
                    Rectangle {
                        Layout.fillHeight: true
                        Layout.preferredWidth: 320
                        color: "white"
                        border.color: "#DEDFE0"

                        ColumnLayout {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            spacing: 20

                            // 参数设置标题
                            Label {
                                Layout.preferredWidth: 320
                                Layout.preferredHeight: 36

                                text: qsTr("参数设置")
                                font.pixelSize: 16
                                font.bold: true
                                color: "#555557"
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                                background: Rectangle {
                                    color: "#DEDFE0"
                                }
                            }

                            // 控温（默认为否）
                            RowLayout{
                                Layout.preferredWidth: 285
                                Layout.preferredHeight: 36
                                spacing: 8
                                Layout.leftMargin: 14

                                Label {
                                    Layout.preferredWidth: 72
                                    Layout.preferredHeight: 42
                                    text: qsTr("控温")
                                    font.pixelSize: 18
                                    font.bold: true
                                    color: "black"
                                    horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
                                }

                                RadioButton {
                                    id: temp_yes
                                    text: qsTr("是")
                                    Layout.alignment: Qt.AlignVCenter
                                    Layout.preferredHeight: 42
                                    font.pixelSize: 16
                                    onCheckedChanged: {
                                        // 控温为是时，目标温度输入框可用
                                        target_temp_edit.enabled = checked
                                    }
                                }

                                RadioButton {
                                    id: temp_no
                                    text: qsTr("否")
                                    Layout.alignment: Qt.AlignVCenter
                                    Layout.preferredHeight: 42
                                    font.pixelSize: 16
                                    checked: true
                                }
                                Item { Layout.fillWidth: true }
                            }

                            // 目标温度
                            RowLayout{
                                Layout.preferredWidth: 285
                                Layout.preferredHeight: 36
                                spacing: 8
                                Layout.leftMargin: 14

                                Label {
                                    Layout.preferredWidth: 72
                                    Layout.preferredHeight: 42
                                    text: qsTr("目标温度")
                                    font.pixelSize: 18
                                    font.bold: true
                                    color: "black"
                                    horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
                                }

                                LineEdit {
                                    id: target_temp_edit
                                    Layout.preferredWidth: 168
                                    Layout.preferredHeight: 42
                                    font.pixelSize: 18
                                    m_radius: 4
                                    border_color: "#82C1F2"
                                    enabled: false
                                    inputMethodHints:Qt.ImhDigitsOnly
                                }

                                Label {
                                    Layout.preferredWidth: 20
                                    Layout.preferredHeight: 42
                                    text: "℃"
                                    font.pixelSize: 18
                                    font.bold: true
                                    color: "black"
                                    horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
                                }
                                Item { Layout.fillWidth: true }
                            }

                            // 扫描区间
                            RowLayout{
                                Layout.preferredWidth: 285
                                Layout.preferredHeight: 36
                                spacing: 8
                                Layout.leftMargin: 14

                                Label {
                                    Layout.preferredWidth: 72
                                    Layout.preferredHeight: 42
                                    text: qsTr("扫描区间")
                                    font.pixelSize: 18
                                    font.bold: true
                                    color: "black"
                                    horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
                                }

                                ComboBox {
                                    id: region_bottom_combo
                                    Layout.preferredWidth: 72
                                    Layout.preferredHeight: 42
                                    font.pixelSize: 18
                                    model: scanRangeBottomModel
                                    background: Rectangle {
                                        border.color: "#82C1F2"
                                        radius: 4
                                    }
                                }

                                Label {
                                    Layout.preferredWidth: 10
                                    Layout.preferredHeight: 42
                                    text: "~"
                                    font.pixelSize: 18
                                    font.bold: true
                                    color: "black"
                                    horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
                                }

                                ComboBox {
                                    id: region_top_combo
                                    Layout.preferredWidth: 72
                                    Layout.preferredHeight: 42
                                    font.pixelSize: 18
                                    model: scanRangeTopModel
                                    background: Rectangle {
                                        border.color: "#82C1F2"
                                        radius: 4
                                    }
                                }

                                Label {
                                    Layout.preferredWidth: 20
                                    Layout.preferredHeight: 42
                                    text: "mm"
                                    font.pixelSize: 18
                                    font.bold: true
                                    color: "black"
                                    horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
                                }
                                Item { Layout.fillWidth: true }
                            }

                            // 扫描间隔（必填）
                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 8
                                Layout.leftMargin: 14

                                Label {
                                    Layout.preferredWidth: 72
                                    Layout.preferredHeight: 42
                                    text: qsTr("扫描间隔")
                                    font.pixelSize: 18
                                    font.bold: true
                                    color: "black"
                                    horizontalAlignment: Text.AlignRight
                                    verticalAlignment: Text.AlignVCenter
                                }

                                ComboBox {
                                    id: scan_step_combo
                                    Layout.preferredWidth: 168
                                    Layout.preferredHeight: 42
                                    font.pixelSize: 18
                                    model: ["20","40","50","100","200"]
                                    background: Rectangle {
                                        border.color: "#82C1F2"
                                        radius: 4
                                    }
                                }

                                Label {
                                    Layout.preferredWidth: 20
                                    Layout.preferredHeight: 42
                                    text: "um"
                                    font.pixelSize: 18
                                    font.bold: true
                                    color: "black"
                                    horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
                                }

                                Item { Layout.fillWidth: true }
                            }
                        }
                    }
                }
            }
        }

        // 应用按钮
        IconButton {
            button_text: qsTr("应   用")
            Layout.preferredWidth: 160
            Layout.preferredHeight: 45
            Layout.alignment: Qt.AlignHCenter
            Layout.topMargin: 10
            button_color: "#3B87E4"
            text_color: "#FFFFFF"
            pixelSize: 18

            onClicked: {
                console.log("应用通道D参数设置")

                // 验证：样品名称必须填写
                if (project_combo.currentIndex < 0) {
                    info_pop.openDialog("请选择工程，若暂无工程请先添加工程")
                    return
                }
                if (sample_name_edit.text === "") {
                    info_pop.openDialog("请填写样品名称")
                    return
                }

                // 验证：测试者必须填写
                if (sampler_edit.text === "") {
                    info_pop.openDialog("请填写测试者")
                    return
                }

                // 验证：持续时间必须填写
                if (getDurationSeconds() <= 0) {
                    info_pop.openDialog("请填写持续时间")
                    return
                }

                // 验证：间隔时间必须填写
                if (getIntervalSeconds() <= 0) {
                    info_pop.openDialog("请填写间隔时间")
                    return
                }

                // 验证：扫描区间上限必须大于下限
                var rangeStart = region_bottom_combo.currentIndex
                var rangeEnd = scanRangeTopModel[region_top_combo.currentIndex]
                if (rangeEnd <= rangeStart) {
                    info_pop.openDialog("扫描区间上限必须大于下限")
                    return
                }

                // 构建参数对象
                var params = {
                    projectId: project_combo.currentIndex + 1,
                    sampleName: sample_name_edit.text,
                    operatorName: sampler_edit.text,
                    description: note_edit.text,
                    durationDays: parseInt(continue_time_day_edit.text) || 0,
                    durationHours: parseInt(continue_time_hour_edit.text) || 0,
                    durationMinutes: parseInt(continue_time_min_edit.text) || 0,
                    durationSeconds: parseInt(continue_time_sec_edit.text) || 0,
                    intervalHours: parseInt(interval_time_hour_edit.text) || 0,
                    intervalMinutes: parseInt(interval_time_min_edit.text) || 0,
                    intervalSeconds: parseInt(interval_time_sec_edit.text) || 0,
                    scanCount: parseInt(_count_edit.text) || 0,
                    temperatureControl: temp_yes.checked,
                    targetTemperature: parseFloat(target_temp_edit.text) || 0,
                    scanRangeStart: rangeStart,
                    scanRangeEnd: rangeEnd,
                    scanStep: scan_step_combo.currentText
                }

                console.log("保存参数:", params)
                experiment_ctrl.saveParams(channel, params)

                if(mainStackView.currentItem.objectName === "ParaSetting_D")
                {
                    mainStackView.pop()
                    select_channel_pop.open()
                }
            }
        }
    }
}
