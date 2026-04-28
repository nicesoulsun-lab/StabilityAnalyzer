import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12

import "component"

Popup {
    id: root
    width: 860
    height: 560
    modal: true
    dim: true
    focus: true
    padding: 0
    anchors.centerIn: Overlay.overlay
    closePolicy: Popup.CloseOnEscape

    property bool importing: false
    property bool loading: false
    property var pendingImportIds: []
    property var pendingImportResult: ({})
    property var columnWidths: [0.12, 0.24, 0.36, 0.16, 0.12]
    property var headerTitles: [qsTr("选择"), qsTr("工程名称"), qsTr("实验名称"), qsTr("创建时间"), qsTr("状态")]

    signal importStarted()
    signal importFinished()

    ListModel {
        id: importListModel
    }

    function handleDeviceImportFinished(result) {
        console.log("[Import][ImportRecordPop] handleDeviceImportFinished",
                    "importing=", root.importing,
                    "success=", result && result.success,
                    "importedCount=", result && result.importedCount,
                    "failedCount=", result && result.failedCount)
        if (!root.importing) {
            return
        }

        root.pendingImportResult = result || ({})
        root.importing = false
        root.close()
        root.importFinished()
        finalizeImportTimer.restart()
    }

    Timer {
        id: importStateWatchTimer
        interval: 200
        repeat: true
        running: root.importing
        onTriggered: {
            if (!root.importing || !data_ctrl || data_ctrl.deviceImportRunning) {
                return
            }
            console.log("[Import][ImportRecordPop] finalize by timer",
                        "visible=", root.visible,
                        "importing=", root.importing)
            root.handleDeviceImportFinished(data_ctrl.lastDeviceImportResult || {})
        }
    }

    Timer {
        id: finalizeImportTimer
        interval: 50
        repeat: false
        onTriggered: {
            var result = root.pendingImportResult || {}

            // 先关闭进度弹框，再做刷新和提示，避免导入快结束时被收尾操作卡住。
            if (result && result.success) {
                if (typeof experiment_list_model !== "undefined") {
                    experiment_list_model.reloadFromDb()
                }
                root.loadDeviceExperiments()
                if (typeof info_pop !== "undefined") {
                    info_pop.openDialog(result.message ? result.message : qsTr("导入完成"))
                }
            } else {
                if (typeof info_pop !== "undefined") {
                    info_pop.openDialog(result && result.message ? result.message : qsTr("导入失败"))
                }
            }
        }
    }

    function resetModel() {
        importListModel.clear()
    }

    function statusText(status) {
        return Number(status) === 1 ? qsTr("已导入") : qsTr("未导入")
    }

    function appendExperimentRow(row) {
        importListModel.append({
            selected: false,
            deviceExperimentId: Number(row.id || 0),
            projectName: row.project_name || "",
            expName: row.sample_name || "",
            createdAt: row.created_at || "",
            status: Number(row.status || 0)
        })
    }

    function loadDeviceExperiments() {
        if (root.importing) {
            return
        }

        root.loading = true
        resetModel()

        var rows = data_ctrl.fetchDeviceImportExperiments()
        for (var i = 0; i < rows.length; ++i) {
            appendExperimentRow(rows[i])
        }

        root.loading = false

        if (rows.length === 0 && typeof info_pop !== "undefined" && data_transmit_ctrl.deviceUiConnectionStateText !== "Connected") {
            info_pop.openDialog(qsTr("设备未连接，请检查连接状态"))
        }
    }

    function selectedExperimentIds() {
        var ids = []
        for (var i = 0; i < importListModel.count; ++i) {
            var item = importListModel.get(i)
            if (item.selected && Number(item.status) !== 1) {
                ids.push(Number(item.deviceExperimentId))
            }
        }
        return ids
    }

    function startImport() {
        if (root.importing || root.loading) {
            return
        }

        var ids = selectedExperimentIds()
        if (ids.length === 0) {
            if (typeof info_pop !== "undefined") {
                info_pop.openDialog(qsTr("请勾选需要导入的记录"))
            }
            return
        }

        root.pendingImportIds = ids
        root.pendingImportResult = ({})
        root.importing = true
        root.importStarted()
        if (!data_ctrl.startImportExperimentsFromDevice(ids)) {
            root.importing = false
            root.importFinished()
        }
    }

    onOpened: {
        root.importing = false
        root.pendingImportIds = []
        root.pendingImportResult = ({})
        finalizeImportTimer.stop()
        loadDeviceExperiments()
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
            height: 38
            color: "#F3F5F7"

            Text {
                anchors.left: parent.left
                anchors.leftMargin: 18
                anchors.verticalCenter: parent.verticalCenter
                text: qsTr("导入记录")
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
                enabled: !root.importing
                onClicked: root.close()

                contentItem: Text {
                    text: "×"
                    font.pixelSize: 22
                    color: closeButton.down ? "#2B2B2B" : "#5D6775"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }

                background: Rectangle {
                    radius: 14
                    color: closeButton.hovered ? "#E8EDF4" : "transparent"
                }
            }
        }

        ColumnLayout {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.topMargin: 58
            anchors.bottomMargin: 22
            anchors.leftMargin: 26
            anchors.rightMargin: 26
            spacing: 14

            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: "#FFFFFF"
                border.color: "#B0C4DE"
                border.width: 1

                Column {
                    anchors.fill: parent
                    spacing: 0

                    Rectangle {
                        width: parent.width
                        height: 34
                        color: "#D0E1F1"
                        border.color: "#B0C4DE"
                        border.width: 1

                        Row {
                            anchors.fill: parent

                            Repeater {
                                model: root.headerTitles.length

                                delegate: Rectangle {
                                    width: parent.width * root.columnWidths[index]
                                    height: parent.height
                                    color: "#D0E1F1"
                                    border.color: "#B0C4DE"
                                    border.width: 1

                                    Text {
                                        anchors.fill: parent
                                        verticalAlignment: Text.AlignVCenter
                                        horizontalAlignment: Text.AlignHCenter
                                        text: root.headerTitles[index]
                                        font.pixelSize: 13
                                        font.bold: true
                                        color: "#5F85A8"
                                        font.family: "Microsoft YaHei"
                                    }
                                }
                            }
                        }
                    }

                    Rectangle {
                        width: parent.width
                        height: parent.height - 34
                        color: "#FFFFFF"

                        Loader {
                            anchors.centerIn: parent
                            active: root.loading
                            sourceComponent: Column {
                                spacing: 12

                                BusyIndicator {
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    running: true
                                }

                                Text {
                                    text: qsTr("正在读取设备实验列表...")
                                    font.pixelSize: 14
                                    color: "#5D6775"
                                    font.family: "Microsoft YaHei"
                                }
                            }
                        }

                        ListView {
                            id: listView
                            anchors.fill: parent
                            model: importListModel
                            clip: true
                            visible: !root.loading
                            boundsBehavior: Flickable.StopAtBounds

                            delegate: Row {
                                width: listView.width
                                height: 36

                                Rectangle {
                                    width: listView.width * root.columnWidths[0]
                                    height: parent.height
                                    color: "#FFFFFF"
                                    border.color: "#B0C4DE"
                                    border.width: 1

                                    CheckBox {
                                        anchors.centerIn: parent
                                        scale: 0.65
                                        enabled: !root.importing && Number(status) !== 1
                                        checked: selected
                                        onToggled: importListModel.setProperty(index, "selected", checked)
                                    }
                                }

                                Rectangle {
                                    width: listView.width * root.columnWidths[1]
                                    height: parent.height
                                    color: "#FFFFFF"
                                    border.color: "#B0C4DE"
                                    border.width: 1

                                    Text {
                                        anchors.centerIn: parent
                                        width: parent.width - 8
                                        text: projectName
                                        font.pixelSize: 12
                                        color: "#333333"
                                        font.family: "Microsoft YaHei"
                                        horizontalAlignment: Text.AlignHCenter
                                        elide: Text.ElideRight
                                    }
                                }

                                Rectangle {
                                    width: listView.width * root.columnWidths[2]
                                    height: parent.height
                                    color: "#FFFFFF"
                                    border.color: "#B0C4DE"
                                    border.width: 1

                                    Text {
                                        anchors.centerIn: parent
                                        width: parent.width - 8
                                        text: expName
                                        font.pixelSize: 12
                                        color: "#333333"
                                        font.family: "Microsoft YaHei"
                                        horizontalAlignment: Text.AlignHCenter
                                        elide: Text.ElideRight
                                    }
                                }

                                Rectangle {
                                    width: listView.width * root.columnWidths[3]
                                    height: parent.height
                                    color: "#FFFFFF"
                                    border.color: "#B0C4DE"
                                    border.width: 1

                                    Text {
                                        anchors.centerIn: parent
                                        width: parent.width - 8
                                        text: createdAt
                                        font.pixelSize: 12
                                        color: "#333333"
                                        font.family: "Microsoft YaHei"
                                        horizontalAlignment: Text.AlignHCenter
                                        elide: Text.ElideRight
                                    }
                                }

                                Rectangle {
                                    width: listView.width * root.columnWidths[4]
                                    height: parent.height
                                    color: "#FFFFFF"
                                    border.color: "#B0C4DE"
                                    border.width: 1

                                    Text {
                                        anchors.centerIn: parent
                                        width: parent.width - 8
                                        text: root.statusText(status)
                                        font.pixelSize: 12
                                        color: Number(status) === 1 ? "#333333" : "#D64545"
                                        font.family: "Microsoft YaHei"
                                        horizontalAlignment: Text.AlignHCenter
                                        elide: Text.ElideRight
                                    }
                                }
                            }
                        }

                        Text {
                            anchors.centerIn: parent
                            visible: !root.loading && importListModel.count === 0
                            text: qsTr("设备端暂无可显示的实验记录")
                            font.pixelSize: 14
                            color: "#7A8797"
                            font.family: "Microsoft YaHei"
                        }
                    }
                }
            }

            Row {
                Layout.alignment: Qt.AlignHCenter
                spacing: 14

                IconButton {
                    width: 108
                    height: 38
                    button_text: qsTr("刷新")
                    button_color: root.loading ? "#B8C7D9" : "#7B9CC6"
                    text_color: "#FFFFFF"
                    pixelSize: 15
                    enabled: !root.importing
                    onClicked: root.loadDeviceExperiments()
                }

                IconButton {
                    width: 108
                    height: 38
                    button_text: qsTr("导入")
                    button_color: root.importing ? "#9EBFE8" : "#3B87E4"
                    text_color: "#FFFFFF"
                    pixelSize: 15
                    enabled: !root.importing && !root.loading
                    onClicked: root.startImport()
                }
            }
        }
    }
}
