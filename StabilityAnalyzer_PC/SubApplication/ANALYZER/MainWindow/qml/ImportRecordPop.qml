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
    property int importDurationMs: 2200
    property var columnWidths: [0.12, 0.28, 0.40, 0.20]
    property var headerTitles: [qsTr("选择"), qsTr("工程名称"), qsTr("实验名称"), qsTr("状态")]

    signal importStarted()
    signal importFinished()

    ListModel {
        id: importListModel
    }

    Timer {
        id: importTimer
        interval: root.importDurationMs
        repeat: false
        onTriggered: {
            for (var i = 0; i < importListModel.count; ++i) {
                var item = importListModel.get(i)
                if (item.selected) {
                    importListModel.setProperty(i, "status", 1)
                    importListModel.setProperty(i, "selected", false)
                }
            }

            root.importing = false
            root.importFinished()

            if (typeof info_pop !== "undefined") {
                info_pop.openDialog(qsTr("导入完成"))
            }
        }
    }

    function resetFakeData() {
        importListModel.clear()
        importListModel.append({ selected: false, projectName: "Project A", expName: "Sample A-01", status: 0 })
        importListModel.append({ selected: false, projectName: "Project B", expName: "Sample B-03", status: 1 })
        importListModel.append({ selected: false, projectName: "Project C", expName: "Sample C-02", status: 0 })
        importListModel.append({ selected: false, projectName: "Project D", expName: "Sample D-05", status: 0 })
        importListModel.append({ selected: false, projectName: "Project E", expName: "Sample E-12", status: 1 })
    }

    function checkedCount() {
        var count = 0
        for (var i = 0; i < importListModel.count; ++i) {
            if (importListModel.get(i).selected)
                count++
        }
        return count
    }

    function startImport() {
        if (root.importing)
            return

        if (checkedCount() === 0) {
            if (typeof info_pop !== "undefined")
                info_pop.openDialog(qsTr("请勾选需要导入的记录"))
            return
        }

        root.importing = true
        root.importStarted()
        //root.close()
        importTimer.restart()
    }

    onOpened: {
        root.importing = false
        importTimer.stop()
        resetFakeData()
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

                        ListView {
                            id: listView
                            anchors.fill: parent
                            model: importListModel
                            clip: true
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
                                        enabled: !root.importing && status !== 1
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
                                        text: status === 1 ? qsTr("已导入") : qsTr("未导入")
                                        font.pixelSize: 12
                                        color: status === 1 ? "#333333" : "#D64545"
                                        font.family: "Microsoft YaHei"
                                        horizontalAlignment: Text.AlignHCenter
                                        elide: Text.ElideRight
                                    }
                                }
                            }
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
                    button_text: qsTr("导入")
                    button_color: root.importing ? "#9EBFE8" : "#3B87E4"
                    text_color: "#FFFFFF"
                    pixelSize: 15
                    enabled: !root.importing
                    onClicked: root.startImport()
                }
            }
        }
    }
}
