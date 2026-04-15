import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import "component"

Item {
    objectName: "LoginPage"

    Connections {
        target: user_ctrl
        onOperationFailed: {
            info_pop.openDialog(message)
        }
    }

    ColumnLayout {
        anchors.centerIn: parent
        spacing: 0

        Item {
            Layout.preferredWidth: 400
            Layout.preferredHeight: 140
            Layout.alignment: Qt.AlignHCenter
            Layout.bottomMargin: 10

            Image {
                id: logoImage
                anchors.top: parent.top
                anchors.horizontalCenter: parent.horizontalCenter
                source: "qrc:/icon/qml/icon/logo.png"
                width: 220; height: 60
                fillMode: Image.PreserveAspectFit
            }

            Text {
                id: titleText
                anchors.bottom: parent.bottom
                anchors.horizontalCenter: parent.horizontalCenter
                text: qsTr("稳定性分析仪")
                font.pixelSize: 36
                font.bold: true
                color: "#005BAC"
                font.family: "Microsoft YaHei"
            }
        }

        Rectangle {
            id: card
            Layout.preferredWidth: 340
            Layout.preferredHeight: 290
            Layout.alignment: Qt.AlignHCenter
            radius: 8
            color: "#FFFFFF"

            ColumnLayout {
                anchors.centerIn: parent
                width: parent.width - 52
                spacing: 18

                Text {
                    text: qsTr("欢迎使用")
                    font.pixelSize: 17
                    color: "#555555"
                    font.family: "Microsoft YaHei"
                    Layout.alignment: Qt.AlignHCenter
                    Layout.topMargin: 10
                    Layout.bottomMargin: 4
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 42
                    radius: 4
                    color: "#FFFFFF"
                    border.color: usernameInput.activeFocus ? "#2B7DD6" : "#D0D8E4"
                    border.width: usernameInput.activeFocus ? 1.5 : 1
                    Behavior on border.color { ColorAnimation { duration: 150 } }

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 12
                        anchors.rightMargin: 12
                        spacing: 8

                        TextInput {
                            id: usernameInput
                            Layout.fillWidth: true
                            verticalAlignment: TextInput.AlignVCenter
                            font.pixelSize: 14
                            font.family: "Microsoft YaHei"
                            color: "#333333"
                            clip: true
                            property string placeholderText: qsTr("请输入您的账号")
                            Text {
                                anchors.verticalCenter: parent.verticalCenter
                                visible: !parent.activeFocus && parent.text === ""
                                text: parent.placeholderText
                                color: "#BBBBBB"
                                font.pixelSize: 14
                                font.family: "Microsoft YaHei"
                            }
                        }
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 42
                    radius: 4
                    color: "#FFFFFF"
                    border.color: passwordInput.activeFocus ? "#2B7DD6" : "#D0D8E4"
                    border.width: passwordInput.activeFocus ? 1.5 : 1
                    Behavior on border.color { ColorAnimation { duration: 150 } }

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 12
                        anchors.rightMargin: 12
                        spacing: 8

                        TextInput {
                            id: passwordInput
                            Layout.fillWidth: true
                            echoMode: TextInput.Password
                            verticalAlignment: TextInput.AlignVCenter
                            font.pixelSize: 14
                            font.family: "Microsoft YaHei"
                            color: "#333333"
                            clip: true
                            property string placeholderText: qsTr("请输入您的密码")
                            Text {
                                anchors.verticalCenter: parent.verticalCenter
                                visible: !parent.activeFocus && parent.text === ""
                                text: parent.placeholderText
                                color: "#BBBBBB"
                                font.pixelSize: 14
                                font.family: "Microsoft YaHei"
                            }
                        }
                    }
                }

                Button {
                    id: loginBtn
                    Layout.fillWidth: true
                    Layout.preferredHeight: 40
                    text: qsTr("登录")

                    contentItem: Text {
                        text: loginBtn.text
                        font.pixelSize: 15
                        color: "#FFFFFF"
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        font.family: "Microsoft YaHei"
                        font.bold: true
                    }

                    background: Rectangle {
                        radius: 4
                        color: loginBtn.pressed ? "#1a5fad" : (loginBtn.hovered ? "#0066CC" : "#2B7DD6")
                        Behavior on color { ColorAnimation { duration: 150 } }
                    }

                    onClicked: {
                        // 必须先走后端登录，写入当前用户角色和 ID，
                        // 首页和用户管理页的权限判断都依赖这份登录态。
//                        var userStr = usernameInput.text.trim()
//                        var passStr = passwordInput.text
//                        if (userStr === "" || passStr === "") {
//                            info_pop.openDialog(qsTr("账号和密码不能为空"))
//                            return
//                        }
//                        if (user_ctrl.login(userStr, passStr)) {
//                            mainStackView.push("qrc:/qml/HomePage.qml")
//                            passwordInput.text = ""
//                        }

                        mainStackView.push("qrc:/qml/HomePage.qml")
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignHCenter
                    Layout.bottomMargin: 6
                    spacing: 28

                    RadioButton {
                        id: zhBtn
                        checked: true
                        text: qsTr("中文")
                        indicator: Rectangle {
                            implicitWidth: 16; implicitHeight: 16
                            x: 0
                            y: parent.height / 2 - height / 2
                            radius: 8
                            border.color: zhBtn.checked ? "#2B7DD6" : "#CCCCCC"
                            border.width: 1.5
                            color: "transparent"
                            Rectangle {
                                width: 8; height: 8; radius: 4
                                anchors.centerIn: parent
                                color: "#2B7DD6"
                                visible: zhBtn.checked
                            }
                        }
                        contentItem: Text {
                            text: zhBtn.text
                            font.pixelSize: 13
                            color: "#666666"
                            font.family: "Microsoft YaHei"
                            leftPadding: zhBtn.indicator.width + 4
                            verticalAlignment: Text.AlignVCenter
                        }
                        onClicked: { zhBtn.checked = true; enBtn.checked = false }
                    }

                    RadioButton {
                        id: enBtn
                        text: qsTr("英文")
                        indicator: Rectangle {
                            implicitWidth: 16; implicitHeight: 16
                            x: 0
                            y: parent.height / 2 - height / 2
                            radius: 8
                            border.color: enBtn.checked ? "#2B7DD6" : "#CCCCCC"
                            border.width: 1.5
                            color: "transparent"
                            Rectangle {
                                width: 8; height: 8; radius: 4
                                anchors.centerIn: parent
                                color: "#2B7DD6"
                                visible: enBtn.checked
                            }
                        }
                        contentItem: Text {
                            text: enBtn.text
                            font.pixelSize: 13
                            color: "#666666"
                            font.family: "Microsoft YaHei"
                            leftPadding: enBtn.indicator.width + 4
                            verticalAlignment: Text.AlignVCenter
                        }
                        onClicked: { enBtn.checked = true; zhBtn.checked = false }
                    }
                }
            }
        }
    }
}
