import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12

import "component"

Popup {
    id: selectionPopup
    width: 557
    height: 366
    anchors.centerIn: Overlay.overlay
    dim: true
    modal: true
    closePolicy: Popup.CloseOnEscape

    background: Rectangle {
        color: "white"
        radius: 5
        border.color: "#e0e0e0"
        border.width: 1
    }

    contentItem: ColumnLayout {
        anchors.fill: parent
        spacing: 50

        // CheckBox 网格：不 fillWidth，改用居中对齐
        GridLayout {
            Layout.topMargin: 80
            rows: 2; columns: 2
            rowSpacing: 50
            columnSpacing: 30
            // 移除 Layout.fillWidth，避免拉伸
            Layout.preferredHeight: 128
            Layout.alignment: Qt.AlignHCenter  // 水平居中


            RowLayout {
                Layout.row: 0; Layout.column: 0
                Layout.preferredHeight: 51
                CheckBox {
                    id: channel_A_check
                    checked: false
                    text: qsTr("A通道")
                    font.pixelSize: 24
                    font.bold: true

                    onCheckStateChanged: {

                    }
                }

                IconButton {
                    button_text: qsTr("设置参数")
                    Layout.preferredWidth: 128; Layout.preferredHeight: 42
                    button_color: "#3B87E4"; text_color: "#FFFFFF"
                    onClicked: {
                        console.log("设置参数：A通道")
                        mainStackView.push(Qt.resolvedUrl("qrc:/qml/ParaSetting_A.qml"))
                        close()
                    }
                }
            }
            RowLayout {
                Layout.row: 0; Layout.column: 1
                Layout.preferredHeight: 51
                CheckBox {
                    id: channel_B_check
                    checked: false
                    text: qsTr("B通道")
                    font.pixelSize: 24
                    font.bold: true

                    onCheckStateChanged: {

                    }
                }
                IconButton {
                    button_text: qsTr("设置参数")
                    Layout.preferredWidth: 128; Layout.preferredHeight: 42
                    button_color: "#3B87E4"; text_color: "#FFFFFF"
                    onClicked: {
                        console.log("设置参数：B通道")
                        mainStackView.push(Qt.resolvedUrl("qrc:/qml/ParaSetting_B.qml"))
                        close()
                    }
                }
            }
            RowLayout {
                Layout.row: 1; Layout.column: 0
                Layout.preferredHeight: 51
                CheckBox {
                    id: channel_C_check
                    checked: false
                    text: qsTr("C通道")
                    font.pixelSize: 24
                    font.bold: true

                    onCheckStateChanged: {

                    }
                }
                IconButton {
                    button_text: qsTr("设置参数")
                    Layout.preferredWidth: 128; Layout.preferredHeight: 42
                    button_color: "#3B87E4"; text_color: "#FFFFFF"
                    onClicked: {
                        console.log("设置参数：C通道")
                        mainStackView.push(Qt.resolvedUrl("qrc:/qml/ParaSetting_C.qml"))
                        close()
                    }
                }
            }
            RowLayout{
                Layout.row: 1; Layout.column: 1
                Layout.preferredHeight: 51
                CheckBox {
                    id: channel_D_check
                    checked: false
                    text: qsTr("D通道")
                    font.pixelSize: 24
                    font.bold: true

                    onCheckStateChanged: {

                    }
                }
                IconButton {
                    button_text: qsTr("设置参数")
                    Layout.preferredWidth: 128; Layout.preferredHeight: 42
                    button_color: "#3B87E4"; text_color: "#FFFFFF"
                    onClicked: {
                        console.log("设置参数：D通道")
                        mainStackView.push(Qt.resolvedUrl("qrc:/qml/ParaSetting_D.qml"))
                        close()
                    }
                }
            }
        }

        // 按钮行：居中
        RowLayout {
            Layout.alignment: Qt.AlignHCenter
            Layout.bottomMargin: 40
            spacing: 64

            IconButton {
                button_text: qsTr("取消")
                Layout.preferredWidth: 120; Layout.preferredHeight: 42
                button_color: "#EDEEF0"; text_color: "#333333"
                onClicked: {
                    console.log("取消开始实验")
                    close()
                }
            }

            IconButton {
                button_text: qsTr("确定")
                Layout.preferredWidth: 120; Layout.preferredHeight: 42
                button_color: "#3B87E4"; text_color: "#FFFFFF"
                enabled: channel_A_check.checked || channel_B_check.checked || channel_C_check.checked || channel_D_check.checked
                onClicked: {
                    console.log("开始实验")
                    var creatorId = user_ctrl && user_ctrl.currentUserId > 0 ? user_ctrl.currentUserId : -1
                    var started = false

                    function startSelectedChannel(channelIndex, channelName) {
                        var ok = experiment_ctrl.startExperiment(channelIndex, creatorId)
                        console.log("启动通道:", channelName, "channel=", channelIndex, "result=", ok, "creatorId=", creatorId)
                        if (ok) {
                            started = true
                        }
                    }

                    if (channel_A_check.checked || channel_B_check.checked || channel_C_check.checked || channel_D_check.checked) {
                        close()
                    }else{
                        info_pop.openDialog(qsTr("请选择通道"))
                    }

                    if (channel_A_check.checked) startSelectedChannel(0, "A")
                    if (channel_B_check.checked) startSelectedChannel(1, "B")
                    if (channel_C_check.checked) startSelectedChannel(2, "C")
                    if (channel_D_check.checked) startSelectedChannel(3, "D")
                }
            }
        }
    }
}
