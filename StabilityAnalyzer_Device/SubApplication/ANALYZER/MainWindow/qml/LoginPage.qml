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
        anchors.centerIn: parent // 简写：让整个列布局在页面居中
        spacing: 24

        Component.onCompleted: {

            // user_name = "admin"
            // user_level = 1
            // mainStackView.push("qrc:/front/page/HomePage.qml")
        }

        Text {
            text: qsTr("欢迎登录")
            font.pixelSize: 20
            font.bold: true

            // 【修改】Layout内部不能用anchors，改用alignment
            Layout.alignment: Qt.AlignHCenter
        }

        LineEdit {
            id: usernameInput

            // 【修改】Layout内部使用 preferredWidth/Height
            Layout.preferredWidth: 287
            Layout.preferredHeight: 40
            Layout.alignment: Qt.AlignHCenter // 确保自身在列中居中

            placeholderText: qsTr("请输入账号")
            horizontalAlignment: TextInput.AlignHCenter
            bg_color: "#FFFFFF"
        }

        LineEdit {
            id: passwordInput

            // 【修改】Layout内部使用 preferredWidth/Height
            Layout.preferredWidth: 287
            Layout.preferredHeight: 40
            Layout.alignment: Qt.AlignHCenter

            echoMode: TextInput.Password
            placeholderText: qsTr("请输入密码")
            horizontalAlignment: TextInput.AlignHCenter
            bg_color: "#FFFFFF"
        }

        IconButton {
            id: loginBtn

            // 【修改】Layout内部使用 preferredWidth/Height
            Layout.preferredWidth: 287
            Layout.preferredHeight: 40
            Layout.alignment: Qt.AlignHCenter

            button_text: qsTr("登录")
            background_source: "qrc:/icon/qml/icon/login_button.png"
            text_color: "#FFFFFF"
            pixelSize: 16

            onClicked: {
                var userStr = usernameInput.text.trim();
                var passStr = passwordInput.text;

                if (userStr === "" || passStr === "") {
                    info_pop.openDialog(qsTr("账号和密码不能为空"));
                    return;
                }

                if (user_ctrl.login(userStr, passStr)) {
                    mainStackView.push("qrc:/qml/HomePage.qml");
                    passwordInput.text = "";
                }
            }
        }
    }
}
