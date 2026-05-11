import QtQuick 2.9
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3
import "component"

Item {
    id: root

    objectName: "system_page"

    // 鍚庣淇″彿杩炴帴锛氬鐞?WIFI 鍒楄〃鎵弿鍜岃繛鎺ュ弽棣?
    Connections {
        target: system_ctrl
        onWifiListReady: {
            if (mode === 0 || mode === 1) {
                // 鍒锋柊 ComboBox 鐨勪笅鎷夊垪琛?
                wifiComboBox.model = list;
                
                // 濡傛灉鏈夊凡杩炴帴鐨?WiFi锛岃缃负褰撳墠閫変腑椤?
                if (mode === 0 && system_ctrl.wifiConnected && system_ctrl.wifiConnected.toString().length > 0) {
                    var connectedSsid = system_ctrl.wifiConnected;
                    for (var i = 0; i < list.length; i++) {
                        if (list[i] === connectedSsid) {
                            wifiComboBox.currentIndex = i;
                            console.log("宸茶繛鎺?WiFi:", connectedSsid, "绱㈠紩:", i);
                            break;
                        }
                    }
                }
            } else if (mode === 2) {
                // 杩欓噷鐨?list[0] 鏄繛鎺ョ粨鏋滅殑鎻愮ず鏂囧瓧
                if (typeof info_pop !== "undefined") {
                    info_pop.openDialog(list[0]);
                    login.close();
                }
                
                // 杩炴帴鎴愬姛鍚庡埛鏂扮姸鎬?
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

    // 椤甸潰鍒濆鍖栨椂鑷姩鑾峰彇涓€娆?WIFI 鍒楄〃
    Component.onCompleted: {
        system_ctrl.getWifiNameAsync(0);
    }

    Logding{
        id:login
    }

    LoginPopup {
        id: login_popup
        onConfirmed: {
            login.openDialog(qsTr("姝ｅ湪妫€鏌ョ郴缁熸洿鏂?.."))
        }
    }

    TimeSetting{
        id:time_select_pop
    }

    Connections {
        target: time_select_pop
        onTimeSelected: {
            console.log("閫夋嫨鏃堕棿锛? + selected_time)
            line_edit.text = selected_time
        }
    }

    Column {
        anchors.centerIn: parent
        spacing: 15

        // 绗竴琛岋細楂樺害 296
        Row {
            spacing: 15

            // 1. WIFI璁剧疆
            SettingBlock {
                title: qsTr("WIFI璁剧疆")
                blockWidth: 315; blockHeight: 296

                ColumnLayout {
                    anchors.centerIn: parent
                    anchors.verticalCenterOffset: 20
                    spacing: 15
                    width: parent.width

                    UiComboBox {
                        id: wifiComboBox
                        Layout.preferredWidth: 220;  Layout.preferredHeight: 40; Layout.alignment: Qt.AlignHCenter
                        model: [] // 鍔ㄦ€佽幏鍙?
                    }
                    LineEdit {
                        id: wifiPasswordInput
                        Layout.preferredWidth: 220; Layout.preferredHeight: 40; Layout.alignment: Qt.AlignHCenter
                        echoMode: TextField.Password
                        placeholderText: qsTr("瀵嗙爜")
                    }
                    RowLayout {
                        Layout.alignment: Qt.AlignHCenter; spacing: 10
                        IconButton {
                            button_text: qsTr("杩炴帴")
                            Layout.preferredWidth: 105; Layout.preferredHeight: 42
                            button_color: "#3B87E4"; text_color: "#FFFFFF"
                            onClicked: {
                                console.log("杩炴帴WiFi")
                                if (wifiComboBox.currentText !== "") {
                                    system_ctrl.connectWifi(wifiComboBox.currentText, wifiPasswordInput.text)
                                    login.openDialog(qsTr("姝ｅ湪杩炴帴鍒癢iFi缃戠粶锛岃绋嶅€?.."));
                                }
                            }
                        }
                        IconButton {
                            button_text: qsTr("鏂紑")
                            Layout.preferredWidth: 105; Layout.preferredHeight: 42
                            button_color: "#EDEEF0"; text_color: "#333333"
                            onClicked: {
                                console.log("鏂紑WiFi")
                                system_ctrl.disconnectWifi()
                            }
                        }
                    }
                    // 鍒锋柊鎸夐挳
                    IconButton {
                        button_text: qsTr("鍒锋柊鍒楄〃")
                        Layout.preferredWidth: 220; Layout.preferredHeight: 35; Layout.alignment: Qt.AlignHCenter
                        button_color: "#EDEEF0"; text_color: "#3B87E4"
                        onClicked: {
                            console.log("鍒锋柊鍒楄〃")
                            system_ctrl.getWifiNameAsync(1)
                        }
                    }
                }
            }

            // 1. 鏃堕棿璁剧疆
            SettingBlock {
                title: qsTr("鏃堕棿璁剧疆")
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
                            anchors.centerIn: parent  // 灞呬腑
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
                        button_text: qsTr("纭畾")
                        Layout.preferredWidth: 220; Layout.preferredHeight: 40
                        button_color: "#3B87E4"; text_color: "white"
                        onClicked: {
                            console.log("璁剧疆鏃ユ湡鏃堕棿锛?+line_edit.text)
                            if(time_select_pop.text === "") return;
                            system_ctrl.updateDateTime(line_edit.text)
                        }
                    }
                }
            }

            // 3. 鍏充簬鏈満
            SettingBlock {
                title: qsTr("鍏充簬鏈満")
                blockWidth: 315; blockHeight: 296
                ColumnLayout {
                    anchors.centerIn: parent; anchors.verticalCenterOffset: 20; spacing: 21

                    IconButton {
                        button_text: qsTr("璇存槑甯姪")
                        Layout.preferredWidth: 245; Layout.preferredHeight: 45
                        button_color: "#EDEEF0"; text_color: "#005BAC"
                        onClicked: {
                            console.log("璇存槑甯姪")
                            mainStackView.push("qrc:/qml/Instruction.qml")
                        }
                    }
                    IconButton {
                        button_text: qsTr("鏌ョ湅搴忓垪鍙?)
                        Layout.preferredWidth: 245; Layout.preferredHeight: 45
                        button_color: "#EDEEF0"; text_color: "#005BAC"
                        onClicked: {
                            console.log("鏌ョ湅搴忓垪鍙?)
                            var num = system_ctrl.getSerialNumber();
                            console.log(num)
                            info_pop.openDialog(num)
                        }
                    }
                    IconButton {
                        button_text: qsTr("娉ㄩ攢鐧诲綍")
                        Layout.preferredWidth: 245; Layout.preferredHeight: 45
                        button_color: "#EDEEF0"; text_color: "#005BAC"
                        onClicked: {
                            console.log("娉ㄩ攢鐧诲綍")

                            custom_pop.show(3)

                            //mainStackView.pop()
                            //mainStackView.pop()
                        }
                    }
                }
            }
        }

        // 绗簩琛岋細楂樺害 185
        Row {
            spacing: 15

            // 4. 璇█璁剧疆 (涓枃 = 0, 鑻辨枃 = 1)
            SettingBlock {
                title: qsTr("璇█璁剧疆")
                blockWidth: 315; blockHeight: 185
                RowLayout {
                    anchors.centerIn: parent; anchors.verticalCenterOffset: 20; spacing: 15
                    IconButton {
                        button_text: "涓枃"
                        Layout.preferredWidth: 120; Layout.preferredHeight: 45
                        button_color:  "#3B87E4"
                        text_color: "#FFFFFF"
                        onClicked: {
                            console.log("鍒囨崲璇█锛氫腑鏂?)
                            system_ctrl.switchLanguage("zh_CN");
                        }
                    }
                    IconButton {
                        button_text: "ENGLISH"
                        Layout.preferredWidth: 120; Layout.preferredHeight: 45
                        button_color: "#EDEEF0"
                        text_color: "#333333"
                        onClicked: {
                            console.log("鍒囨崲璇█锛氳嫳鏂?)
                            system_ctrl.switchLanguage("en_US");
                        }
                    }
                }
            }

            // 5. 浜害璋冭妭
            SettingBlock {
                title: qsTr("浜害璋冭妭")
                blockWidth: 315; blockHeight: 185
                Slider {
                    anchors.centerIn: parent; anchors.verticalCenterOffset: 20
                    width: 250
                    from: 10; to: 254
                    value: 40
                    onMoved: {
                        console.log("璋冭妭浜害")
                        system_ctrl.switchBrightness(value);
                    }
                }
            }

            // 6. 绯荤粺鍗囩骇
            SettingBlock {
                title: qsTr("绯荤粺鍗囩骇")
                blockWidth: 315; blockHeight: 185
                IconButton {
                    button_text: qsTr("鍗囩骇")
                    anchors.centerIn: parent; anchors.verticalCenterOffset: 20
                    width: 160; height: 45
                    button_color: "#EDEEF0"; text_color: "#005BAC"
                    onClicked: {
                        console.log("鐐瑰嚮绯荤粺鍗囩骇")
                        custom_pop.show(0)
                        //system_ctrl.update_system();
                    }
                }
            }
        }
    }
}

