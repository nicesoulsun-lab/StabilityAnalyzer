import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12

ComboBox {
    id: comboBox
    width: 200

    property int pixelSize : 14
    property string text_color : "#000000"
    property string family : "Microsoft YaHei"

    property color hover_color: "#F4F8FF"
    property color border_color: "#EDEEF0"
    property int border_width: 1
    property real popHeight: 0


    // 主显示内容
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

    // 主控件背景（输入框）
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

                    // 自定义单选按钮
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
                                width: 8   // ← 改小了
                                height: 8  // ← 改小了
                                x: 4       // ← 调整居中：(16 - 8) / 2 = 4，但注意外框在 (x=2,y=2)，所以这里相对于 radioOuter 的坐标
                                y: 4
                                radius: 4  // 半径为宽高的一半，保持圆形
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
