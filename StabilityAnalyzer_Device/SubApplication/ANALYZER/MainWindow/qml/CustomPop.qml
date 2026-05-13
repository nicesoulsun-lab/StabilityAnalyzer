import QtQuick 2.12
import QtQuick.Controls 2.12
import QtGraphicalEffects 1.12
import QtQuick.Layouts 1.12

import "component"

Popup {
    id: root
    z:5
    width: 472
    height: 257
    anchors.centerIn: parent
    modal: true        // 关键：阻止背景交互
    closePolicy: Popup.NoAutoClose // 禁止点击外部关闭popup

    property string update_account
    property string update_pwd
    property string wifi_pwd

    property int model: 0

    function show(model){
        root.model = model
        open()
    }

    background: Rectangle {
        color: "transparent"  // 或者写成 color: rgba(0, 0, 0, 0)
    }
    onOpened: {
        subPop.open()
    }
    onClosed: {
        subPop.close()
    }
    Popup {
        id:subPop
        z:15
        width: root.width; height: root.height
        anchors.centerIn: parent
        closePolicy: Popup.NoAutoClose
        background: Rectangle {
            color: "white"
            radius: 4
            border.color: "#e0e0e0"
            border.width: 1
        }

        contentItem: ColumnLayout {
            anchors.fill: parent
            spacing: 20

            Loader {
                id: controlLoader
                Layout.fillWidth: true
                Layout.preferredHeight: 150

                sourceComponent: {
                    if (root.model === 0) return update_Component
                    else if (root.model === 1) return update_latestComponent
                    else if (root.model === 2) return delete_confirm_Component
                    else if (root.model === 3) return log_out_confirm_Component
                    return null
                }
            }

            // 按钮行：居中
            RowLayout {
                Layout.alignment: Qt.AlignHCenter
                Layout.bottomMargin: 30
                spacing: 24

                IconButton {
                    button_text: qsTr("取消")
                    Layout.preferredWidth: 120; Layout.preferredHeight: 42
                    button_color: "#EDEEF0"; text_color: "#333333"
                    onClicked: {
                        if (root.model === 0) console.log("取消升级系统")
                        else if (root.model === 1) return
                        else if (root.model === 2) console.log("取消删除记录")
                        else if (root.model === 3) console.log("取消注销登录")
                        close()
                    }
                }

                IconButton {
                    button_text: qsTr("确认")
                    Layout.preferredWidth: 120; Layout.preferredHeight: 42
                    button_color: "#3B87E4"; text_color: "#FFFFFF"
                    onClicked: {
                        if (root.model === 0) {
                            console.log("确认升级系统")
                            system_ctrl.update_system();
                        }
                        else if (root.model === 1) {

                        }
                        else if (root.model === 2) {
                            console.log("确认删除记录")
                            var checkedIds = experiment_list_model.getCheckedExpIds()
                            console.log("选中的实验ID:", checkedIds)

                            if (checkedIds.length === 0) {
                                console.log("没有选中任何实验")
                                return
                            }

                            var success = data_ctrl.deleteExperiments(checkedIds)
                            if (success) {
                                console.log("删除成功，刷新列表")
                                experiment_list_model.reloadFromDb()
                            }
                        }
                        else if (root.model === 3) {
                            console.log("确认注销登录")
                            mainStackView.pop()
                            mainStackView.pop()
                        }
                        close()
                    }
                }
            }
        }
    }


    Component {
        id: update_Component

        ColumnLayout {
            Text {
                text: qsTr("确认升级系统？");
                font.pixelSize: 18
                font.bold: true
                color: "black"
                horizontalAlignment: Text.AlignHCenter
                Layout.alignment: Qt.AlignHCenter  // 关键：在 Layout 中居中
            }
        }
    }

    Component {
        id: update_latestComponent
        Text {
            text: qsTr("已是最新版本:") + DataStore.currentVersion
            font.pixelSize: 18
            font.bold: true
            color: "black"
            verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignHCenter
        }
    }

    Component {
        id: delete_confirm_Component
        Text {
            text: qsTr("确认删除勾选记录？")
            font.pixelSize: 18
            font.bold: true
            color: "black"
            verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignHCenter
            lineHeight: 1.4
        }
    }

    Component {
        id: log_out_confirm_Component
        Text {
            text: qsTr("确认注销登录？")
            font.pixelSize: 18
            font.bold: true
            color: "black"
            verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignHCenter
            lineHeight: 1.4
        }
    }
}


