import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Shapes 1.12
import QtQuick.Layouts 1.12

import "component"

Item {
    id: user_root
    objectName: "user_page"

    property bool hasSelection: selectedRow >= 0 && selectedUserId > 0
    property int selectedRow: -1
    property int selectedUserId: -1
    property int selectedUserRole: -1
    property bool isAdminUser: user_ctrl.isAdmin
    property bool canAddUser: isAdminUser
    property bool canViewPassword: isAdminUser
    property bool selectedIsAdmin: selectedUserRole === 1
    property bool canEditSelected: hasSelection && isAdminUser
                                    && (!selectedIsAdmin || selectedUserId === user_ctrl.currentUserId)
    property bool formReadOnly: hasSelection ? !canEditSelected : !canAddUser

    function resetInputs() {
        userNameInput.text = "";
        userPasswordInput.text = "";
        userRoleComboBox.currentIndex = -1;
        hasSelection = false;
        selectedRow = -1;
        selectedUserId = -1;
    }

    function roleTextToIndex(role) {
        if(role === qsTr("管理员")) return 1;
        if(role === qsTr("操作员")) return 0;
        return -1;
    }

    function indexToRoleText(index) {
        if(index === 1) return qsTr("管理员");
        if(index === 0) return qsTr("操作员");
        return "";
    }

    function msg(text) {
        if (typeof info_pop !== "undefined" && info_pop.openDialog) {
            info_pop.openDialog(text)
        } else {
            console.log(text)
        }
    }

    function roleIndex(value) {
        if (typeof value === "string") {
            value = value.trim()
        }
        return (value === 1 || value === "1") ? 1 : 0
    }

    function resetCreateMode() {
        selectedRow = -1
        selectedUserId = -1
        selectedUserRole = -1
        userNameInput.text = ""
        userPasswordInput.text = ""
        userRoleComboBox.currentIndex = 0
        if (userListView) {
            userListView.currentIndex = -1
        }
    }

    function selectUser(row) {
        selectedRow = row
        selectedUserId = Number(user_list_model.get(row, "id"))
        selectedUserRole = roleIndex(user_list_model.get(row, "lv"))
        userNameInput.text = user_list_model.get(row, "username")
        userPasswordInput.text = canViewPassword ? user_list_model.get(row, "password") : ""
        userRoleComboBox.currentIndex = selectedUserRole

        if (!isAdminUser) {
            msg(qsTr("操作员不可修改用户信息"))
        } else if (!canEditSelected) {
            msg(qsTr("管理员不可以修改除自己外的其他管理员"))
        }
    }

    function toggleUserSelection(row) {
        if (!isAdminUser) {
            if (selectedRow === row) {
                resetCreateMode()
            } else {
                selectedRow = row
                selectedUserId = -1
                selectedUserRole = -1
                userNameInput.text = ""
                userPasswordInput.text = ""
                userRoleComboBox.currentIndex = 0
            }
            return
        }

        if (hasSelection && selectedRow === row) {
            resetCreateMode()
            return
        }

        selectUser(row)
    }

    function checkCreate() {
        var username = userNameInput.text.trim()
        var password = userPasswordInput.text.trim()

        if (username === "") {
            msg(qsTr("请输入用户名"))
            return false
        }
        if (username.toLowerCase() === "admin") {
            msg(qsTr("用户名不能为 admin"))
            return false
        }
        if (password === "") {
            msg(qsTr("请输入密码"))
            return false
        }
        return true
    }

    function checkSave() {
        var username = userNameInput.text.trim()

        if (username === "") {
            msg(qsTr("请输入用户名"))
            return false
        }
        if (username.toLowerCase() === "admin") {
            msg(qsTr("用户名不能为 admin"))
            return false
        }
        return true
    }

    function addUser() {
        if (!canAddUser) {
            msg(qsTr("操作员不可新增或修改用户"))
            return
        }
        if (!checkCreate()) {
            return
        }

        if (user_ctrl.addUser(userNameInput.text.trim(),
                              userPasswordInput.text.trim(),
                              userRoleComboBox.currentIndex)) {
            user_list_model.reloadFromDb()
            resetCreateMode()
            msg(qsTr("添加用户成功"))
        }
    }

    function saveUser() {
        if (!hasSelection) {
            msg(qsTr("请先选中用户"))
            return
        }
        if (!canEditSelected) {
            if (!isAdminUser) {
                msg(qsTr("操作员不可修改用户信息"))
            } else {
                msg(qsTr("管理员不可以修改除自己外的其他管理员"))
            }
            return
        }
        if (!checkSave()) {
            return
        }

        if (user_ctrl.updateUser(selectedUserId,
                                 userNameInput.text.trim(),
                                 userPasswordInput.text.trim(),
                                 userRoleComboBox.currentIndex)) {
            user_list_model.reloadFromDb()
            resetCreateMode()
            msg(qsTr("修改用户成功"))
        }
    }

    Connections {
        target: user_ctrl

        onOperationFailed: {
            user_root.msg(message)
        }

        onCurrentUserChanged: {
            user_root.resetCreateMode()
        }
    }

    Component.onCompleted: {
        user_list_model.reloadFromDb()
        resetCreateMode()
    }

    Rectangle {
        anchors.fill: parent
        anchors.margins: 10
        color: "transparent"

        ColumnLayout {
            anchors.fill: parent
            spacing: 15

            RowLayout {
                id: lieneditrow
                Layout.fillWidth: true
                Layout.preferredHeight: 42
                spacing: 28

                LineEdit {
                    id: userNameInput
                    Layout.preferredHeight: 42
                    Layout.preferredWidth: 245
                    validator: RegExpValidator { regExp: /^[a-zA-Z0-9_]{1,16}$/ }
                    input_en: !user_root.formReadOnly
                    placeholderText: qsTr("请输入用户名")
                    onTextChanged: {
                        if (userNameInput.text.toLowerCase() === "admin") {
                            userNameInput.text = "";
                            info_pop.message = qsTr("用户名不能为 admin");
                            info_pop.open();
                        }
                    }
                }

                LineEdit {
                    id: userPasswordInput
                    Layout.preferredHeight: 42
                    Layout.preferredWidth: 245
                    echoMode: user_root.canViewPassword ? TextInput.Normal : TextInput.Password
                    input_en: !user_root.formReadOnly && user_root.canViewPassword
                    placeholderText: qsTr(hasSelection ? qsTr("留空则不修改密码") : qsTr("请输入密码"))
                }

                RowLayout{
                    Layout.preferredHeight: 42
                    Layout.preferredWidth: 245
                    spacing: 0
                    Label {
                        Layout.preferredWidth: 55
                        Layout.preferredHeight: 42
                        color: "black"
                        text: qsTr("等级:")
                        font.pixelSize: 18
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }

                    ComboBox {
                        id: userRoleComboBox
                        Layout.preferredWidth: 180
                        Layout.preferredHeight: 42
                        font.pixelSize: 18
                        enabled: !user_root.formReadOnly
                        model: [qsTr("操作员"), qsTr("管理员")]
                        background: Rectangle {
                            border.color: "#cccccc"
                            radius: 4
                        }
                    }
                }

                RowLayout {
                    id: actionButtons
                    Layout.preferredWidth: hasSelection ? 280 : 180
                    spacing: 10

                    Popup_button {
                        id: addBtn
                        visible: !hasSelection
                        opacity: user_root.canAddUser ? 1.0 : 0.6
                        Layout.fillWidth: true
                        Layout.preferredHeight: 45
                        background: Rectangle {
                            radius: 4
                            color: "#3B87E4"
                            scale: root.scaleFactor
                            Behavior on scale { NumberAnimation { duration: 80; easing.type: Easing.InOutQuad } }
                        }
                        btnText: qsTr("添加")
                        onClicked: {
                            user_root.addUser()
                            return
                            if (userNameInput.text === "" || userPasswordInput.text === "") {
                                info_pop.message = qsTr("用户名和密码不能为空");
                                info_pop.open();
                                return;
                            }

                            var ok = user_ctrl.addUser(userNameInput.text, userPasswordInput.text, userRoleComboBox.currentIndex)
                            if (ok) {
                                info_pop.message = qsTr("添加用户成功");
                                info_pop.open();
                                user_list_model.reloadFromDb()
                                resetInputs();
                            }
                        }
                    }

                    Popup_button {
                        id: modifyBtn
                        visible: hasSelection
                        opacity: user_root.canEditSelected ? 1.0 : 0.6
                        Layout.fillWidth: true
                        Layout.preferredHeight: 45
                        background: Rectangle {
                            radius: 4
                            color: "#F5A623"
                            scale: root.scaleFactor
                            Behavior on scale { NumberAnimation { duration: 80; easing.type: Easing.InOutQuad } }
                        }
                        btnText: qsTr("修改")
                        onClicked: {
                            user_root.saveUser()
                            return
                            if (selectedUserId <= 0) {
                                info_pop.message = qsTr("请先选择要修改的用户");
                                info_pop.open();
                                return;
                            }

                            var newUsername = userNameInput.text.trim();
                            var newPassword = userPasswordInput.text.trim();
                            var newRole = userRoleComboBox.currentIndex;

                            if (newUsername === "") {
                                info_pop.message = qsTr("用户名不能为空");
                                info_pop.open();
                                return;
                            }

                            var ok = user_ctrl.updateUser(selectedUserId, newUsername, newPassword, newRole);
                            if (ok) {
                                info_pop.message = qsTr("修改用户成功");
                                info_pop.open();
                                user_list_model.reloadFromDb()
                                resetInputs();
                            }
                        }
                    }
                }
            }

            CustomListView_user {
                id: userListView
                Layout.fillWidth: true
                Layout.fillHeight: true
                maskPassword: !user_ctrl.isAdmin

                onRowSelected: {
                    user_root.toggleUserSelection(row)
                }
                onRowDeselected: {
                    user_root.resetCreateMode()
                }
            }
        }
    }
}
