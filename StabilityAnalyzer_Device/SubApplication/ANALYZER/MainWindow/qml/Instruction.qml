import QtQuick 2.9
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.2


Item {
    id: instruction
    width: 972
    height: 501

    objectName: "Instrction"

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // --- 椤堕儴鏍囬鏍?---
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 30
            Layout.margins: 15

            color: "transparent"

            RowLayout {
                anchors.fill: parent
                spacing: 10

                Label {
                    Layout.leftMargin: 50
                    text: qsTr("鎿嶄綔璇存槑涔?)
                    font.pixelSize: 20
                    font.bold: true
                    color: "#005BAC"
                    verticalAlignment: Text.AlignVCenter
                }
            }

            // 搴曢儴钃濈嚎
            Rectangle {
                anchors.bottom: parent.bottom
                width: parent.width
                height: 2
                color: "#005BAC"
            }
        }

        // --- 鏄剧ず鍖哄煙 ---
        Rectangle {
            id: instructions
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: "#333333" // 娣辩伆鑳屾櫙锛岃‖鎵樼櫧鑹叉枃妗?
            radius: 4

            Column {
                anchors.fill: parent

                // 2. 鏍稿績淇敼鍖哄煙锛氱洿鎺ヤ娇鐢?ListView
                ListView {
                    id: listView
                    width: parent.width
                    height: parent.height - 40 - 10 // 鍑忓幓 Row 楂樺害鍜?spacing
                    clip: true

                    // 鏁版嵁婧愶細28椤?
                    model: 25

                    // 闂磋窛锛氬搴斿師鍏?Column 鐨?spacing
                    spacing: 50

                    // 鎬ц兘浼樺寲鍏抽敭灞炴€?
                    cacheBuffer: 1000       // 棰勫姞杞戒笂涓?1000 鍍忕礌鍖哄煙鐨勫唴瀹癸紝淇濊瘉婊戝姩娴佺晠
                    delegate: Image {
                        width: listView.width
                        height: width * 1.414
                        fillMode: Image.PreserveAspectFit

                        // TODO:鍥剧墖瀹為檯璺緞
                        source: "qrc:/instructions/output_images/page_" + (index + 1) + ".png"

                        // 杩涗竴姝ヨ妭鐪佸唴瀛橈細濡傛灉鍥剧墖澶ぇ锛屽彲浠ラ檺鍒舵簮灏哄
                        sourceSize.width: width

                        asynchronous: true

                    }
                    //褰撳畬鎴愬姞杞藉悗榛樿鍦?0 鐨勪綅缃?
                    Component.onCompleted: {
                        listView.positionViewAtIndex(0, ListView.Beginning)
                    }
                }
            }
        }
    }
}

