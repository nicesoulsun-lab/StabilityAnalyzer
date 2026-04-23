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

    property string actualTemperature: "25"
    property var scanRangeStartModel: []
    property var scanRangeEndModel: []
    property var checkTypeOptions: [
        qsTr("透射光（硅油）"),
        qsTr("背射光（硅油）")
    ]

    property bool transmissionActive: true
    property bool backscatterActive: false
    property int measurementStep: 0
    // 记录本次需要执行的检测项，后续接真实设备时可直接替换为异步任务队列。
    property var measurementQueue: []

    property var transmissionRow: emptyRowState()
    property var backscatterRow: emptyRowState()

    function showMessage(message) {
        if (typeof info_pop !== "undefined") {
            info_pop.openDialog(message)
        } else {
            console.log(message)
        }
    }

    function emptyRowState() {
        return {
            first: qsTr("未测试"),
            second: qsTr("未测试"),
            third: qsTr("未测试"),
            result: qsTr("未测试"),
            status: qsTr("未测试"),
            statusColor: "#999999"
        }
    }

    function resetRow(name) {
        if (name === "transmission") {
            transmissionRow = emptyRowState()
        } else {
            backscatterRow = emptyRowState()
        }
    }

    function syncCheckTypeState() {
        transmissionActive = checkTypeCombo.currentIndex === 0
        backscatterActive = checkTypeCombo.currentIndex === 1

        if (!transmissionActive) {
            resetRow("transmission")
        }
        if (!backscatterActive) {
            resetRow("backscatter")
        }
    }

    function resetAll() {
        measuringTimer.stop()
        measurementStep = 0
        measurementQueue = []
        startButton.enabled = true
        actualTemperature = "25"
        targetTempEdit.text = "25"
        scanStartCombo.model = root.scanRangeStartModel
        scanEndCombo.model = root.scanRangeEndModel
        scanStartCombo.currentIndex = 0
        scanEndCombo.currentIndex = 0
        checkTypeCombo.currentIndex = 0
        syncCheckTypeState()
    }

    function formatPercent(value) {
        return value.toFixed(2) + "%"
    }

    function randomValue(base, spread) {
        return base + (Math.random() * 2 - 1) * spread
    }

    function applyMeasurement(kind) {
        var firstValue = 0.0
        var secondValue = 0.0
        var thirdValue = 0.0
        var averageValue = 0.0
        var nextRow = emptyRowState()

        // 当前先用模拟值打通 UI 流程，后续可替换成设备返回的真实结果。
        if (kind === "transmission") {
            firstValue = randomValue(105.05, 0.10)
            secondValue = randomValue(105.01, 0.10)
            thirdValue = randomValue(105.14, 0.10)
            averageValue = (firstValue + secondValue + thirdValue) / 3.0
            nextRow.status = averageValue >= 104.80 && averageValue <= 105.25 ? qsTr("正常") : qsTr("异常")
        } else {
            firstValue = randomValue(98.85, 0.15)
            secondValue = randomValue(98.92, 0.15)
            thirdValue = randomValue(98.88, 0.15)
            averageValue = (firstValue + secondValue + thirdValue) / 3.0
            nextRow.status = averageValue >= 98.50 && averageValue <= 99.20 ? qsTr("正常") : qsTr("异常")
        }

        nextRow.first = formatPercent(firstValue)
        nextRow.second = formatPercent(secondValue)
        nextRow.third = formatPercent(thirdValue)
        nextRow.result = formatPercent(averageValue)
        nextRow.statusColor = nextRow.status === qsTr("正常") ? "#2FA36B" : "#E05656"

        if (kind === "transmission") {
            transmissionRow = nextRow
        } else {
            backscatterRow = nextRow
        }
    }

    function startMeasurement() {
        var rangeStart = parseInt(scanRangeStartModel[scanStartCombo.currentIndex])
        var rangeEnd = parseInt(scanRangeEndModel[scanEndCombo.currentIndex])
        var target = parseFloat(targetTempEdit.text)

        if (temperatureSwitch.checked && (targetTempEdit.text.trim() === "" || isNaN(target))) {
            showMessage(qsTr("请输入目标温度"))
            return
        }

        if (rangeEnd < rangeStart) {
            showMessage(qsTr("扫描区间上限必须大于或等于下限"))
            return
        }

        syncCheckTypeState()
        measurementQueue = []

        if (transmissionActive) {
            measurementQueue.push("transmission")
            resetRow("transmission")
        }
        if (backscatterActive) {
            measurementQueue.push("backscatter")
            resetRow("backscatter")
        }

        actualTemperature = temperatureSwitch.checked ? target.toFixed(0) : "25"
        measurementStep = 0
        startButton.enabled = false
        measuringTimer.start()
    }

    Component.onCompleted: {
        // 扫描区间当前固定为 0~55mm，起始框正序，结束框倒序。
        for (var i = 0; i <= 55; ++i) {
            scanRangeStartModel.push(i)
        }
        for (var j = 55; j >= 0; --j) {
            scanRangeEndModel.push(j)
        }
        resetAll()
    }

    onOpened: {
        resetAll()
    }

    background: Rectangle {
        color: "transparent"
    }

    Timer {
        id: measuringTimer
        interval: 420
        repeat: true
        onTriggered: {
            if (measurementStep >= measurementQueue.length) {
                stop()
                startButton.enabled = true
                return
            }

            // 分步刷新结果，让界面行为更接近真实检测过程。
            applyMeasurement(measurementQueue[measurementStep])
            measurementStep += 1
        }
    }

    Component {
        id: valueCellComponent

        Rectangle {
            property string cellText: ""
            property bool rowActive: true

            Layout.preferredWidth: 170
            Layout.preferredHeight: 42
            radius: 4
            color: rowActive ? "#FFFFFF" : "#F8FAFC"
            border.color: "#E5EAF1"
            border.width: 1

            Text {
                anchors.centerIn: parent
                text: parent.cellText
                font.pixelSize: 16
                color: parent.rowActive ? "#333333" : "#AEB7C3"
                font.family: "Microsoft YaHei"
            }
        }
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
                    text: qsTr("仪器检查")
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
                            text: qsTr("检查参数")
                            font.pixelSize: 18
                            color: "#4A4A4A"
                            //font.bold: true
                            font.family: "Microsoft YaHei"
                        }
                    }

                    GridLayout {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        Layout.margins: 20
                        columns: 3
                        rowSpacing: 18
                        columnSpacing: 60

                        RowLayout {
                            spacing: 12

                            Text {
                                text: qsTr("开启控温")
                                font.pixelSize: 16
                                color: "#333333"
                                font.family: "Microsoft YaHei"
                            }

                            Switch {
                                id: temperatureSwitch
                                checked: true
                                Layout.preferredWidth: 70

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
                        }

                        RowLayout {
                            spacing: 12

                            Text {
                                text: qsTr("目标温度")
                                font.pixelSize: 16
                                color: "#333333"
                                font.family: "Microsoft YaHei"
                            }

                            LineEdit {
                                id: targetTempEdit
                                Layout.preferredWidth: 96
                                Layout.preferredHeight: 36
                                input_en: temperatureSwitch.checked
                                font.pixelSize: 14
                                text: "25"
                                horizontalAlignment: Text.AlignHCenter
                                input_rules: DoubleValidator {
                                    bottom: 0
                                    top: 200
                                    decimals: 1
                                }
                            }

                            Text {
                                text: "°C"
                                font.pixelSize: 16
                                color: "#333333"
                                font.family: "Microsoft YaHei"
                            }
                        }

                        RowLayout {
                            spacing: 12

                            Text {
                                text: qsTr("实际温度")
                                font.pixelSize: 16
                                color: "#333333"
                                font.family: "Microsoft YaHei"
                            }

                            Text {
                                text: root.actualTemperature + "°C"
                                font.pixelSize: 16
                                color: "#333333"
                                font.family: "Microsoft YaHei"
                            }
                        }

                        RowLayout {
                            spacing: 12

                            Text {
                                text: qsTr("检查类型")
                                font.pixelSize: 16
                                color: "#333333"
                                font.family: "Microsoft YaHei"
                            }

                            UiComboBox {
                                id: checkTypeCombo
                                Layout.preferredWidth: 220
                                Layout.preferredHeight: 40
                                model: root.checkTypeOptions
                                onCurrentIndexChanged: root.syncCheckTypeState()
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

                            UiComboBox {
                                id: scanStartCombo
                                Layout.preferredWidth: 92
                                Layout.preferredHeight: 40
                                //model: root.scanRangeStartModel
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
                                //model: root.scanRangeEndModel
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

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 18
                            anchors.rightMargin: 18
                            spacing: 12

                            Item { Layout.preferredWidth: 120 }

                            Repeater {
                                model: [qsTr("第一次"), qsTr("第二次"), qsTr("第三次"), qsTr("结果")]
                                delegate: Text {
                                    Layout.preferredWidth: 170
                                    text: modelData
                                    horizontalAlignment: Text.AlignHCenter
                                    font.pixelSize: 16
                                    color: "#333333"
                                    //font.bold: true
                                    font.family: "Microsoft YaHei"
                                }
                            }

                            Text {
                                Layout.preferredWidth: 90
                                text: qsTr("状态")
                                horizontalAlignment: Text.AlignHCenter
                                font.pixelSize: 16
                                color: "#333333"
                                //font.bold: true
                                font.family: "Microsoft YaHei"
                            }

                            Item { Layout.preferredWidth: 92 }
                        }
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        Layout.margins: 18
                        spacing: 14

                        RowLayout {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 74
                            spacing: 12

                            Text {
                                Layout.preferredWidth: 120
                                text: qsTr("透射光")
                                font.pixelSize: 18
                                color: transmissionActive ? "#333333" : "#B6BDC9"
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                                font.family: "Microsoft YaHei"
                            }

                            Loader {
                                Layout.preferredWidth: 170
                                Layout.preferredHeight: 42
                                sourceComponent: valueCellComponent
                                onLoaded: {
                                    item.cellText = Qt.binding(function() { return transmissionRow.first })
                                    item.rowActive = Qt.binding(function() { return transmissionActive })
                                }
                            }
                            Loader {
                                Layout.preferredWidth: 170
                                Layout.preferredHeight: 42
                                sourceComponent: valueCellComponent
                                onLoaded: {
                                    item.cellText = Qt.binding(function() { return transmissionRow.second })
                                    item.rowActive = Qt.binding(function() { return transmissionActive })
                                }
                            }
                            Loader {
                                Layout.preferredWidth: 170
                                Layout.preferredHeight: 42
                                sourceComponent: valueCellComponent
                                onLoaded: {
                                    item.cellText = Qt.binding(function() { return transmissionRow.third })
                                    item.rowActive = Qt.binding(function() { return transmissionActive })
                                }
                            }
                            Loader {
                                Layout.preferredWidth: 170
                                Layout.preferredHeight: 42
                                sourceComponent: valueCellComponent
                                onLoaded: {
                                    item.cellText = Qt.binding(function() { return transmissionRow.result })
                                    item.rowActive = Qt.binding(function() { return transmissionActive })
                                }
                            }

                            Text {
                                Layout.preferredWidth: 90
                                text: transmissionRow.status
                                color: transmissionActive ? transmissionRow.statusColor : "#AEB7C3"
                                font.pixelSize: 17
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                                font.family: "Microsoft YaHei"
                            }

                            IconButton {
                                Layout.preferredWidth: 92
                                Layout.preferredHeight: 40
                                button_text: qsTr("重置")
                                button_color: "#3B87E4"
                                text_color: "#FFFFFF"
                                onClicked: root.resetRow("transmission")
                            }
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 1
                            color: "#EEF2F6"
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 74
                            spacing: 12

                            Text {
                                Layout.preferredWidth: 120
                                text: qsTr("背射光")
                                font.pixelSize: 18
                                color: backscatterActive ? "#333333" : "#B6BDC9"
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                                font.family: "Microsoft YaHei"
                            }

                            Loader {
                                Layout.preferredWidth: 170
                                Layout.preferredHeight: 42
                                sourceComponent: valueCellComponent
                                onLoaded: {
                                    item.cellText = Qt.binding(function() { return backscatterRow.first })
                                    item.rowActive = Qt.binding(function() { return backscatterActive })
                                }
                            }
                            Loader {
                                Layout.preferredWidth: 170
                                Layout.preferredHeight: 42
                                sourceComponent: valueCellComponent
                                onLoaded: {
                                    item.cellText = Qt.binding(function() { return backscatterRow.second })
                                    item.rowActive = Qt.binding(function() { return backscatterActive })
                                }
                            }
                            Loader {
                                Layout.preferredWidth: 170
                                Layout.preferredHeight: 42
                                sourceComponent: valueCellComponent
                                onLoaded: {
                                    item.cellText = Qt.binding(function() { return backscatterRow.third })
                                    item.rowActive = Qt.binding(function() { return backscatterActive })
                                }
                            }
                            Loader {
                                Layout.preferredWidth: 170
                                Layout.preferredHeight: 42
                                sourceComponent: valueCellComponent
                                onLoaded: {
                                    item.cellText = Qt.binding(function() { return backscatterRow.result })
                                    item.rowActive = Qt.binding(function() { return backscatterActive })
                                }
                            }

                            Text {
                                Layout.preferredWidth: 90
                                text: backscatterRow.status
                                color: backscatterActive ? backscatterRow.statusColor : "#AEB7C3"
                                font.pixelSize: 17
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                                font.family: "Microsoft YaHei"
                            }

                            IconButton {
                                Layout.preferredWidth: 92
                                Layout.preferredHeight: 40
                                button_text: qsTr("重置")
                                button_color: "#3B87E4"
                                text_color: "#FFFFFF"
                                onClicked: root.resetRow("backscatter")
                            }
                        }

                        Item { Layout.fillHeight: true }

                        IconButton {
                            id: startButton
                            Layout.alignment: Qt.AlignHCenter
                            Layout.bottomMargin: 8
                            Layout.preferredWidth: 190
                            Layout.preferredHeight: 48
                            button_text: measuringTimer.running ? qsTr("测量中...") : qsTr("开始测量")
                            button_color: "#3B87E4"
                            text_color: "#FFFFFF"
                            enabled: true
                            onClicked: root.startMeasurement()
                        }
                    }
                }
            }
        }
    }
}
