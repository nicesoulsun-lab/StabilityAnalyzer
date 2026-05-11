import QtQuick 2.9
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3

Column {
    id: root
    width: 820
    height: 445
    spacing: 0

    property var columnWidths: [0.1, 0.3, 0.4, 0.2]
    property var rowlist: [qsTr("閫夋嫨"), qsTr("宸ョ▼鍚嶇О"), qsTr("瀹為獙鍚嶇О"), qsTr("鐘舵€?)]

    property alias listView: list
    property int currentIndex: -1

    property int pixel_size: 12

    property var experimentListModel: experiment_list_model

    signal rowSelected(int row, string col1, string col2, string col3, string hiddenId)
    signal rowDeselected()
    signal checkBoxToggled(int row, bool isChecked, string hiddenId)

    // --- 琛ㄥご ---
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

    // --- 鍒楄〃鍖哄煙 ---
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

                // 璋冭瘯锛氭墦鍗?model 鏁版嵁
                Component.onCompleted: {
                    console.log("Row", index, "model:", model)
                    console.log("  - checked:", model.checked)
                    console.log("  - projectName:", model.projectName)
                    console.log("  - expName:", model.expName)
                    console.log("  - status:", model.status)
                    console.log("  - expId:", model.expId)
                }

                // --- 绗?1 鍒楋細澶嶉€夋 ---
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

                            // 淇濇寔鍘熸湁鐨勯€昏緫
                            checked: model ? model.checked : false

                            // 銆愪慨鏀圭偣銆戜娇鐢?scale 缂╁皬锛?.7 琛ㄧず缂╁皬鍒?70%
                            scale: 0.7

                            // 娉ㄦ剰锛氱缉鏀惧悗锛屽畠鍗犳嵁鐨勫竷灞€绌洪棿鍙兘杩樻槸鍘熸潵鐨勫ぇ灏忋€?                            // 濡傛灉鍛ㄥ洿鏈夊叾浠栧厓绱犲彈甯冨眬褰卞搷锛屽彲鑳介渶瑕佽皟鏁?anchors 鎴栦娇鐢?Item 鍖呰９闄愬埗绌洪棿銆?                            // 鍦ㄨ繖閲屽洜涓轰綘鏄?anchors.centerIn锛屾墍浠ヨ瑙夊彉灏忓嵆鍙€?
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

                // --- 绗?2 鍒楋細宸ョ▼鍚嶇О ---
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

                // --- 绗?3 鍒楋細瀹為獙鍚嶇О ---
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

                // --- 绗?4 鍒楋細鐘舵€?---
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
                            return s === 1 ? qsTr("宸插鍏?) : qsTr("鏈鍏?);
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

