import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import "component"

Popup {
    id: root
    z: 99
    width: 557
    height: 366
    modal: true
    // anchors.centerIn: Overlay.overlay
    closePolicy: Popup.NoAutoClose
    anchors.centerIn: parent

    parent: rootRectangle

    property int pixelSize : 16
    property string text_color : "#000000"
    //property string family : notoSansSCRegular.name

    onOpened: {
        popup.open()
    }

    onClosed: {
        popup.close()
    }

    background: Rectangle {
        color: "white"
        radius: 4
        border.color: "transparent"
        border.width: 0
    }

    Popup{
        id:popup
        width: parent.width
        height: parent.height
        modal: false
        // anchors.centerIn: Overlay.overlay
        anchors.centerIn: parent

        closePolicy: Popup.NoAutoClose
        z:root.z + 1

        background: Rectangle {
            color: "white"
            radius: 4
            border.width: 0
        }
        ColumnLayout {
            anchors.centerIn: parent
            spacing: 30
            Layout.margins: 60

            Text {
                Layout.fillWidth: true
                Layout.fillHeight: true
                text: qsTr("新建工程")
                font.pixelSize: 20
                color: "black"
                //font.family: family
                wrapMode: Text.Wrap
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }

            RowLayout{
                Layout.fillWidth: true
                Layout.preferredHeight: 42
                spacing: 10

                Text {
                    Layout.preferredWidth: 78
                    Layout.fillHeight: true
                    text: qsTr("工程名")
                    font.pixelSize: 18
                    color: "black"
                    //font.family: family
                    wrapMode: Text.Wrap
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }

                LineEdit{
                    id: project_name
                    Layout.preferredWidth: 351
                    Layout.preferredHeight: 42
                    font.pixelSize: 18
                    border_color: "#82C1F2"
                    wrapMode: Text.Wrap
                }
            }

            RowLayout{
                Layout.fillWidth: true
                Layout.preferredHeight: 82
                spacing: 10

                Text {
                    Layout.preferredWidth: 78
                    Layout.fillHeight: true
                    text: qsTr("工程备注")
                    font.pixelSize: 18
                    color: "black"
                    //font.family: family
                    wrapMode: Text.Wrap
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }


                LineEdit_wrap {
                    id: project_note
                    Layout.preferredWidth: 351
                    Layout.fillHeight: true

                    multiLine: true       // 开启换行
                    maxLines: 3           // 最多显示5行，超出后内部滚动
                    font.pixelSize: 18
                    border_color: "#82C1F2"
                }
            }

            RowLayout{
                Layout.fillWidth: true
                Layout.preferredHeight: 42
                spacing: 10
                Layout.alignment: Qt.AlignHCenter


                IconButton {
                    button_text: qsTr("取消")
                    Layout.preferredWidth: 120; Layout.preferredHeight: 42
                    button_color: "#EDEEF0"; text_color: "#333333"
                    onClicked: {
                        console.log("取消新建工程")
                        root.close()
                    }
                }

                IconButton {
                    button_text: qsTr("确定")
                    Layout.preferredWidth: 120; Layout.preferredHeight: 42
                    button_color: "#3B87E4"; text_color: "#FFFFFF"
                    onClicked: {
                        console.log("确定新建工程")
                        console.log(project_name.text +" "+ project_note.text)
                        data_ctrl.addProject(project_name.text,project_note.text)
                        close()
                    }
                }
            }
        }
    }
}
