import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import "component"

Popup {
    id: root
    z: 99
    width: 560
    height: 360
    modal: true
    focus: true
    anchors.centerIn: Overlay.overlay
    closePolicy: Popup.NoAutoClose
    padding: 0

    function resetForm() {
        project_name.text = ""
        project_note.text = ""
    }

    onOpened: resetForm()

    background: Rectangle {
        color: "#FFFFFF"
        radius: 6
        border.color: "#DCE3EC"
        border.width: 1
    }

    contentItem: ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 48
            color: "#F3F5F7"
            radius: 6

            Rectangle {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                height: 6
                color: "#F3F5F7"
            }

            Text {
                anchors.left: parent.left
                anchors.leftMargin: 18
                anchors.verticalCenter: parent.verticalCenter
                text: qsTr("新建工程")
                font.pixelSize: 18
                color: "#333333"
                font.family: "Microsoft YaHei"
                font.bold: true
            }

            Button {
                id: closeButton
                anchors.right: parent.right
                anchors.rightMargin: 10
                anchors.verticalCenter: parent.verticalCenter
                width: 28
                height: 28
                onClicked: root.close()

                contentItem: Text {
                    text: "×"
                    font.pixelSize: 22
                    color: closeButton.down ? "#2B2B2B" : "#5D6775"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }

                background: Rectangle {
                    radius: 14
                    color: closeButton.hovered ? "#E8EDF4" : "transparent"
                }
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.leftMargin: 28
            Layout.rightMargin: 28
            Layout.topMargin: 28
            Layout.bottomMargin: 24
            spacing: 22

            GridLayout {
                Layout.fillWidth: true
                columns: 2
                columnSpacing: 16
                rowSpacing: 18

                Text {
                    Layout.alignment: Qt.AlignTop
                    Layout.preferredWidth: 84
                    text: qsTr("工程名")
                    font.pixelSize: 16
                    color: "#333333"
                    font.family: "Microsoft YaHei"
                    verticalAlignment: Text.AlignVCenter
                }

                LineEdit {
                    id: project_name
                    Layout.preferredHeight: 42
                    Layout.preferredWidth: 380
                    font.pixelSize: 16
                    border_color: "#82C1F2"
                    placeholderText: qsTr("请输入工程名称")
                }

                Text {
                    Layout.alignment: Qt.AlignTop
                    Layout.preferredWidth: 84
                    text: qsTr("工程备注")
                    font.pixelSize: 16
                    color: "#333333"
                    font.family: "Microsoft YaHei"
                    verticalAlignment: Text.AlignTop
                    topPadding: 10
                }

                LineEdit_wrap {
                    id: project_note
                    multiLine: true
                    Layout.preferredWidth: 380
                    maxLines: 5
                    font.pixelSize: 16
                    border_color: "#82C1F2"
                }
            }

            Item { Layout.fillHeight: true }

            IconButton {
                button_text: qsTr("确定")
                Layout.preferredWidth: 120
                Layout.preferredHeight: 42
                Layout.alignment: Qt.AlignHCenter
                button_color: "#3B87E4"
                text_color: "#FFFFFF"
                onClicked: {
                    var projectName = project_name.text.trim()
                    if (projectName === "") {
                        info_pop.openDialog(qsTr("请输入工程名称"))
                        return
                    }

                    data_ctrl.addProject(projectName, project_note.text.trim())
                    root.close()
                }
            }
        }
    }
}
