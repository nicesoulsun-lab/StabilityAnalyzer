import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12

import "component"

Item {
    id: pageRoot
    objectName: "ExperimentRecordPage"

    property string recordContentPageUrl: "qrc:/qml/component/ExperimentRecordListContent.qml"
    property int selectedRow: -1
    property var selectedExperiment: ({})

    function openRecordContentPage(pageUrl) {
        if (pageUrl === undefined || pageUrl === null || pageUrl === "")
            return
        recordContentPageUrl = pageUrl
    }

    function resetSelection() {
        selectedRow = -1
        selectedExperiment = ({})
    }

    function selectExperiment(row) {
        selectedRow = row
        if (experiment_list_model) {
            selectedExperiment = experiment_list_model.getRow(row)
        }
    }

    // 只根据复选框状态定位“真正选中”的实验，行点击高亮不参与详情打开。
    function findSingleCheckedRow() {
        if (!experiment_list_model)
            return -1

        for (var row = 0; row < experiment_list_model.count(); ++row) {
            var rowData = experiment_list_model.getRow(row)
            if (rowData.checked) {
                return row
            }
        }

        return -1
    }

    // 根据复选框选中的唯一实验进入详情页，行点击高亮不参与详情打开。
    function openCheckedExperimentDetail() {
        var checkedIds = experiment_list_model ? experiment_list_model.getCheckedExpIds() : []
        if (checkedIds.length === 0) {
            console.log(qsTr("查看实验详情需要先选择实验记录"))
            info_pop.openDialog(qsTr("请勾选实验记录"))
            return
        }
        if (checkedIds.length > 1) {
            console.log(qsTr("查看实验详情只能选择一个实验"))
            info_pop.openDialog(qsTr("无法同时查看多个实验详情，请选择单个实验"))
            return
        }

        var checkedRow = pageRoot.findSingleCheckedRow()
        if (checkedRow >= 0) {
            pageRoot.selectExperiment(checkedRow)
        }

        pageRoot.openRecordContentPage("qrc:/qml/ExperimentDetailPage.qml")
    }

    function valueOrDash(value, suffix) {
        if (value === undefined || value === null || value === "")
            return "--"
        return suffix ? (String(value) + suffix) : String(value)
    }

    function formatDateTime(value) {
        return valueOrDash(value, "")
    }

    function formatDuration(secondsValue) {
        var totalSeconds = parseInt(secondsValue || 0)
        if (totalSeconds <= 0)
            return "--"

        var days = Math.floor(totalSeconds / 86400)
        var hours = Math.floor((totalSeconds % 86400) / 3600)
        var minutes = Math.floor((totalSeconds % 3600) / 60)
        var seconds = totalSeconds % 60
        var parts = []

        if (days > 0)
            parts.push(days + qsTr("天"))
        if (hours > 0)
            parts.push(hours + qsTr("时"))
        if (minutes > 0)
            parts.push(minutes + qsTr("分"))
        if (seconds > 0 || parts.length === 0)
            parts.push(seconds + qsTr("秒"))

        return parts.join("")
    }

    function formatRange() {
        if (!selectedExperiment || selectedExperiment.scan_range_start === undefined || selectedExperiment.scan_range_end === undefined)
            return "--"
        return String(selectedExperiment.scan_range_start) + " - " + String(selectedExperiment.scan_range_end)
    }

    function formatTemperature() {
        if (!selectedExperiment || !selectedExperiment.temperature_control)
            return qsTr("未设置")
        if (selectedExperiment.target_temp === undefined || selectedExperiment.target_temp === null)
            return qsTr("已启用")
        return String(selectedExperiment.target_temp) + qsTr(" ℃")
    }

    function formatTemperatureControl() {
        if (!selectedExperiment || selectedExperiment.temperature_control === undefined || selectedExperiment.temperature_control === null)
            return "--"
        return selectedExperiment.temperature_control ? qsTr("是") : qsTr("否")
    }

    function tryCompare() {
        var checkedIds = experiment_list_model ? experiment_list_model.getCheckedExpIds() : []
        if (checkedIds.length < 2) {
            console.log(qsTr("比较功能需要至少勾选两条实验记录"))
            info_pop.openDialog(qsTr("请至少勾选两条实验记录"))
            return
        }
        console.log(qsTr("比较实验记录:"), checkedIds)
    }

    function tryExport() {
        var checkedIds = experiment_list_model ? experiment_list_model.getCheckedExpIds() : []
        if (checkedIds.length === 0) {
            console.log(qsTr("导出功能需要先选择实验记录"))
            info_pop.openDialog(qsTr("请勾选实验记录"))
            return
        }
        console.log(qsTr("导出实验记录:"), checkedIds)
    }

    function tryDelete() {
        var checkedIds = experiment_list_model ? experiment_list_model.getCheckedExpIds() : []
        if (checkedIds.length === 0) {
            console.log(qsTr("删除前请先勾选实验记录"))
            info_pop.openDialog(qsTr("请勾选实验记录"))
            return
        }
        custom_pop.show(2)
    }

    function tryCheck() {
        var checkedIds = experiment_list_model ? experiment_list_model.getCheckedExpIds() : []
        if (checkedIds.length === 0) {
            console.log(qsTr("查看实验详情需要先选择实验记录"))
            info_pop.openDialog(qsTr("请勾选实验记录"))
            return
        }else if (checkedIds.length > 1) {
            console.log(qsTr("查看实验详情只能选择一个实验"))
            info_pop.openDialog(qsTr("无法同时查看多个实验详情，请选择单个实验"))
            return
        }
        console.log(qsTr("查看实验记录:"), checkedIds)
    }

    readonly property var projectInfoRows: [
        { label: qsTr("工程名"), value: valueOrDash(selectedExperiment.project_name, "") },
        { label: qsTr("备注"), value: valueOrDash(selectedExperiment.description, "") }
    ]

    readonly property var sampleInfoRows: [
        { label: qsTr("样品名称"), value: valueOrDash(selectedExperiment.sample_name, "") },
        { label: qsTr("测试者"), value: valueOrDash(selectedExperiment.operator_name, "") },
        { label: qsTr("备注"), value: valueOrDash(selectedExperiment.description, "") }
    ]

    readonly property var testInfoRows: [
        { label: qsTr("开始时间"), value: formatDateTime(selectedExperiment.created_at) },
        { label: qsTr("持续时间"), value: formatDuration(selectedExperiment.duration) },
        { label: qsTr("测试间隔"), value: valueOrDash(selectedExperiment.interval, qsTr(" 秒")) },
        { label: qsTr("扫描次数"), value: valueOrDash(selectedExperiment.count, "") },
        { label: qsTr("扫描区间"), value: formatRange() },
        { label: qsTr("扫描步长"), value: valueOrDash(selectedExperiment.scan_step, "") },
        { label: qsTr("是否控温"), value: formatTemperatureControl() },
        { label: qsTr("目标温度"), value: formatTemperature() }
    ]

    Rectangle {
        anchors.fill: parent
        color: "#FFFFFF"

        RowLayout {
            anchors.fill: parent
            anchors.margins: 0
            spacing: 2

            Item {
                Layout.fillWidth: true
                Layout.fillHeight: true

                Loader {
                    id: recordContentLoader
                    anchors.fill: parent
                    source: pageRoot.recordContentPageUrl

                    // 详情页加载完成后，把当前实验数据注入进去，后续真实图表可直接复用。
                    onLoaded: {
                        if (item && item.hasOwnProperty("experimentData")) {
                            item.experimentData = pageRoot.selectedExperiment
                        }
                    }
                }

                Connections {
                    target: recordContentLoader.item
                    ignoreUnknownSignals: true

                    onRowSelected: pageRoot.selectExperiment(row)
                    onRowDeselected: pageRoot.resetSelection()
                    onDeleteRequested: pageRoot.tryDelete()
                    onCompareRequested: pageRoot.tryCompare()
                    onExportRequested: pageRoot.tryExport()
                    onCheckRequested: pageRoot.openCheckedExperimentDetail()
                    // 详情页内部返回时，切回记录列表页。
                    onBackRequested: pageRoot.openRecordContentPage("qrc:/qml/component/ExperimentRecordListContent.qml")
                }
            }


            // 实验记录-详细信息
            Rectangle {
                Layout.preferredWidth: 228
                Layout.fillHeight: true
                color: "#FAFBFD"
                border.color: "#EEF1F5"
                border.width: 1

                Column {
                    anchors.fill: parent
                    anchors.margins: 10
                    spacing: 12

                    Rectangle {
                        width: parent.width
                        height: 108
                        color: "#FFFFFF"
                        border.color: "#E3E8EE"
                        border.width: 1

                        Column {
                            anchors.fill: parent
                            spacing: 0

                            Rectangle {
                                width: parent.width
                                height: 28
                                color: "#DDEEFF"

                                Text {
                                    anchors.centerIn: parent
                                    text: qsTr("工程信息")
                                    font.pixelSize: 12
                                    font.bold: true
                                    color: "#4D5C70"
                                    font.family: "Microsoft YaHei"
                                }
                            }

                            Repeater {
                                model: pageRoot.projectInfoRows

                                delegate: Row {
                                    width: parent.width
                                    height: 40

                                    Rectangle {
                                        width: 74
                                        height: parent.height
                                        color: "#FFFFFF"
                                        border.color: "#E6EBF0"
                                        border.width: 1

                                        Text {
                                            anchors.centerIn: parent
                                            text: modelData.label
                                            font.pixelSize: 11
                                            color: "#566274"
                                            font.family: "Microsoft YaHei"
                                        }
                                    }

                                    Rectangle {
                                        width: parent.width - 74
                                        height: parent.height
                                        color: "#FFFFFF"
                                        border.color: "#E6EBF0"
                                        border.width: 1

                                        Text {
                                            anchors.verticalCenter: parent.verticalCenter
                                            anchors.left: parent.left
                                            anchors.leftMargin: 8
                                            anchors.right: parent.right
                                            anchors.rightMargin: 8
                                            text: modelData.value
                                            font.pixelSize: 11
                                            color: "#2F3A4A"
                                            font.family: "Microsoft YaHei"
                                            elide: Text.ElideRight
                                        }
                                    }
                                }
                            }
                        }
                    }

                    Rectangle {
                        width: parent.width
                        height: 136
                        color: "#FFFFFF"
                        border.color: "#E3E8EE"
                        border.width: 1

                        Column {
                            anchors.fill: parent
                            spacing: 0

                            Rectangle {
                                width: parent.width
                                height: 28
                                color: "#DDEEFF"

                                Text {
                                    anchors.centerIn: parent
                                    text: qsTr("样品信息")
                                    font.pixelSize: 12
                                    font.bold: true
                                    color: "#4D5C70"
                                    font.family: "Microsoft YaHei"
                                }
                            }

                            Repeater {
                                model: pageRoot.sampleInfoRows

                                delegate: Row {
                                    width: parent.width
                                    height: 36

                                    Rectangle {
                                        width: 74
                                        height: parent.height
                                        color: "#FFFFFF"
                                        border.color: "#E6EBF0"
                                        border.width: 1

                                        Text {
                                            anchors.centerIn: parent
                                            text: modelData.label
                                            font.pixelSize: 11
                                            color: "#566274"
                                            font.family: "Microsoft YaHei"
                                        }
                                    }

                                    Rectangle {
                                        width: parent.width - 74
                                        height: parent.height
                                        color: "#FFFFFF"
                                        border.color: "#E6EBF0"
                                        border.width: 1

                                        Text {
                                            anchors.verticalCenter: parent.verticalCenter
                                            anchors.left: parent.left
                                            anchors.leftMargin: 8
                                            anchors.right: parent.right
                                            anchors.rightMargin: 8
                                            text: modelData.value
                                            font.pixelSize: 11
                                            color: "#2F3A4A"
                                            font.family: "Microsoft YaHei"
                                            elide: Text.ElideRight
                                        }
                                    }
                                }
                            }
                        }
                    }

                    Rectangle {
                        width: parent.width
                        height: 316
                        color: "#FFFFFF"
                        border.color: "#E3E8EE"
                        border.width: 1

                        Column {
                            anchors.fill: parent
                            spacing: 0

                            Rectangle {
                                width: parent.width
                                height: 28
                                color: "#DDEEFF"

                                Text {
                                    anchors.centerIn: parent
                                    text: qsTr("测试信息")
                                    font.pixelSize: 12
                                    font.bold: true
                                    color: "#4D5C70"
                                    font.family: "Microsoft YaHei"
                                }
                            }

                            Repeater {
                                model: pageRoot.testInfoRows

                                delegate: Row {
                                    width: parent.width
                                    height: 36

                                    Rectangle {
                                        width: 74
                                        height: parent.height
                                        color: "#FFFFFF"
                                        border.color: "#E6EBF0"
                                        border.width: 1

                                        Text {
                                            anchors.centerIn: parent
                                            text: modelData.label
                                            font.pixelSize: 11
                                            color: "#566274"
                                            font.family: "Microsoft YaHei"
                                        }
                                    }

                                    Rectangle {
                                        width: parent.width - 74
                                        height: parent.height
                                        color: "#FFFFFF"
                                        border.color: "#E6EBF0"
                                        border.width: 1

                                        Text {
                                            anchors.verticalCenter: parent.verticalCenter
                                            anchors.left: parent.left
                                            anchors.leftMargin: 8
                                            anchors.right: parent.right
                                            anchors.rightMargin: 8
                                            text: modelData.value
                                            font.pixelSize: 11
                                            color: "#2F3A4A"
                                            font.family: "Microsoft YaHei"
                                            elide: Text.ElideRight
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    Component.onCompleted: {
        if (experiment_list_model) {
            experiment_list_model.reloadFromDb()
        }
    }
}
