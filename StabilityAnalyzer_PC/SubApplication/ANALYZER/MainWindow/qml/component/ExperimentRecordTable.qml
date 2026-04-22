import QtQuick 2.12
import QtQuick.Controls 2.12

Column {
    id: root
    width: 640
    height: 400
    spacing: 0

    property alias listView: listView
    property int currentIndex: -1
    property var experimentListModel: experiment_list_model
    property string emptyText: qsTr("暂无实验记录")
    // Keep header labels in one place so the visual columns and data columns stay aligned.
    property var headerLabels: [qsTr("选择"), qsTr("序号"), qsTr("工程名称"), qsTr("实验名称")]
    // Split checkbox and sequence into two explicit columns to make batch selection clearer.
    property int selectionColumnWidth: 58
    property var headerTitles: [
        qsTr("序号"),
        qsTr("工程名称"),
        qsTr("实验名称")
    ]
    property int sequenceColumnWidth: 68
    property int projectColumnWidth: Math.max(180, Math.round((root.width - root.selectionColumnWidth - root.sequenceColumnWidth) * 0.36))
    property int expNameColumnWidth: Math.max(220, root.width - root.selectionColumnWidth - root.sequenceColumnWidth - root.projectColumnWidth)

    signal rowSelected(int row, string col1, string col2, string col3, string hiddenId)
    signal rowDeselected()
    signal checkBoxToggled(int row, bool isChecked, string hiddenId)

    // Read roles defensively because some optional backend fields may be empty.
    function roleValue(rowIndex, roleName, fallbackValue) {
        if (!root.experimentListModel)
            return fallbackValue

        var value = root.experimentListModel.get(rowIndex, roleName)
        if (value === undefined || value === null || value === "")
            return fallbackValue
        return value
    }

    function selectRow(rowIndex) {
        if (!root.experimentListModel)
            return

        root.currentIndex = rowIndex
        root.rowSelected(
                    rowIndex,
                    String(root.roleValue(rowIndex, "projectName", "")),
                    String(root.roleValue(rowIndex, "expName", "")),
                    "",
                    String(root.roleValue(rowIndex, "expId", "")))
    }

    function columnWidth(index) {
        if (index === 0)
            return root.selectionColumnWidth
        if (index === 1)
            return root.sequenceColumnWidth
        if (index === 2)
            return root.projectColumnWidth
        return root.expNameColumnWidth
    }

    function resetSelection() {
        if (root.currentIndex !== -1) {
            root.currentIndex = -1
            root.rowDeselected()
        }
    }

    Rectangle {
        id: headerRow
        width: parent.width
        height: 34
        color: "#F5F7FA"
        border.color: "#DCE3EA"
        border.width: 1

        Row {
            anchors.fill: parent

            Repeater {
                model: root.headerLabels

                delegate: Rectangle {
                    width: root.columnWidth(index)
                    height: parent.height
                    color: "#F5F7FA"
                    border.color: "#DCE3EA"
                    border.width: 1

                    Text {
                        anchors.centerIn: parent
                        text: modelData
                        font.pixelSize: 13
                        font.family: "Microsoft YaHei"
                        color: "#333333"
                    }
                }
            }
        }
    }

    Rectangle {
        width: parent.width
        height: parent.height - headerRow.height
        color: "#FFFFFF"
        border.color: "#DCE3EA"
        border.width: 1

        MouseArea {
            anchors.fill: parent
            propagateComposedEvents: true
            onClicked: {
                root.resetSelection()
                mouse.accepted = false
            }
        }

        ListView {
            id: listView
            anchors.fill: parent
            clip: true
            model: root.experimentListModel
            boundsBehavior: Flickable.StopAtBounds
            spacing: 0

            ScrollBar.vertical: ScrollBar {
                width: 10
            }

            delegate: Rectangle {
                id: rowItem
                width: listView.width
                height: 30
                color: root.currentIndex === index ? "#D8EBFF" : "#FFFFFF"
                border.color: "#E5EAF0"
                border.width: 1

                property var checkedValue: root.roleValue(index, "checked", false)
                property var sequenceValue: root.roleValue(index, "sequence", index + 1)
                property var projectNameValue: root.roleValue(index, "projectName", "--")
                property var expNameValue: root.roleValue(index, "expName", "--")
                property var expIdValue: root.roleValue(index, "expId", "")

                Row {
                    anchors.fill: parent

                    Rectangle {
                        width: root.selectionColumnWidth
                        height: parent.height
                        color: "transparent"
                        border.color: "#E5EAF0"
                        border.width: 1

                        CheckBox {
                            anchors.centerIn: parent
                            scale: 0.72
                            checked: Boolean(rowItem.checkedValue)

                            onClicked: {
                                if (root.experimentListModel) {
                                    root.experimentListModel.setChecked(index, checked)
                                }
                                root.checkBoxToggled(index, checked, String(rowItem.expIdValue))
                            }
                        }
                    }

                    Rectangle {
                        width: root.sequenceColumnWidth
                        height: parent.height
                        color: "transparent"
                        border.color: "#E5EAF0"
                        border.width: 1

                        Text {
                            anchors.centerIn: parent
                            text: rowItem.sequenceValue
                            font.pixelSize: 12
                            font.family: "Microsoft YaHei"
                            color: "#333333"
                        }
                    }

                    Rectangle {
                        width: root.projectColumnWidth
                        height: parent.height
                        color: "transparent"
                        border.color: "#E5EAF0"
                        border.width: 1

                        Text {
                            anchors.centerIn: parent
                            width: parent.width - 12
                            horizontalAlignment: Text.AlignHCenter
                            text: rowItem.projectNameValue
                            font.pixelSize: 12
                            font.family: "Microsoft YaHei"
                            color: "#333333"
                            elide: Text.ElideRight
                        }

                        MouseArea {
                            anchors.fill: parent
                            onClicked: {
                                root.selectRow(index)
                            }
                        }
                    }

                    Rectangle {
                        width: root.expNameColumnWidth
                        height: parent.height
                        color: "transparent"
                        border.color: "#E5EAF0"
                        border.width: 1

                        Text {
                            anchors.centerIn: parent
                            width: parent.width - 12
                            horizontalAlignment: Text.AlignHCenter
                            text: rowItem.expNameValue
                            font.pixelSize: 12
                            font.family: "Microsoft YaHei"
                            color: "#333333"
                            elide: Text.ElideRight
                        }

                        MouseArea {
                            anchors.fill: parent
                            onClicked: {
                                root.selectRow(index)
                            }
                        }
                    }
                }
            }
        }

        Text {
            anchors.centerIn: parent
            visible: listView.count === 0
            text: qsTr("暂无实验记录")
            font.pixelSize: 14
            color: "#97A3B2"
            font.family: "Microsoft YaHei"
        }
    }
}
