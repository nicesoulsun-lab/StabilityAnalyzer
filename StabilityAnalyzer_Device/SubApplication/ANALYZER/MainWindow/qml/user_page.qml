import QtQuick 2.9
import QtQuick.Controls 2.2
import QtQuick.Shapes 1.12
import QtQuick.Layouts 1.3

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
        if(role === qsTr("绠＄悊鍛?)) return 1;
        if(role === qsTr("鎿嶄綔鍛?)) return 0;
        return -1;
    }

    function indexToRoleText(index) {
        if(index === 1) return qsTr("绠＄悊鍛?);
        if(index === 0) return qsTr("鎿嶄綔鍛?);
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
            msg(qsTr("鎿嶄綔鍛樹笉鍙慨鏀圭敤鎴蜂俊鎭?))
        } else if (!canEditSelected) {
            msg(qsTr("绠＄悊鍛樹笉鍙互淇敼闄よ嚜宸卞鐨勫叾浠栫鐞嗗憳"))
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
            msg(qsTr("璇疯緭鍏ョ敤鎴峰悕"))
            return false
        }
        if (username.toLowerCase() === "admin") {
            msg(qsTr("鐢ㄦ埛鍚嶄笉鑳戒负 admin"))
            return false
        }
        if (password === "") {
            msg(qsTr("璇疯緭鍏ュ瘑鐮?))
            return false
        }
        return true
    }

    function checkSave() {
        var username = userNameInput.text.trim()

        if (username === "") {
            msg(qsTr("璇疯緭鍏ョ敤鎴峰悕"))
            return false
        }
        if (username.toLowerCase() === "admin") {
            msg(qsTr("鐢ㄦ埛鍚嶄笉鑳戒负 admin"))
            return false
        }
        return true
    }

    function addUser() {
        if (!canAddUser) {
            msg(qsTr("鎿嶄綔鍛樹笉鍙柊澧炴垨淇敼鐢ㄦ埛"))
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
            msg(qsTr("娣诲姞鐢ㄦ埛鎴愬姛"))
        }
    }

    function saveUser() {
        if (!hasSelection) {
            msg(qsTr("璇峰厛閫変腑鐢ㄦ埛"))
            return
        }
        if (!canEditSelected) {
            if (!isAdminUser) {
                msg(qsTr("鎿嶄綔鍛樹笉鍙慨鏀圭敤鎴蜂俊鎭?))
            } else {
                msg(qsTr("绠＄悊鍛樹笉鍙互淇敼闄よ嚜宸卞鐨勫叾浠栫鐞嗗憳"))
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
            msg(qsTr("淇敼鐢ㄦ埛鎴愬姛"))
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
                    placeholderText: qsTr("璇疯緭鍏ョ敤鎴峰悕")
                    onTextChanged: {
                        if (userNameInput.text.toLowerCase() === "admin") {
                            userNameInput.text = "";
                            info_pop.message = qsTr("鐢ㄦ埛鍚嶄笉鑳戒负 admin");
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
                    placeholderText: qsTr(hasSelection ? qsTr("鐣欑┖鍒欎笉淇敼瀵嗙爜") : qsTr("璇疯緭鍏ュ瘑鐮?))
                }

                RowLayout{
                    Layout.preferredHeight: 42
                    Layout.preferredWidth: 245
                    spacing: 0
                    Label {
                        Layout.preferredWidth: 55
                        Layout.preferredHeight: 42
                        color: "black"
                        text: qsTr("绛夌骇:")
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
                        model: [qsTr("鎿嶄綔鍛?), qsTr("绠＄悊鍛?)]
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
                        btnText: qsTr("娣诲姞")
                        onClicked: {
                            user_root.addUser()
                            return
                            if (userNameInput.text === "" || userPasswordInput.text === "") {
                                info_pop.message = qsTr("鐢ㄦ埛鍚嶅拰瀵嗙爜涓嶈兘涓虹┖");
                                info_pop.open();
                                return;
                            }

                            var ok = user_ctrl.addUser(userNameInput.text, userPasswordInput.text, userRoleComboBox.currentIndex)
                            if (ok) {
                                info_pop.message = qsTr("娣诲姞鐢ㄦ埛鎴愬姛");
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
                        btnText: qsTr("淇敼")
                        onClicked: {
                            user_root.saveUser()
                            return
                            if (selectedUserId <= 0) {
                                info_pop.message = qsTr("璇峰厛閫夋嫨瑕佷慨鏀圭殑鐢ㄦ埛");
                                info_pop.open();
                                return;
                            }

                            var newUsername = userNameInput.text.trim();
                            var newPassword = userPasswordInput.text.trim();
                            var newRole = userRoleComboBox.currentIndex;

                            if (newUsername === "") {
                                info_pop.message = qsTr("鐢ㄦ埛鍚嶄笉鑳戒负绌?);
                                info_pop.open();
                                return;
                            }

                            var ok = user_ctrl.updateUser(selectedUserId, newUsername, newPassword, newRole);
                            if (ok) {
                                info_pop.message = qsTr("淇敼鐢ㄦ埛鎴愬姛");
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

