import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12

Rectangle{
    id: optionsBar
    property int _activeMenu: 0
    property bool _langSubOpen: false
    property int _lastClosedMenu: 0
    property double _lastClosedTs: 0
    property string activePage: "record"
    property bool deviceAvailable: true
    // 由首页接住这个信号，统一决定如何打开仪器检查界面。
    signal instrumentCheckRequested()
    signal importRecordRequested()
    signal newExperimentRequested()
    signal experimentRecordRequested()
    signal userManagementRequested()
    signal instructionRequested()
    signal recycleBinRequested()
    color: "transparent"

    function justClosed(menuId) {
        return _lastClosedMenu === menuId && (Date.now() - _lastClosedTs) < 250
    }

    function closeMenus() {
        experimentPopup.close()
        instrumentPopup.close()
        userPopup.close()
        helpPopup.close()
        _activeMenu = 0
        _langSubOpen = false
    }

    function showDeviceUnavailableTip() {
        // 实验和仪器操作依赖设备在线；离线时只提示，不进入业务页面。
        if (typeof info_pop !== "undefined") {
            info_pop.openDialog(qsTr("设备已断开，请检查连接状态后再使用该功能。"))
        }
    }

    onDeviceAvailableChanged: {
        if (!deviceAvailable) {
            // 设备断开后主动收起菜单，避免用户继续点击已不可用的二级入口。
            closeMenus()
        }
    }

    Image{
        anchors.fill: parent
        height: optionsBar.height
        source: "qrc:/icon/qml/icon/option_bar_bg.png"
    }

    RowLayout {
        //Layout.fillWidth: true
        //Layout.preferredHeight: 36
        anchors.fill: parent
        spacing: 100

        // ---- 实验 ----
        Rectangle {
            Layout.leftMargin: 50
            Layout.preferredWidth: txtExp.implicitWidth + 32
            Layout.preferredHeight: 30
            //color: maExp.containsMouse || _activeMenu === 1 ? "#C0D8F0" : "transparent"
            Behavior on color { ColorAnimation { duration: 120 } }
            color: "transparent"
            z: 100

            Row {
                anchors.centerIn: parent
                spacing: 4
                Text {
                    id: txtExp
                    text: qsTr("实验")
                    font.pixelSize: 14
                    color: !optionsBar.deviceAvailable ? "#A8A8A8" : (_activeMenu === 1 ? "#336FFF" : "#444444")
                    font.family: "Microsoft YaHei"
                    anchors.verticalCenter: parent.verticalCenter
                }
                Text {
                    text: _activeMenu === 1 ? "\u2227" : "\u2228"
                    font.pixelSize: 10
                    color: optionsBar.deviceAvailable ? "#999999" : "#C0C0C0"
                    anchors.verticalCenter: parent.verticalCenter
                }
            }
            MouseArea {
                id: maExp
                anchors.fill: parent
                hoverEnabled: true
                onClicked: {
                    if (!optionsBar.deviceAvailable) {
                        optionsBar.showDeviceUnavailableTip()
                        return
                    }
                    if (justClosed(1)) { return }
                    if (_activeMenu === 1 && experimentPopup.visible) {
                        closeMenus()
                        return
                    }
                    closeMenus()
                    _activeMenu = 1
                    experimentPopup.open()
                }
            }

            Popup {
                id: experimentPopup
                y: 36
                x: 0
                width: 150
                height: colExp.height + 16
                padding: 0
                closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
                onClosed: {
                    _lastClosedMenu = 1
                    _lastClosedTs = Date.now()
                    if (_activeMenu === 1) _activeMenu = 0
                }

                background: Rectangle {
                    color: "#FFFFFF"
                    border.color: "#D6E4F0"
                    border.width: 1
                    radius: 6

                    Rectangle {
                        width: 3
                        anchors.top: parent.top
                        anchors.bottom: parent.bottom
                        anchors.left: parent.left
                        anchors.topMargin: 8
                        anchors.bottomMargin: 8
                        anchors.leftMargin: 6
                        color: "#3B8CFF"
                        radius: 2
                    }
                }

                contentItem: Item {
                    Column {
                        id: colExp
                        x: 16; y: 8
                        width: 150 - 22

                        Rectangle {
                            width: parent.width; height: 36
                            color: maE1.containsMouse ? "#EBF3FF" : "transparent"
                            radius: 4
                            Text { text: qsTr("导入记录"); font.pixelSize: 14; color: "#333333"; font.family: "Microsoft YaHei"; anchors.verticalCenter: parent.verticalCenter; x: 10 }
                            MouseArea { id: maE1; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor;
                                onClicked: {
                                    optionsBar.importRecordRequested()
                                    closeMenus()
                                }
                            }
                        }
                        Rectangle {
                            width: parent.width; height: 36
                            color: maE2.containsMouse ? "#EBF3FF" : "transparent"
                            radius: 4
                            Text { text: qsTr("新建工程"); font.pixelSize: 14; color: "#333333"; font.family: "Microsoft YaHei"; anchors.verticalCenter: parent.verticalCenter; x: 10 }
                            MouseArea { id: maE2; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor;
                                onClicked: {
                                    console.log("新建工程");
                                    new_project_pop.open()
                                    closeMenus()
                                }
                            }
                        }
                        Rectangle {
                            width: parent.width; height: 36
                            color: maE3.containsMouse ? "#EBF3FF" : "transparent"
                            radius: 4
                            Text { text: qsTr("新建实验"); font.pixelSize: 14; color: "#333333"; font.family: "Microsoft YaHei"; anchors.verticalCenter: parent.verticalCenter; x: 10 }
                            MouseArea { id: maE3; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor;
                                onClicked: {
                                    optionsBar.newExperimentRequested();
                                    closeMenus()
                                }
                            }
                        }
                    }
                }
            }
        }

        // 实验记录
        Rectangle {
            Layout.preferredWidth: txtExp.implicitWidth + 32
            Layout.preferredHeight: 36
            //color: maRecord.containsMouse || _activeMenu === 2 ? "#C0D8F0" : "transparent"
            Behavior on color { ColorAnimation { duration: 120 } }
            color: "transparent"
            z: 100

            Row {
                anchors.centerIn: parent
                spacing: 4
                Text {
                    id: txtRecord
                    text: qsTr("实验记录")
                    font.pixelSize: 14
                    color: _activeMenu === 2 || optionsBar.activePage === "record" ? "#336FFF" : "#444444"
                    font.family: "Microsoft YaHei"
                    anchors.verticalCenter: parent.verticalCenter
                }
            }
            Rectangle {
                width: txtRecord.implicitWidth
                height: 2
                radius: 1
                color: "#4A90FF"
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.bottom: parent.bottom
                anchors.bottomMargin: 2
                visible: optionsBar.activePage === "record"
            }
            MouseArea {
                id: maRecord
                anchors.fill: parent
                hoverEnabled: true
                onClicked: {
                    closeMenus()
                    _activeMenu = 2
                    optionsBar.experimentRecordRequested()
                }
            }
        }

        // ---- 仪器 ----
        Rectangle {
            Layout.preferredWidth: txtInst.implicitWidth + 32
            Layout.preferredHeight: 36
            //color: maInst.containsMouse || _activeMenu === 3 ? "#C0D8F0" : "transparent"
            Behavior on color { ColorAnimation { duration: 120 } }
            color: "transparent"
            z: 100

            Row {
                anchors.centerIn: parent
                spacing: 4
                Text {
                    id: txtInst
                    text: qsTr("仪器")
                    font.pixelSize: 14
                    color: !optionsBar.deviceAvailable ? "#A8A8A8" : (_activeMenu === 3 ? "#336FFF" : "#444444")
                    font.family: "Microsoft YaHei"
                    anchors.verticalCenter: parent.verticalCenter
                }
                Text {
                    text: _activeMenu === 3 ? "\u2227" : "\u2228"
                    font.pixelSize: 10
                    color: optionsBar.deviceAvailable ? "#999999" : "#C0C0C0"
                    anchors.verticalCenter: parent.verticalCenter
                }
            }
            MouseArea {
                id: maInst
                anchors.fill: parent
                hoverEnabled: true
                onClicked: {
                    if (!optionsBar.deviceAvailable) {
                        optionsBar.showDeviceUnavailableTip()
                        return
                    }
                    if (justClosed(3)) { return }
                    if (_activeMenu === 3 && instrumentPopup.visible) {
                        closeMenus()
                        return
                    }
                    closeMenus()
                    _activeMenu = 3
                    instrumentPopup.open()
                }
            }

            Popup {
                id: instrumentPopup
                y: 36
                x: 0
                width: 150
                height: colInst.height + 16
                padding: 0
                closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
                onClosed: {
                    _lastClosedMenu = 3
                    _lastClosedTs = Date.now()
                    if (_activeMenu === 3) _activeMenu = 0
                }

                background: Rectangle {
                    color: "#FFFFFF"
                    border.color: "#D6E4F0"
                    border.width: 1
                    radius: 6

                    Rectangle {
                        width: 3
                        anchors.top: parent.top
                        anchors.bottom: parent.bottom
                        anchors.left: parent.left
                        anchors.topMargin: 8
                        anchors.bottomMargin: 8
                        anchors.leftMargin: 6
                        color: "#3B8CFF"
                        radius: 2
                    }
                }

                contentItem: Item {
                    Column {
                        id: colInst
                        x: 16; y: 8
                        width: 150 - 22

                        Rectangle {
                            width: parent.width; height: 36
                            color: maI1.containsMouse ? "#EBF3FF" : "transparent"
                            radius: 4
                            Text { text: qsTr("仪器检查"); font.pixelSize: 14; color: "#333333"; font.family: "Microsoft YaHei"; anchors.verticalCenter: parent.verticalCenter; x: 10 }
                            // 菜单项本身只负责派发事件，避免直接依赖外部页面或弹窗 id。
                            MouseArea {
                                id: maI1;
                                anchors.fill: parent;
                                hoverEnabled: true;
                                cursorShape: Qt.PointingHandCursor;
                                onClicked: {
                                    optionsBar.instrumentCheckRequested();
                                    closeMenus()
                                }
                            }
                        }
                    }
                }
            }
        }

        // ---- 用户 ----
        Rectangle {
            Layout.preferredWidth: txtUser.implicitWidth + 32
            Layout.preferredHeight: 36
            //color: maUser.containsMouse || _activeMenu === 4 ? "#C0D8F0" : "transparent"
            Behavior on color { ColorAnimation { duration: 120 } }
            color: "transparent"
            z: 100

            Row {
                anchors.centerIn: parent
                spacing: 4
                Text {
                    id: txtUser
                    text: qsTr("用户")
                    font.pixelSize: 14
                    color: _activeMenu === 4 ? "#336FFF" : "#444444"
                    font.family: "Microsoft YaHei"
                    anchors.verticalCenter: parent.verticalCenter
                }
                Text {
                    text: _activeMenu === 4 ? "\u2227" : "\u2228"
                    font.pixelSize: 10
                    color: "#999999"
                    anchors.verticalCenter: parent.verticalCenter
                }
            }
            MouseArea {
                id: maUser
                anchors.fill: parent
                hoverEnabled: true
                onClicked: {
                    if (justClosed(4)) { return }
                    if (_activeMenu === 4 && userPopup.visible) {
                        closeMenus()
                        return
                    }
                    closeMenus()
                    _activeMenu = 4
                    userPopup.open()
                }
            }

            Popup {
                id: userPopup
                y: 36
                x: 0
                width: 150
                height: colUser.height + 16
                padding: 0
                closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
                onClosed: {
                    _lastClosedMenu = 4
                    _lastClosedTs = Date.now()
                    if (_activeMenu === 4) _activeMenu = 0
                }

                background: Rectangle {
                    color: "#FFFFFF"
                    border.color: "#D6E4F0"
                    border.width: 1
                    radius: 6

                    Rectangle {
                        width: 3
                        anchors.top: parent.top
                        anchors.bottom: parent.bottom
                        anchors.left: parent.left
                        anchors.topMargin: 8
                        anchors.bottomMargin: 8
                        anchors.leftMargin: 6
                        color: "#3B8CFF"
                        radius: 2
                    }
                }

                contentItem: Item {
                    Column {
                        id: colUser
                        x: 16; y: 8
                        width: 150 - 22

                        Rectangle {
                            width: parent.width; height: 36
                            color: maU1.containsMouse ? "#EBF3FF" : "transparent"
                            radius: 4
                            Text { text: qsTr("用户管理"); font.pixelSize: 14; color: "#333333"; font.family: "Microsoft YaHei"; anchors.verticalCenter: parent.verticalCenter; x: 10 }
                            MouseArea {
                                id: maU1
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: {
                                    optionsBar.userManagementRequested()
                                    closeMenus()
                                }
                            }
                        }
                        Rectangle {
                            width: parent.width; height: 36
                            color: maU2.containsMouse ? "#EBF3FF" : "transparent"
                            radius: 4
                            Text { text: qsTr("注销登录"); font.pixelSize: 14; color: "#333333"; font.family: "Microsoft YaHei"; anchors.verticalCenter: parent.verticalCenter; x: 10 }
                            MouseArea { id: maU2; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor;
                                onClicked: { console.log("注销登录");
                                    closeMenus()
                                    custom_pop.show(3)
                                }
                            }
                        }
                    }
                }
            }
        }

        // ---- 帮助 ----
        Rectangle {
            Layout.preferredWidth: txtHelp.implicitWidth + 32
            Layout.preferredHeight: 36
            //color: maHelp.containsMouse || _activeMenu === 5 ? "#C0D8F0" : "transparent"
            Behavior on color { ColorAnimation { duration: 120 } }
            color: "transparent"
            z: 100

            Row {
                anchors.centerIn: parent
                spacing: 4
                Text {
                    id: txtHelp
                    text: qsTr("帮助")
                    font.pixelSize: 14
                    color: _activeMenu === 5 ? "#336FFF" : "#444444"
                    font.family: "Microsoft YaHei"
                    anchors.verticalCenter: parent.verticalCenter
                }
                Text {
                    text: _activeMenu === 5 ? "\u2227" : "\u2228"
                    font.pixelSize: 10
                    color: "#999999"
                    anchors.verticalCenter: parent.verticalCenter
                }
            }
            MouseArea {
                id: maHelp
                anchors.fill: parent
                hoverEnabled: true
                onClicked: {
                    if (justClosed(5)) { return }
                    if (_activeMenu === 5 && helpPopup.visible) {
                        closeMenus()
                        return
                    }
                    closeMenus()
                    _activeMenu = 5
                    helpPopup.open()
                }
            }

            Popup {
                id: helpPopup
                y: 36
                x: 0
                width: 160
                height: colHelp.height + 16
                padding: 0
                closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
                onClosed: {
                    _lastClosedMenu = 5
                    _lastClosedTs = Date.now()
                    if (_activeMenu === 5) _activeMenu = 0
                }

                onOpened: {
                    _langSubOpen = false
                }

                background: Rectangle {
                    color: "#FFFFFF"
                    border.color: "#D6E4F0"
                    border.width: 1
                    radius: 6

                    Rectangle {
                        width: 3
                        anchors.top: parent.top
                        anchors.bottom: parent.bottom
                        anchors.left: parent.left
                        anchors.topMargin: 8
                        anchors.bottomMargin: 8
                        anchors.leftMargin: 6
                        color: "#3B8CFF"
                        radius: 2
                    }
                }

                contentItem: Item {
                    Column {
                        id: colHelp
                        x: 16; y: 8
                        width: 160 - 22

                        Rectangle {
                            width: parent.width; height: 36
                            color: maH1.containsMouse ? "#EBF3FF" : "transparent"
                            radius: 4
                            Text { text: qsTr("说明书"); font.pixelSize: 14; color: "#333333"; font.family: "Microsoft YaHei"; anchors.verticalCenter: parent.verticalCenter; x: 10 }
                            MouseArea {
                                id: maH1
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: {
                                    optionsBar.instructionRequested()
                                    closeMenus()
                                }
                            }
                        }
                        Rectangle {
                            width: parent.width; height: 36
                            color: maH2.containsMouse ? "#EBF3FF" : "transparent"
                            radius: 4
                            Text { text: qsTr("回收站"); font.pixelSize: 14; color: "#333333"; font.family: "Microsoft YaHei"; anchors.verticalCenter: parent.verticalCenter; x: 10 }
                            MouseArea {
                                id: maH2
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: {
                                    optionsBar.recycleBinRequested()
                                    closeMenus()
                                }
                            }
                        }
                        Rectangle {
                            width: parent.width; height: 36
                            color: maH3.containsMouse ? "#EBF3FF" : "transparent"
                            radius: 4
                            Text { text: qsTr("切换语言"); font.pixelSize: 14; color: "#333333"; font.family: "Microsoft YaHei"; anchors.verticalCenter: parent.verticalCenter; x: 10 }
                            Text { text: _langSubOpen ? "\u2227" : "\u203A"; font.pixelSize: 12; color: "#999999"; anchors.verticalCenter: parent.verticalCenter; anchors.right: parent.right; anchors.rightMargin: 8 }
                            MouseArea { id: maH3; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: _langSubOpen = !_langSubOpen }
                        }
                        Rectangle {
                            visible: _langSubOpen
                            width: parent.width; height: 32
                            color: maL1.containsMouse ? "#EBF3FF" : "transparent"
                            radius: 4
                            Text { text: "English"; font.pixelSize: 13; color: "#555555"; font.family: "Microsoft YaHei"; anchors.verticalCenter: parent.verticalCenter; x: 24 }
                            MouseArea { id: maL1; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: { console.log("English"); closeMenus() } }
                        }
                        Rectangle {
                            visible: _langSubOpen
                            width: parent.width; height: 32
                            color: maL2.containsMouse ? "#EBF3FF" : "transparent"
                            radius: 4
                            Text { text: qsTr("简体中文"); font.pixelSize: 13; color: "#555555"; font.family: "Microsoft YaHei"; anchors.verticalCenter: parent.verticalCenter; x: 24 }
                            MouseArea { id: maL2; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: { console.log("简体中文"); closeMenus() } }
                        }
                        Rectangle {
                            width: parent.width; height: 36
                            color: maH4.containsMouse ? "#EBF3FF" : "transparent"
                            radius: 4
                            Text { text: qsTr("升级"); font.pixelSize: 14; color: "#333333"; font.family: "Microsoft YaHei"; anchors.verticalCenter: parent.verticalCenter; x: 10 }
                            MouseArea { id: maH4; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: { console.log("升级"); closeMenus() } }
                        }
                    }
                }
            }
        }

        Item { Layout.fillWidth: true }

    }

}
