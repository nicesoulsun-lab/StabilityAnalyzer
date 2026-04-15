import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Shapes 1.12
import QtQuick.Layouts 1.12

import "component"

Item {
    id: user_root
    objectName: "data_page"

    property bool hasSelection: false
    property int selectedRow: -1

    // 辅助函数：重置输入状态 [cite: 218, 226, 230]
    function resetInputs() {
        hasSelection = false;
        selectedRow = -1;
        // 如果 ListView 有取消选中的方法，可以在此调用
    }

    Rectangle {
        anchors.fill: parent
        anchors.margins: 10
        color: "transparent"

        // 使用 ColumnLayout 撑起整个页面布局 [cite: 33, 234]
        ColumnLayout {
            anchors.fill: parent
            spacing: 15

            CustomListView_data {
                id: dataListView
                Layout.fillWidth: true
                Layout.fillHeight: true

                onRowSelected: {
                    hasSelection = true;
                    selectedRow = row;

                    console.log(row + " " + col1 + " " + col2 + " " + col3 + " " + hiddenId)
                }
                onRowDeselected: {
                    resetInputs();
                }
            }

            // 删除
            IconButton {
                button_text: qsTr("删   除")
                Layout.preferredWidth: 160
                Layout.preferredHeight: 45
                Layout.alignment: Qt.AlignHCenter
                button_color: "#3B87E4"
                text_color: "#FFFFFF"
                pixelSize: 18

                onClicked: {
                    console.log("删除选中的实验")
                    custom_pop.show(2)
                }
            }
        }
    }
}
