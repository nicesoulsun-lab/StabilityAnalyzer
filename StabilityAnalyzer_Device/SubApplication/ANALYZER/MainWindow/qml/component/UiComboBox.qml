import QtQuick 2.9
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3

ComboBox {
    id: comboBox
    width: 200

    property int pixelSize : 14
    property string text_color : "#000000"
    //property string family : notoSansSCRegular.name

    property color hover_color: "#F4F8FF"
    property color border_color: "#EDEEF0"
    property int border_width: 1
    property real popHeight: 0


    // 涓绘樉绀哄唴瀹?
    contentItem: Item {
        anchors.fill: parent
        Text{
            anchors{
                left: parent.left
                leftMargin: 10
                verticalCenter: parent.verticalCenter
            }

            text: comboBox.displayText
            color: text_color
            font.pixelSize: pixelSize
            font.family: family
            // elide: Text.ElideRight

        }
    }

    // 涓绘帶浠惰儗鏅紙杈撳叆妗嗭級
    background: Rectangle {
        implicitWidth: parent.width
        implicitHeight: parent.height
        color: hover_color
        border.color: border_color
        border.width: border_width
        radius: 4
    }



    popup: Popup {
        width: parent.width
        y: parent.height + 10
        implicitHeight: Math.min(comboBox.model.length * (comboBox.height) * 1.5, 300) || 0

        background: Rectangle {
            color: "white"
            radius: 4
            border.color: "#EDEEF0"
            border.width: 1
        }

        contentItem: ListView {
            clip: true
            implicitHeight: contentHeight
            model: comboBox.visible ? comboBox.model : null
            currentIndex: comboBox.highlightedIndex
            spacing: 8

            ButtonGroup {
                id: radioGroup
            }

            delegate: ItemDelegate {
                width: parent.width
                height: comboBox.height

                contentItem: Row {
                    spacing: 15
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.left: parent.left
                    anchors.leftMargin: 10

                    // 鑷畾涔夊崟閫夋寜閽?
                    Item {
                        width: 20
                        height: 20
                        anchors.verticalCenter: parent.verticalCenter

                        Rectangle {
                            id: radioOuter
                            width: 16
                            height: 16
                            x: 2
                            y: 2
                            radius: 8
                            border.color: index === comboBox.currentIndex ? "#1976D2" : "#CCCCCC"
                            border.width: 2
                            color: "transparent"

                            Rectangle {
                                id: radioInner
                                width: 8   // 鈫?鏀瑰皬浜?
                                height: 8  // 鈫?鏀瑰皬浜?
                                x: 4       // 鈫?璋冩暣灞呬腑锛?16 - 8) / 2 = 4锛屼絾娉ㄦ剰澶栨鍦?(x=2,y=2)锛屾墍浠ヨ繖閲岀浉瀵逛簬 radioOuter 鐨勫潗鏍?
                                y: 4
                                radius: 4  // 鍗婂緞涓哄楂樼殑涓€鍗婏紝淇濇寔鍦嗗舰
                                visible: index === comboBox.currentIndex
                                color: "#1976D2"
                            }
                        }
                    }

                    Text {
                        text: modelData
                        height: 20
                        color: text_color
                        font.pixelSize: pixelSize
                        font.family: family
                        elide: Text.ElideRight
                        verticalAlignment: Text.AlignVCenter
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }

                onClicked: {
                    comboBox.currentIndex = index
                    comboBox.popup.close()
                }
            }

            ScrollIndicator.vertical: ScrollIndicator {}
        }
    }

}

