import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12

Column {
    id: root
    width: 820
    height: 445
    spacing: 0

    property var columnWidths: [0.1, 0.3, 0.4, 0.2]
    property var rowlist: [qsTr("选择"), qsTr("工程名称"), qsTr("实验名称"), qsTr("状态")]

    property alias listView: list
    property int currentIndex: -1

    property int pixel_size: 12

    property var experimentListModel: experiment_list_model

    signal rowSelected(int row, string col1, string col2, string col3, string hiddenId)
    signal rowDeselected()
    signal checkBoxToggled(int row, bool isChecked, string hiddenId)

    // --- 表头 ---
    Rectangle {
        id: headerRow
        width: parent.width
        height: 32
        color: "#D0E1F1"
        border.color: "#B0C4DE"
        border.width: 1

        Row {
            anchors.fill: parent
            Repeater {
                model: root.rowlist.length
                delegate: Rectangle {
                    width: list.width * columnWidths[index]
                    height: parent.height
                    color: "#D0E1F1"
                    border.color: "#B0C4DE"
                    border.width: 1

                    Text {
                        anchors.fill: parent
                        verticalAlignment: Text.AlignVCenter
                        horizontalAlignment: Text.AlignHCenter
                        text: root.rowlist[index]
                        font.pointSize: 13
                        color: "#5F85A8"
                        font.bold: true
                        elide: Text.ElideRight
                        clip: true
                    }
                }
            }
        }
    }

    // --- 列表区域 ---
    Rectangle {
        id: listview
        width: parent.width
        height: parent.height - headerRow.height
        color: "white"
        border.color: "#B0C4DE"
        border.width: 1

        MouseArea {
            anchors.fill: parent
            z: 1
            onClicked: {
                if (root.currentIndex !== -1) {
                    root.currentIndex = -1;
                    root.rowDeselected();
                }
            }
        }

        ListView {
            id: list
            anchors.fill: parent
            model: root.experimentListModel
            currentIndex: root.currentIndex
            clip: true
            z: 2
            boundsBehavior: Flickable.StopAtBounds

            delegate: Row {
                id: delegateRow
                width: list.width
                height: 32

                property int delegateIndex: index

                // 调试：打印 model 数据
                Component.onCompleted: {
                    console.log("Row", index, "model:", model)
                    console.log("  - checked:", model.checked)
                    console.log("  - projectName:", model.projectName)
                    console.log("  - expName:", model.expName)
                    console.log("  - status:", model.status)
                    console.log("  - expId:", model.expId)
                }

                // --- 第 1 列：复选框 ---
                Rectangle {
                    width: list.width * columnWidths[0]
                    height: parent.height
                    color: root.currentIndex === delegateRow.delegateIndex ? "#A0C4E4" : "white"
                    border.color: "#B0C4DE"
                    border.width: 1

                    Item {
                        anchors.fill: parent

                        CheckBox {
                            id: checkBox
                            anchors.centerIn: parent

                            // 保持原有的逻辑
                            checked: model ? model.checked : false

                            // 【修改点】使用 scale 缩小，0.7 表示缩小到 70%
                            scale: 0.7

                            // 注意：缩放后，它占据的布局空间可能还是原来的大小。
                            // 如果周围有其他元素受布局影响，可能需要调整 anchors 或使用 Item 包裹限制空间。
                            // 在这里因为你是 anchors.centerIn，所以视觉变小即可。

                            onCheckStateChanged: {
                                if (root.experimentListModel) {
                                    root.experimentListModel.setChecked(delegateRow.delegateIndex, checked);
                                }
                                var expId = model ? model.expId : "";
                                root.checkBoxToggled(delegateRow.delegateIndex, checked, expId);
                            }
                        }
                    }
                }

                // --- 第 2 列：工程名称 ---
                Rectangle {
                    width: list.width * columnWidths[1]
                    height: parent.height
                    color: root.currentIndex === delegateRow.delegateIndex ? "#A0C4E4" : "white"
                    border.color: "#B0C4DE"
                    border.width: 1

                    Text {
                        anchors.centerIn: parent
                        width: parent.width - 4
                        text: model ? model.projectName : ""
                        font.pointSize: pixel_size
                        color: "black"
                        horizontalAlignment: Text.AlignHCenter
                        elide: Text.ElideRight
                        clip: true
                    }

                    MouseArea {
                        anchors.fill: parent
                        hoverEnabled: true
                        onClicked: {
                            if (root.currentIndex !== delegateRow.delegateIndex) {
                                root.currentIndex = delegateRow.delegateIndex;
                                var pName = model ? model.projectName : "";
                                var eName = model ? model.expName : "";
                                var s = model ? model.status : 0;
                                var eId = model ? model.expId : "";
                                console.log("Row Selected:", delegateRow.delegateIndex,
                                            "| Project:", pName,
                                            "| Exp:", eName,
                                            "| ID:", eId);
                                root.rowSelected(delegateRow.delegateIndex, pName, eName, s, eId);
                            }
                        }
                    }
                }

                // --- 第 3 列：实验名称 ---
                Rectangle {
                    width: list.width * columnWidths[2]
                    height: parent.height
                    color: root.currentIndex === delegateRow.delegateIndex ? "#A0C4E4" : "white"
                    border.color: "#B0C4DE"
                    border.width: 1

                    Text {
                        anchors.centerIn: parent
                        width: parent.width - 4
                        text: model ? model.expName : ""
                        font.pointSize: pixel_size
                        color: "black"
                        horizontalAlignment: Text.AlignHCenter
                        elide: Text.ElideRight
                        clip: true
                    }

                    MouseArea {
                        anchors.fill: parent
                        hoverEnabled: true
                        onClicked: {
                            if (root.currentIndex !== delegateRow.delegateIndex) {
                                root.currentIndex = delegateRow.delegateIndex;
                                var pName = model ? model.projectName : "";
                                var eName = model ? model.expName : "";
                                var s = model ? model.status : 0;
                                var eId = model ? model.expId : "";
                                console.log("Row Selected:", delegateRow.delegateIndex,
                                            "| Project:", pName,
                                            "| Exp:", eName,
                                            "| ID:", eId);
                                root.rowSelected(delegateRow.delegateIndex, pName, eName, s, eId);
                            }
                        }
                    }
                }

                // --- 第 4 列：状态 ---
                Rectangle {
                    width: list.width * columnWidths[3]
                    height: parent.height
                    color: root.currentIndex === delegateRow.delegateIndex ? "#A0C4E4" : "white"
                    border.color: "#B0C4DE"
                    border.width: 1

                    Text {
                        anchors.centerIn: parent
                        width: parent.width - 4
                        text: {
                            if (!model) return "";
                            var s = model.status;
                            console.log("status value:", s, "type:", typeof s);
                            return s === 1 ? qsTr("已导入") : qsTr("未导入");
                        }
                        font.pointSize: pixel_size
                        color: {
                            if (!model) return "black";
                            var s = model.status;
                            return s === 1 ? "black" : "red";
                        }
                        horizontalAlignment: Text.AlignHCenter
                        elide: Text.ElideRight
                        clip: true
                    }

                    MouseArea {
                        anchors.fill: parent
                        hoverEnabled: true
                        onClicked: {
                            if (root.currentIndex !== delegateRow.delegateIndex) {
                                root.currentIndex = delegateRow.delegateIndex;
                                var pName = model ? model.projectName : "";
                                var eName = model ? model.expName : "";
                                var s = model ? model.status : 0;
                                var eId = model ? model.expId : "";
                                console.log("Row Selected:", delegateRow.delegateIndex,
                                            "| Project:", pName,
                                            "| Exp:", eName,
                                            "| ID:", eId);
                                root.rowSelected(delegateRow.delegateIndex, pName, eName, s, eId);
                            }
                        }
                    }
                }
            }
        }
    }
}
