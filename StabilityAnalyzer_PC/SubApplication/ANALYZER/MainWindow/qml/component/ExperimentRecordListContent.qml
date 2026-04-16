import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12

Item {
    id: root

    signal rowSelected(int row)
    signal rowDeselected()
    signal deleteRequested()
    signal compareRequested()
    signal checkRequested()
    signal exportRequested()

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 14
        spacing: 14

        // Render the record list with the original table component.
        ExperimentRecordTable {
            Layout.fillWidth: true
            Layout.fillHeight: true

            onRowSelected: {
                root.rowSelected(row)
            }

            onRowDeselected: {
                root.rowDeselected()
            }
        }

        Row {
            Layout.alignment: Qt.AlignHCenter
            spacing: 18

            IconButton {
                width: 88
                height: 34
                button_text: qsTr("查看")
                button_color: "#4A89DC"
                text_color: "#FFFFFF"
                pixelSize: 13
                onClicked: root.checkRequested()
            }

            IconButton {
                width: 88
                height: 34
                button_text: qsTr("比较")
                button_color: "#4A89DC"
                text_color: "#FFFFFF"
                pixelSize: 13
                onClicked: root.compareRequested()
            }

            IconButton {
                width: 88
                height: 34
                button_text: qsTr("删除")
                button_color: "#4A89DC"
                text_color: "#FFFFFF"
                pixelSize: 13
                onClicked: root.deleteRequested()
            }

            IconButton {
                width: 88
                height: 34
                button_text: qsTr("导出")
                button_color: "#4A89DC"
                text_color: "#FFFFFF"
                pixelSize: 13
                onClicked: root.exportRequested()
            }
        }
    }
}
