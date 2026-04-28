import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import "component"

Popup {
    id: root
    width: 930
    height: 620
    modal: true
    dim: true
    focus: true
    anchors.centerIn: Overlay.overlay
    closePolicy: Popup.CloseOnEscape
    padding: 0

    property var projectNameList: []
    property var channelOptions: ["A", "B", "C", "D"]
    property var availableChannelOptions: []
    property var availableChannelIndexes: []
    property var scanRangeStartModel: []
    property var scanRangeEndModel: []
    property bool loadingParams: false

    function showMessage(message) {
        if (typeof info_pop !== "undefined")
            info_pop.openDialog(message)
        else
            console.log(message)
    }

    function refreshProjectList() {
        projectNameList = data_ctrl.getProjectName()
        projectCombo.model = projectNameList
    }

    function currentChannel() {
        if (channelCombo.currentIndex < 0 || channelCombo.currentIndex >= availableChannelIndexes.length)
            return -1
        return availableChannelIndexes[channelCombo.currentIndex]
    }

    function isChannelRunning(channelIndex) {
        if (!data_transmit_ctrl || !data_transmit_ctrl.experimentChannels
                || channelIndex < 0 || channelIndex >= data_transmit_ctrl.experimentChannels.length) {
            return false
        }

        var channelInfo = data_transmit_ctrl.experimentChannels[channelIndex]
        return !!(channelInfo && channelInfo.running)
    }

    function refreshAvailableChannels(preferredChannel) {
        var nextOptions = []
        var nextIndexes = []

        for (var i = 0; i < channelOptions.length; ++i) {
            if (!isChannelRunning(i)) {
                nextOptions.push(channelOptions[i])
                nextIndexes.push(i)
            }
        }

        availableChannelOptions = nextOptions
        availableChannelIndexes = nextIndexes
        channelCombo.model = availableChannelOptions

        if (availableChannelIndexes.length === 0) {
            channelCombo.currentIndex = -1
            return
        }

        var targetChannel = preferredChannel
        if (targetChannel === undefined || targetChannel < 0)
            targetChannel = currentChannel()

        var targetIndex = availableChannelIndexes.indexOf(targetChannel)
        channelCombo.currentIndex = targetIndex >= 0 ? targetIndex : 0
    }

    function initModels() {
        scanRangeStartModel = []
        scanRangeEndModel = []
        for (var i = 0; i <= 55; ++i) {
            scanRangeStartModel.push(i)
        }
        for (var j = 55; j >= 0; --j) {
            scanRangeEndModel.push(j)
        }
        scanStartCombo.model = scanRangeStartModel
        scanEndCombo.model = scanRangeEndModel
    }

    function loadChannelParams() {
        var selectedChannel = currentChannel()
        if (selectedChannel < 0) {
            loadingParams = false
            return
        }

        loadingParams = true
        refreshProjectList()

        var params = experiment_ctrl.loadParams(selectedChannel)

        if (projectNameList.length > 0 && params.projectId > 0 && params.projectId <= projectNameList.length)
            projectCombo.currentIndex = params.projectId - 1
        else
            projectCombo.currentIndex = projectNameList.length > 0 ? 0 : -1

        sampleNameEdit.text = params.sampleName || ""
        operatorNameEdit.text = params.operatorName || ""
        noteEdit.text = params.description || ""
        temperatureSwitch.checked = params.temperatureControl || false
        targetTempEdit.text = params.targetTemperature !== undefined ? String(params.targetTemperature) : "25"

        scanStartCombo.currentIndex = Math.max(0, parseInt(params.scanRangeStart || 0))
        scanEndCombo.currentIndex = Math.max(0, 55 - parseInt(params.scanRangeEnd !== undefined ? params.scanRangeEnd : 55))

        durationDayEdit.text = String(parseInt(params.durationDays || 0))
        durationHourEdit.text = String(parseInt(params.durationHours || 0))
        durationMinuteEdit.text = String(parseInt(params.durationMinutes || 0))
        durationSecondEdit.text = String(parseInt(params.durationSeconds || 0))
        intervalHourEdit.text = String(parseInt(params.intervalHours || 0))
        intervalMinuteEdit.text = String(parseInt(params.intervalMinutes || 0))
        intervalSecondEdit.text = String(parseInt(params.intervalSeconds || 0))

        scanCountEdit.text = params.scanCount !== undefined ? String(params.scanCount) : "0"
        loadingParams = false
    }

    function calculateScanCount() {
        if (loadingParams) return

        var durationSeconds = parseInt(durationDayEdit.text || 0) * 86400
                + parseInt(durationHourEdit.text || 0) * 3600
                + parseInt(durationMinuteEdit.text || 0) * 60
                + parseInt(durationSecondEdit.text || 0)
        var intervalSeconds = parseInt(intervalHourEdit.text || 0) * 3600
                + parseInt(intervalMinuteEdit.text || 0) * 60
                + parseInt(intervalSecondEdit.text || 0)

        if (durationSeconds <= 0 || intervalSeconds <= 0) {
            scanCountEdit.text = "0"
            return
        }

        var count = Math.floor(durationSeconds / intervalSeconds)
        scanCountEdit.text = String(Math.max(1, count))
    }

    function saveAndStartExperiment() {
        var selectedChannel = currentChannel()
        var projectId = projectCombo.currentIndex + 1
        var projectName = projectCombo.currentIndex >= 0 && projectCombo.currentIndex < projectNameList.length
                ? String(projectNameList[projectCombo.currentIndex]) : ""
        var scanRangeStart = parseInt(scanRangeStartModel[scanStartCombo.currentIndex] || 0)
        var scanRangeEnd = parseInt(scanRangeEndModel[scanEndCombo.currentIndex] || 55)
        var targetTemperature = parseFloat(targetTempEdit.text || "0")

        if (selectedChannel < 0) {
            showMessage(qsTr("当前没有可用通道，请等待实验结束后再新建实验"))
            return
        }
        if (projectCombo.currentIndex < 0) {
            showMessage(qsTr("请先选择工程"))
            return
        }
        if (sampleNameEdit.text.trim() === "") {
            showMessage(qsTr("请填写样品名称"))
            return
        }
        if (operatorNameEdit.text.trim() === "") {
            showMessage(qsTr("请填写测试者"))
            return
        }
        if (parseInt(scanCountEdit.text || "0") <= 0) {
            showMessage(qsTr("请填写有效的扫描次数"))
            return
        }
        if (scanRangeEnd <= scanRangeStart) {
            showMessage(qsTr("扫描区间上限必须大于下限"))
            return
        }
        if (temperatureSwitch.checked && (targetTempEdit.text.trim() === "" || isNaN(targetTemperature))) {
            showMessage(qsTr("请输入目标温度"))
            return
        }

        experiment_ctrl.saveParams(selectedChannel, {
                                       projectId: projectId,
                                       projectName: projectName,
                                       sampleName: sampleNameEdit.text.trim(),
                                       operatorName: operatorNameEdit.text.trim(),
                                       description: noteEdit.text.trim(),
                                       durationDays: parseInt(durationDayEdit.text || 0),
                                       durationHours: parseInt(durationHourEdit.text || 0),
                                       durationMinutes: parseInt(durationMinuteEdit.text || 0),
                                       durationSeconds: parseInt(durationSecondEdit.text || 0),
                                       intervalHours: parseInt(intervalHourEdit.text || 0),
                                       intervalMinutes: parseInt(intervalMinuteEdit.text || 0),
                                       intervalSeconds: parseInt(intervalSecondEdit.text || 0),
                                       scanCount: parseInt(scanCountEdit.text || "0"),
                                       temperatureControl: temperatureSwitch.checked,
                                       targetTemperature: temperatureSwitch.checked ? targetTemperature : 0,
                                       scanRangeStart: scanRangeStart,
                                       scanRangeEnd: scanRangeEnd,
                                       scanStep: 20
                                   })

        if (experiment_ctrl.startExperiment(selectedChannel, user_ctrl.currentUserId))
            root.close()
    }

    Component.onCompleted: {
        initModels()
        refreshAvailableChannels(0)
        loadChannelParams()
    }

    onOpened: {
        refreshAvailableChannels(currentChannel())
        if (availableChannelIndexes.length === 0) {
            showMessage(qsTr("当前所有通道都在实验中，请等待空闲后再新建实验"))
        }
        loadChannelParams()
        calculateScanCount()
    }

    Connections {
        target: data_transmit_ctrl

        onExperimentChannelsChanged: {
            var previousChannel = root.currentChannel()
            root.refreshAvailableChannels(previousChannel)
            if (root.currentChannel() !== previousChannel || previousChannel < 0) {
                root.loadChannelParams()
            }
        }
    }

    background: Rectangle {
        color: "#FFFFFF"
        radius: 4
        border.color: "#DCE3EC"
        border.width: 1
    }

    contentItem: Item {
        anchors.fill: parent

        Rectangle {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            height: 34
            color: "#F3F5F7"

            Text {
                anchors.left: parent.left
                anchors.leftMargin: 18
                anchors.verticalCenter: parent.verticalCenter
                text: qsTr("新建实验")
                font.pixelSize: 18
                color: "#333333"
                font.family: "Microsoft YaHei"
            }

            Button {
                id: closeButton
                anchors.right: parent.right
                anchors.rightMargin: 10
                anchors.verticalCenter: parent.verticalCenter
                width: 28
                height: 28
                onClicked: root.close()

                contentItem: Text {
                    text: "×"
                    font.pixelSize: 22
                    color: closeButton.down ? "#2B2B2B" : "#5D6775"
                    font.family: "Microsoft YaHei"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }

                background: Rectangle {
                    radius: 14
                    color: closeButton.hovered ? "#E8EDF4" : "transparent"
                }
            }
        }

        Item {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.topMargin: 58
            anchors.bottom: startButton.top
            anchors.bottomMargin: 26
            anchors.leftMargin: 28
            anchors.rightMargin: 28

            SectionBox {
                id: projectSection
                title: qsTr("工程设置")
                x: 0
                y: 0
                width: 310
                height: 164

                GridLayout {
                    anchors.fill: parent
                    anchors.topMargin: 54
                    anchors.leftMargin: 26
                    anchors.rightMargin: 26
                    anchors.bottomMargin: 22
                    columns: 2
                    columnSpacing: 14
                    rowSpacing: 18

                    Text { text: qsTr("测量通道"); font.pixelSize: 16; color: "#333333"; font.family: "Microsoft YaHei" }
                    UiComboBox {
                        id: channelCombo
                        Layout.fillWidth: true
                        Layout.preferredHeight: 36
                        model: root.availableChannelOptions
                        enabled: root.availableChannelOptions.length > 0
                        onCurrentIndexChanged: root.loadChannelParams()
                        background: Rectangle {
                            border.color: channelCombo.enabled ? "#82C1F2" : "#DCE3EC"
                            radius: 4
                            color: channelCombo.enabled ? "#FFFFFF" : "#F3F5F7"
                        }
                        text_color: channelCombo.enabled ? "#333333" : "#98A1AF"
                    }

                    Text { text: qsTr("选择工程"); font.pixelSize: 16; color: "#333333"; font.family: "Microsoft YaHei" }
                    UiComboBox {
                        id: projectCombo
                        Layout.fillWidth: true
                        Layout.preferredHeight: 36
                        model: root.projectNameList
                        background: Rectangle {
                            border.color: "#82C1F2"
                            radius: 4
                        }
                        text_color: "#333333"
                    }
                }
            }

            SectionBox {
                id: infoSection
                title: qsTr("信息设置")
                x: 0
                y: 186
                width: 310
                height: parent.height - 186

                GridLayout {
                    anchors.fill: parent
                    anchors.topMargin: 54
                    anchors.leftMargin: 26
                    anchors.rightMargin: 26
                    anchors.bottomMargin: 22
                    columns: 2
                    columnSpacing: 14
                    rowSpacing: 16

                    Text { text: qsTr("样品名称"); font.pixelSize: 16; color: "#333333"; font.family: "Microsoft YaHei" }
                    LineEdit {
                        id: sampleNameEdit
                        Layout.fillWidth: true
                        Layout.preferredHeight: 36
                        font.pixelSize: 16
                        color: "#333333"
                    }

                    Text { text: qsTr("测试者"); font.pixelSize: 16; color: "#333333"; font.family: "Microsoft YaHei" }
                    LineEdit {
                        id: operatorNameEdit
                        Layout.fillWidth: true
                        Layout.preferredHeight: 36
                        font.pixelSize: 16
                        color: "#333333"
                    }

                    Text {
                        Layout.alignment: Qt.AlignTop
                        text: qsTr("备注")
                        font.pixelSize: 16
                        color: "#333333"
                        font.family: "Microsoft YaHei"
                    }
                    LineEdit_wrap {
                        id: noteEdit
                        Layout.fillWidth: true
                        Layout.preferredHeight: 112
                        multiLine: true
                        maxLines: 4
                        font.pixelSize: 16
                        font.family: "Microsoft YaHei"
                    }
                }
            }

            SectionBox {
                id: baseParamSection
                title: qsTr("参数设置")
                x: 332
                y: 0
                width: parent.width - 332
                height: 225

                GridLayout {
                    anchors.fill: parent
                    anchors.topMargin: 48
                    anchors.leftMargin: 26
                    anchors.rightMargin: 26
                    anchors.bottomMargin: 22
                    columns: 2
                    columnSpacing: 20
                    rowSpacing: 8

                    Text {
                        text: qsTr("开启控温")
                        font.pixelSize: 16
                        color: "#333333"
                        font.family: "Microsoft YaHei"
                        Layout.alignment: Qt.AlignVCenter
                    }
                    Switch {
                        id: temperatureSwitch
                        checked: false
                        Layout.preferredWidth: 70
                        Layout.alignment: Qt.AlignVCenter
                        indicator: Rectangle {
                            implicitWidth: 46
                            implicitHeight: 24
                            radius: 12
                            color: temperatureSwitch.checked ? "#3578F6" : "#D3DAE6"
                            Rectangle {
                                width: 20
                                height: 20
                                radius: 10
                                y: 2
                                x: temperatureSwitch.checked ? 24 : 2
                                color: "#FFFFFF"
                            }
                        }
                        contentItem: Item {}
                    }

                    Text { text: qsTr("目标温度"); font.pixelSize: 16; color: "#333333"; font.family: "Microsoft YaHei" }
                    RowLayout {
                        spacing: 8
                        LineEdit {
                            id: targetTempEdit
                            Layout.preferredWidth: 100
                            Layout.preferredHeight: 36
                            input_en: temperatureSwitch.checked
                            text: "25"
                            horizontalAlignment: Text.AlignHCenter
                        }
                        Text { text: "°C"; font.pixelSize: 16; color: "#333333"; font.family: "Microsoft YaHei" }
                    }

                    Text { text: qsTr("扫描区间"); font.pixelSize: 16; color: "#333333"; font.family: "Microsoft YaHei" }
                    RowLayout {
                        spacing: 8
                        UiComboBox {
                            id: scanStartCombo
                            Layout.preferredWidth: 100
                            Layout.preferredHeight: 36
                            //model: root.scanRangeStartModel
                            background: Rectangle {
                                border.color: "#82C1F2"
                                radius: 4
                            }
                        }
                        Text { text: "mm ~"; font.pixelSize: 16; color: "#333333"; font.family: "Microsoft YaHei" }
                        UiComboBox {
                            id: scanEndCombo
                            Layout.preferredWidth: 100
                            Layout.preferredHeight: 36
                            //model: root.scanRangeEndModel
                            background: Rectangle {
                                border.color: "#82C1F2"
                                radius: 4
                            }
                        }
                        Text { text: "mm"; font.pixelSize: 16; color: "#333333"; font.family: "Microsoft YaHei" }
                    }

                    Text { text: qsTr("扫描间隔"); font.pixelSize: 16; color: "#333333"; font.family: "Microsoft YaHei" }

                    RowLayout {
                        spacing: 8
                        UiComboBox {
                            id: scan_step_combo
                            Layout.preferredWidth: 100
                            Layout.preferredHeight: 36
                            font.pixelSize: 14
                            model: ["20","40","100","200"]
                            background: Rectangle {
                                border.color: "#82C1F2"
                                radius: 4
                            }
                        }
                        Text { text: "µm"; font.pixelSize: 16; color: "#333333"; font.family: "Microsoft YaHei" }
                    }
                }
            }

            SectionBox {
                id: advancedParamSection
                title: qsTr("时间设置")
                x: 332
                y: 236
                width: parent.width - 332
                height: parent.height - 236

                GridLayout {
                    anchors.fill: parent
                    anchors.topMargin: 54
                    anchors.leftMargin: 26
                    anchors.rightMargin: 26
                    anchors.bottomMargin: 22
                    columns: 2
                    columnSpacing: 20
                    rowSpacing: 18

                    Text { text: qsTr("持续时间"); font.pixelSize: 16; color: "#333333"; font.family: "Microsoft YaHei" }
                    RowLayout {
                        spacing: 8
                        LineEdit { id: durationDayEdit; Layout.preferredWidth: 64; Layout.preferredHeight: 36; text: "0"; horizontalAlignment: Text.AlignHCenter; input_rules: IntValidator { bottom: 0; top: 999 } onTextChangedEx: root.calculateScanCount() }
                        Text { text: "d"; font.pixelSize: 16; color: "#333333"; font.family: "Microsoft YaHei" }
                        LineEdit { id: durationHourEdit; Layout.preferredWidth: 64; Layout.preferredHeight: 36; text: "0"; horizontalAlignment: Text.AlignHCenter; input_rules: IntValidator { bottom: 0; top: 999 } onTextChangedEx: root.calculateScanCount() }
                        Text { text: "h"; font.pixelSize: 16; color: "#333333"; font.family: "Microsoft YaHei" }
                        LineEdit { id: durationMinuteEdit; Layout.preferredWidth: 64; Layout.preferredHeight: 36; text: "0"; horizontalAlignment: Text.AlignHCenter; input_rules: IntValidator { bottom: 0; top: 999 } onTextChangedEx: root.calculateScanCount() }
                        Text { text: "min"; font.pixelSize: 16; color: "#333333"; font.family: "Microsoft YaHei" }
                        LineEdit { id: durationSecondEdit; Layout.preferredWidth: 64; Layout.preferredHeight: 36; text: "0"; horizontalAlignment: Text.AlignHCenter; input_rules: IntValidator { bottom: 0; top: 999 } onTextChangedEx: root.calculateScanCount() }
                        Text { text: "s"; font.pixelSize: 16; color: "#333333"; font.family: "Microsoft YaHei" }
                    }

                    Text { text: qsTr("间隔时间"); font.pixelSize: 16; color: "#333333"; font.family: "Microsoft YaHei" }
                    RowLayout {
                        spacing: 8
                        LineEdit { id: intervalHourEdit; Layout.preferredWidth: 64; Layout.preferredHeight: 36; text: "0"; horizontalAlignment: Text.AlignHCenter; input_rules: IntValidator { bottom: 0; top: 999 } onTextChangedEx: root.calculateScanCount() }
                        Text { text: "h"; font.pixelSize: 16; color: "#333333"; font.family: "Microsoft YaHei" }
                        LineEdit { id: intervalMinuteEdit; Layout.preferredWidth: 64; Layout.preferredHeight: 36; text: "0"; horizontalAlignment: Text.AlignHCenter; input_rules: IntValidator { bottom: 0; top: 999 } onTextChangedEx: root.calculateScanCount() }
                        Text { text: "min"; font.pixelSize: 16; color: "#333333"; font.family: "Microsoft YaHei" }
                        LineEdit { id: intervalSecondEdit; Layout.preferredWidth: 64; Layout.preferredHeight: 36; text: "0"; horizontalAlignment: Text.AlignHCenter; input_rules: IntValidator { bottom: 0; top: 999 } onTextChangedEx: root.calculateScanCount() }
                        Text { text: "s"; font.pixelSize: 16; color: "#333333"; font.family: "Microsoft YaHei" }
                    }

                    Text { text: qsTr("扫描次数"); font.pixelSize: 16; color: "#333333"; font.family: "Microsoft YaHei" }
                    LineEdit {
                        id: scanCountEdit
                        Layout.preferredWidth: 150
                        Layout.preferredHeight: 36
                        font.pixelSize: 16
                        border_color: "#DCE3EC"
                        input_rules: IntValidator { bottom: 0; top: 999999 }
                    }
                }
            }
        }

        IconButton {
            id: startButton
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 26
            width: 138
            height: 42
            button_text: qsTr("开始实验")
            button_color: root.availableChannelOptions.length > 0 ? "#3B87E4" : "#AEB8C6"
            text_color: "#FFFFFF"
            enabled: root.availableChannelOptions.length > 0
            onClicked: root.saveAndStartExperiment()
        }
    }
}
