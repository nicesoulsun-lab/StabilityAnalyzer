import QtQuick 2.9
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3
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
        anchors.centerIn: parent // 绠€鍐欙細璁╂暣涓垪甯冨眬鍦ㄩ〉闈㈠眳涓?        spacing: 24

        Component.onCompleted: {

            // user_name = "admin"
            // user_level = 1
            // mainStackView.push("qrc:/front/page/HomePage.qml")
        }

        Text {
            text: qsTr("娆㈣繋鐧诲綍")
            font.pixelSize: 20
            font.bold: true

            // 銆愪慨鏀广€慙ayout鍐呴儴涓嶈兘鐢╝nchors锛屾敼鐢╝lignment
            Layout.alignment: Qt.AlignHCenter
        }

        LineEdit {
            id: usernameInput

            // 銆愪慨鏀广€慙ayout鍐呴儴浣跨敤 preferredWidth/Height
            Layout.preferredWidth: 287
            Layout.preferredHeight: 40
            Layout.alignment: Qt.AlignHCenter // 纭繚鑷韩鍦ㄥ垪涓眳涓?
            placeholderText: qsTr("璇疯緭鍏ヨ处鍙?)
            horizontalAlignment: TextInput.AlignHCenter
            bg_color: "#FFFFFF"
        }

        LineEdit {
            id: passwordInput

            // 銆愪慨鏀广€慙ayout鍐呴儴浣跨敤 preferredWidth/Height
            Layout.preferredWidth: 287
            Layout.preferredHeight: 40
            Layout.alignment: Qt.AlignHCenter

            echoMode: TextInput.Password
            placeholderText: qsTr("璇疯緭鍏ュ瘑鐮?)
            horizontalAlignment: TextInput.AlignHCenter
            bg_color: "#FFFFFF"
        }

        IconButton {
            id: loginBtn

            // 銆愪慨鏀广€慙ayout鍐呴儴浣跨敤 preferredWidth/Height
            Layout.preferredWidth: 287
            Layout.preferredHeight: 40
            Layout.alignment: Qt.AlignHCenter

            button_text: qsTr("鐧诲綍")
            background_source: "qrc:/icon/qml/icon/login_button.png"
            text_color: "#FFFFFF"
            pixelSize: 16

            onClicked: {
                var userStr = usernameInput.text.trim();
                var passStr = passwordInput.text;

                if (userStr === "" || passStr === "") {
                    info_pop.openDialog(qsTr("璐﹀彿鍜屽瘑鐮佷笉鑳戒负绌?));
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

