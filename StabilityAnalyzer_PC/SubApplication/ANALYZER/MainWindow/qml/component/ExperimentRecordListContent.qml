import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12

Item {
    id: root

    property var experimentListModel: experiment_list_model
    property bool showCheckButton: true
    property bool showCompareButton: true
    property bool showExportButton: true
    property bool showDeleteButton: true
    property bool showRestoreButton: false
    property string deleteButtonText: qsTr("删除")
    property string restoreButtonText: qsTr("还原")

    signal rowSelected(int row)
    signal rowDeselected()
    signal deleteRequested()
    signal compareRequested()
    signal checkRequested()
    signal exportRequested()
    signal restoreRequested()

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 14
        spacing: 14

        ExperimentRecordTable {
            Layout.fillWidth: true
            Layout.fillHeight: true
            experimentListModel: root.experimentListModel

            onRowSelected: root.rowSelected(row)
            onRowDeselected: root.rowDeselected()
        }

        Row {
            Layout.alignment: Qt.AlignHCenter
            spacing: 18

            IconButton {
                visible: root.showCheckButton
                width: 88
                height: 34
                button_text: qsTr("查看")
                button_color: "#4A89DC"
                text_color: "#FFFFFF"
                pixelSize: 13
                onClicked: root.checkRequested()
            }

            IconButton {
                visible: root.showCompareButton
                width: 88
                height: 34
                button_text: qsTr("比较")
                button_color: "#4A89DC"
                text_color: "#FFFFFF"
                pixelSize: 13
                onClicked: root.compareRequested()
            }

            IconButton {
                visible: root.showDeleteButton
                width: 88
                height: 34
                button_text: root.deleteButtonText
                button_color: "#4A89DC"
                text_color: "#FFFFFF"
                pixelSize: 13
                onClicked: root.deleteRequested()
            }

            IconButton {
                visible: root.showExportButton
                width: 88
                height: 34
                button_text: qsTr("导出")
                button_color: "#4A89DC"
                text_color: "#FFFFFF"
                pixelSize: 13
                onClicked: root.exportRequested()
            }

            IconButton {
                visible: root.showRestoreButton
                width: 88
                height: 34
                button_text: root.restoreButtonText
                button_color: "#4A89DC"
                text_color: "#FFFFFF"
                pixelSize: 13
                onClicked: root.restoreRequested()
            }
        }
    }
}
