import QtQuick 2.9
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3

import "component"

Item {
    id: user_root
    objectName: "data_page"

    property bool hasSelection: false
    property int selectedRow: -1

    function refreshExperimentList() {
        resetInputs()
        if (typeof experiment_list_model !== "undefined" && experiment_list_model) {
            experiment_list_model.reloadFromDb()
        }
    }

    function resetInputs() {
        hasSelection = false
        selectedRow = -1
    }

    Component.onCompleted: refreshExperimentList()

    onVisibleChanged: {
        if (visible) {
            refreshExperimentList()
        }
    }

    Connections {
        target: experiment_ctrl
        onExperimentStarted: refreshExperimentList()
        onExperimentStopped: refreshExperimentList()
    }

    Rectangle {
        anchors.fill: parent
        anchors.margins: 10
        color: "transparent"

        ColumnLayout {
            anchors.fill: parent
            spacing: 15

            CustomListView_data {
                id: dataListView
                Layout.fillWidth: true
                Layout.fillHeight: true

                onRowSelected: {
                    hasSelection = true
                    selectedRow = row
                    console.log(row + " " + col1 + " " + col2 + " " + col3 + " " + hiddenId)
                }

                onRowDeselected: {
                    resetInputs()
                }
            }

            IconButton {
                button_text: qsTr("鍒? 闄?)
                Layout.preferredWidth: 160
                Layout.preferredHeight: 45
                Layout.alignment: Qt.AlignHCenter
                button_color: "#3B87E4"
                text_color: "#FFFFFF"
                pixelSize: 18

                onClicked: {
                    console.log("鍒犻櫎閫変腑鐨勫疄楠?)

                    var checkedIds = experiment_list_model.getCheckedExpIds()
                    if (checkedIds.length === 0) {
                        info_pop.openDialog(qsTr("璇峰嬀閫夊疄楠岃褰?))
                        return
                    }

                    custom_pop.show(2)
                }
            }
        }
    }
}

