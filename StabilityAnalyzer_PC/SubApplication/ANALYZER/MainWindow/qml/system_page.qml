import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import "component"

Item {
    id: root

    objectName: "system_page"

    // 后端信号连接：处理 WIFI 列表扫描和连接反馈
    Connections {
        target: system_ctrl
        onWifiListReady: {
            if (mode === 0 || mode === 1) {
                // 刷新 ComboBox 的下拉列表
                wifiComboBox.model = list;
                
                // 如果有已连接的 WiFi，设置为当前选中项
                if (mode === 0 && system_ctrl.wifiConnected && system_ctrl.wifiConnected.toString().length > 0) {
                    var connectedSsid = system_ctrl.wifiConnected;
                    for (var i = 0; i < list.length; i++) {
                        if (list[i] === connectedSsid) {
                            wifiComboBox.currentIndex = i;
                            console.log("已连接 WiFi:", connectedSsid, "索引:", i);
                            break;
                        }
                    }
                }
            } else if (mode === 2) {
                // 这里的 list[0] 是连接结果的提示文字
                if (typeof info_pop !== "undefined") {
                    info_pop.openDialog(list[0]);
                    login.close();
                }
                
                // 连接成功后刷新状态
                if (system_ctrl.wifiConnected && system_ctrl.wifiConnected.toString().length > 0) {
                    var connectedSsid = system_ctrl.wifiConnected;
                    for (var i = 0; i < wifiComboBox.model.length; i++) {
                        if (wifiComboBox.model[i] === connectedSsid) {
                            wifiComboBox.currentIndex = i;
                            break;
                        }
                    }
                }
            }
        }
        onSend_show_msg: {
            info_pop.openDialog(msg);
        }
    }

    // 页面初始化时自动获取一次 WIFI 列表
    Component.onCompleted: {
        system_ctrl.getWifiNameAsync(0);
    }

    Logding{
        id:login
    }

    LoginPopup {
        id: login_popup
        onConfirmed: {
            login.openDialog(qsTr("正在检查系统更新..."))
        }
    }

    TimeSetting{
        id:time_select_pop
    }

    Connections {
        target: time_select_pop
        onTimeSelected: {
            console.log("选择时间：" + selected_time)
            line_edit.text = selected_time
        }
    }

    Column {
        anchors.centerIn: parent
        spacing: 15

        // 第一行：高度 296
        Row {
            spacing: 15

            // 1. WIFI设置
            SettingBlock {
                title: qsTr("WIFI设置")
                blockWidth: 315; blockHeight: 296

                ColumnLayout {
                    anchors.centerIn: parent
                    anchors.verticalCenterOffset: 20
                    spacing: 15
                    width: parent.width

                    UiComboBox {
                        id: wifiComboBox
                        Layout.preferredWidth: 220;  Layout.preferredHeight: 40; Layout.alignment: Qt.AlignHCenter
                        model: [] // 动态获取
                    }
                    LineEdit {
                        id: wifiPasswordInput
                        Layout.preferredWidth: 220; Layout.preferredHeight: 40; Layout.alignment: Qt.AlignHCenter
                        echoMode: TextField.Password
                        placeholderText: qsTr("密码")
                    }
                    RowLayout {
                        Layout.alignment: Qt.AlignHCenter; spacing: 10
                        IconButton {
                            button_text: qsTr("连接")
                            Layout.preferredWidth: 105; Layout.preferredHeight: 42
                            button_color: "#3B87E4"; text_color: "#FFFFFF"
                            onClicked: {
                                console.log("连接WiFi")
                                if (wifiComboBox.currentText !== "") {
                                    system_ctrl.connectWifi(wifiComboBox.currentText, wifiPasswordInput.text)
                                    login.openDialog(qsTr("正在连接到WiFi网络，请稍候..."));
                                }
                            }
                        }
                        IconButton {
                            button_text: qsTr("断开")
                            Layout.preferredWidth: 105; Layout.preferredHeight: 42
                            button_color: "#EDEEF0"; text_color: "#333333"
                            onClicked: {
                                console.log("断开WiFi")
                                system_ctrl.disconnectWifi()
                            }
                        }
                    }
                    // 刷新按钮
                    IconButton {
                        button_text: qsTr("刷新列表")
                        Layout.preferredWidth: 220; Layout.preferredHeight: 35; Layout.alignment: Qt.AlignHCenter
                        button_color: "#EDEEF0"; text_color: "#3B87E4"
                        onClicked: {
                            console.log("刷新列表")
                            system_ctrl.getWifiNameAsync(1)
                        }
                    }
                }
            }

            // 1. 时间设置
            SettingBlock {
                title: qsTr("时间设置")
                blockWidth: 315; blockHeight: 296
                ColumnLayout {
                    anchors.centerIn: parent; anchors.verticalCenterOffset: 20; spacing: 42

                    Rectangle {
                        Layout.preferredWidth: 220
                        Layout.preferredHeight: 40
                        border.color: "#82C1F2"
                        border.width: 1
                        radius: 4

                        Text {
                            id: line_edit
                            text: system_ctrl.getDateTime()
                            font.pixelSize: 20
                            anchors.centerIn: parent  // 居中
                        }

                        MouseArea {
                            anchors.fill: parent
                            onClicked: {
                                time_select_pop.showTime = true
                                time_select_pop.open()
                            }
                        }
                    }
                    IconButton {
                        button_text: qsTr("确定")
                        Layout.preferredWidth: 220; Layout.preferredHeight: 40
                        button_color: "#3B87E4"; text_color: "white"
                        onClicked: {
                            console.log("设置日期时间："+line_edit.text)
                            if(time_select_pop.text === "") return;
                            system_ctrl.updateDateTime(line_edit.text)
                        }
                    }
                }
            }

            // 3. 关于本机
            SettingBlock {
                title: qsTr("关于本机")
                blockWidth: 315; blockHeight: 296
                ColumnLayout {
                    anchors.centerIn: parent; anchors.verticalCenterOffset: 20; spacing: 42

                    IconButton {
                        button_text: qsTr("说明帮助")
                        Layout.preferredWidth: 245; Layout.preferredHeight: 45
                        button_color: "#EDEEF0"; text_color: "#005BAC"
                        onClicked: {
                            console.log("说明帮助")
                            mainStackView.push("qrc:/qml/Instruction.qml")
                        }
                    }
                    IconButton {
                        button_text: qsTr("注销登录")
                        Layout.preferredWidth: 245; Layout.preferredHeight: 45
                        button_color: "#EDEEF0"; text_color: "#005BAC"
                        onClicked: {
                            console.log("注销登录")

                            custom_pop.show(3)

                            //mainStackView.pop()
                            //mainStackView.pop()
                        }
                    }
                }
            }
        }

        // 第二行：高度 185
        Row {
            spacing: 15

            // 4. 语言设置 (中文 = 0, 英文 = 1)
            SettingBlock {
                title: qsTr("语言设置")
                blockWidth: 315; blockHeight: 185
                RowLayout {
                    anchors.centerIn: parent; anchors.verticalCenterOffset: 20; spacing: 15
                    IconButton {
                        button_text: "中文"
                        Layout.preferredWidth: 120; Layout.preferredHeight: 45
                        button_color:  "#3B87E4"
                        text_color: "#FFFFFF"
                        onClicked: {
                            console.log("切换语言：中文")
                            system_ctrl.switchLanguage("zh_CN");
                        }
                    }
                    IconButton {
                        button_text: "ENGLISH"
                        Layout.preferredWidth: 120; Layout.preferredHeight: 45
                        button_color: "#EDEEF0"
                        text_color: "#333333"
                        onClicked: {
                            console.log("切换语言：英文")
                            system_ctrl.switchLanguage("en_US");
                        }
                    }
                }
            }

            // 5. 亮度调节
            SettingBlock {
                title: qsTr("亮度调节")
                blockWidth: 315; blockHeight: 185
                Slider {
                    anchors.centerIn: parent; anchors.verticalCenterOffset: 20
                    width: 250
                    from: 10; to: 254
                    value: 40
                    onMoved: {
                        console.log("调节亮度")
                        system_ctrl.switchBrightness(value);
                    }
                }
            }

            // 6. 系统升级
            SettingBlock {
                title: qsTr("系统升级")
                blockWidth: 315; blockHeight: 185
                IconButton {
                    button_text: qsTr("升级")
                    anchors.centerIn: parent; anchors.verticalCenterOffset: 20
                    width: 160; height: 45
                    button_color: "#EDEEF0"; text_color: "#005BAC"
                    onClicked: {
                        console.log("点击系统升级")
                        custom_pop.show(0)
                        //system_ctrl.update_system();
                    }
                }
            }
        }
    }
}
