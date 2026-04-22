import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12

import "component"

Item {
    id: pageRoot
    objectName: "RecycleBinPage"

    property int selectedRow: -1
    property var selectedExperiment: ({})

    function resetSelection() {
        selectedRow = -1
        selectedExperiment = ({})
    }

    function selectExperiment(row) {
        selectedRow = row
        if (recycle_experiment_list_model) {
            selectedExperiment = recycle_experiment_list_model.getRow(row)
        }
    }

    function checkedIds() {
        return recycle_experiment_list_model ? recycle_experiment_list_model.getCheckedExpIds() : []
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

    function tryRestore() {
        var ids = checkedIds()
        if (ids.length === 0) {
            info_pop.openDialog(qsTr("请勾选实验记录"))
            return
        }

        var successCount = 0
        for (var i = 0; i < ids.length; ++i) {
            if (data_ctrl.restoreExperiment(ids[i])) {
                successCount++
            }
        }

        if (successCount > 0) {
            experiment_list_model.reloadFromDb()
            recycle_experiment_list_model.reloadFromDb()
            resetSelection()
        }
    }

    function tryDelete() {
        var ids = checkedIds()
        if (ids.length === 0) {
            info_pop.openDialog(qsTr("请勾选实验记录"))
            return
        }

        custom_pop.show(4)
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
        { label: qsTr("目标温度"), value: formatTemperature() },
        { label: qsTr("删除时间"), value: formatDateTime(selectedExperiment.deleted_at) },
        { label: qsTr("清理时间"), value: formatDateTime(selectedExperiment.purge_after) }
    ]

    Rectangle {
        anchors.fill: parent
        color: "#FFFFFF"

        ColumnLayout {
            anchors.fill: parent
            spacing: 0

            Item {
                Layout.fillWidth: true
                Layout.preferredHeight: 56

                Text {
                    anchors.left: parent.left
                    anchors.leftMargin: 18
                    anchors.verticalCenter: parent.verticalCenter
                    text: qsTr("回收站")
                    font.pixelSize: 22
                    //font.bold: true
                    color: "#2F3A4A"
                    font.family: "Microsoft YaHei"
                }
            }

            RowLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 2

                Item {
                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    ExperimentRecordListContent {
                        anchors.fill: parent
                        experimentListModel: recycle_experiment_list_model
                        showCheckButton: false
                        showCompareButton: false
                        showExportButton: false
                        showDeleteButton: true
                        showRestoreButton: true
                        deleteButtonText: qsTr("删除")
                        restoreButtonText: qsTr("还原")

                        onRowSelected: pageRoot.selectExperiment(row)
                        onRowDeselected: pageRoot.resetSelection()
                        onDeleteRequested: pageRoot.tryDelete()
                        onRestoreRequested: pageRoot.tryRestore()
                    }
                }

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

                        Repeater {
                            model: [
                                { title: qsTr("工程信息"), rows: pageRoot.projectInfoRows, rowHeight: 40, boxHeight: 108 },
                                { title: qsTr("样品信息"), rows: pageRoot.sampleInfoRows, rowHeight: 36, boxHeight: 136 },
                                { title: qsTr("测试信息"), rows: pageRoot.testInfoRows, rowHeight: 36, boxHeight: 388 }
                            ]

                            delegate: Rectangle {
                                property var sectionData: modelData
                                width: parent.width
                                height: sectionData.boxHeight
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
                                            text: sectionData.title
                                            font.pixelSize: 12
                                            font.bold: true
                                            color: "#4D5C70"
                                            font.family: "Microsoft YaHei"
                                        }
                                    }

                                    Repeater {
                                        model: sectionData.rows

                                        delegate: Row {
                                            width: parent.width
                                            height: sectionData.rowHeight

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
        }
    }

    Component.onCompleted: {
        if (recycle_experiment_list_model) {
            recycle_experiment_list_model.reloadFromDb()
        }
    }
}
