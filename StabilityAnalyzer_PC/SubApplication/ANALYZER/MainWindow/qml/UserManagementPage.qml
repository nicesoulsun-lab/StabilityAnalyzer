import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import "component"

Item {
    id: root
    objectName: "UserManagementPage"

    // 当前表格选中的用户。未选中时处于“新增用户”态。
    property int selectedRow: -1
    property int selectedUserId: -1
    property int selectedUserRole: -1
    property bool hasSelection: selectedRow >= 0 && selectedUserId > 0
    property bool isAdminUser: user_ctrl.isAdmin
    property bool canAddUser: isAdminUser
    property bool canViewPassword: isAdminUser
    property bool selectedIsAdmin: selectedUserRole === 1
    // 权限规则：
    // 1. 只有管理员可以新增/修改用户。
    // 2. 管理员可以修改所有操作员。
    // 3. 管理员只能修改自己，不能修改其他管理员。
    property bool canEditSelected: hasSelection && isAdminUser
                                    && (!selectedIsAdmin || selectedUserId === user_ctrl.currentUserId)
    property bool formReadOnly: hasSelection ? !canEditSelected : !canAddUser

    function msg(s) {
        if (typeof info_pop !== "undefined") info_pop.openDialog(s)
        else console.log(s)
    }

    function roleIndex(v) {
        if (typeof v === "string") v = v.trim()
        return (v === 1 || v === "1") ? 1 : 0
    }

    function roleText(v) {
        return roleIndex(v) === 1 ? qsTr("管理员") : qsTr("操作员")
    }

    function resetCreateMode() {
        selectedRow = -1
        selectedUserId = -1
        selectedUserRole = -1
        userNameInput.text = ""
        userPasswordInput.text = ""
        userRoleComboBox.currentIndex = 1
    }

    function selectUser(row) {
        selectedRow = row
        selectedUserId = Number(user_list_model.get(row, "id"))
        selectedUserRole = roleIndex(user_list_model.get(row, "lv"))
        userNameInput.text = user_list_model.get(row, "username")
        userPasswordInput.text = canViewPassword ? user_list_model.get(row, "password") : ""
        userRoleComboBox.currentIndex = selectedUserRole

        // 允许选中所有用户，但不满足权限时只进入“查看态”。
        if (!isAdminUser) msg(qsTr("操作员不可修改用户信息"))
        else if (!canEditSelected) msg(qsTr("管理员不可以修改除自己外的其他管理员"))
    }

    function toggleUserSelection(row) {
        if (!isAdminUser) {
            // 操作员只允许选中高亮，不进入编辑态。
            if (selectedRow === row) {
                selectedRow = -1
            } else {
                selectedRow = row
            }
            selectedUserId = -1
            selectedUserRole = -1
            userNameInput.text = ""
            userPasswordInput.text = ""
            userRoleComboBox.currentIndex = 1
            return
        }

        // 再次点击同一用户时退出编辑态，并丢弃未保存的表单内容。
        if (hasSelection && selectedRow === row) {
            resetCreateMode()
            return
        }

        selectUser(row)
    }

    function checkCreate() {
        var u = userNameInput.text.trim()
        var p = userPasswordInput.text.trim()
        if (u === "") { msg(qsTr("请输入用户名")); return false }
        if (u.toLowerCase() === "admin") { msg(qsTr("用户名不能为 admin")); return false }
        if (p === "") { msg(qsTr("请输入密码")); return false }
        return true
    }

    function checkSave() {
        var u = userNameInput.text.trim()
        if (u === "") { msg(qsTr("请输入用户名")); return false }
        if (u.toLowerCase() === "admin") { msg(qsTr("用户名不能为 admin")); return false }
        return true
    }

    function addUser() {
        if (!canAddUser) { msg(qsTr("操作员不可新增或修改用户")); return }
        if (!checkCreate()) return
        if (user_ctrl.addUser(userNameInput.text.trim(), userPasswordInput.text.trim(), userRoleComboBox.currentIndex)) {
            user_list_model.reloadFromDb()
            resetCreateMode()
            msg(qsTr("添加用户成功"))
        }
    }

    function saveUser() {
        if (!hasSelection) { msg(qsTr("请先选中用户")); return }
        if (!canEditSelected) {
            if (!isAdminUser) msg(qsTr("操作员不可修改用户信息"))
            else msg(qsTr("管理员不可以修改除自己外的其他管理员"))
            return
        }
        if (!checkSave()) return
        if (user_ctrl.updateUser(selectedUserId, userNameInput.text.trim(), userPasswordInput.text.trim(), userRoleComboBox.currentIndex)) {
            user_list_model.reloadFromDb()
            resetCreateMode()
            msg(qsTr("保存成功"))
        }
    }

    Connections {
        target: user_ctrl
        onOperationFailed: root.msg(message)
    }

    Component.onCompleted: {
        user_list_model.reloadFromDb()
        resetCreateMode()
    }

    Rectangle {
        anchors.fill: parent
        anchors.margins: 14
        color: "#FFFFFF"

        ColumnLayout {
            anchors.fill: parent
            spacing: 14

            Text {
                text: qsTr("用户管理")
                font.pixelSize: 18
                font.family: "Microsoft YaHei"
                color: "#333333"
                Layout.leftMargin: 12
                Layout.topMargin: 4
            }

            RowLayout {
                Layout.fillWidth: true
                Layout.leftMargin: 12
                Layout.rightMargin: 12
                Layout.preferredHeight: 40
                spacing: 14

                LineEdit {
                    id: userNameInput
                    Layout.preferredWidth: 160
                    Layout.preferredHeight: 34
                    border_color: "#A9CCF5"
                    placeholderText: qsTr("请输入用户名")
                    input_en: !root.formReadOnly
                    validator: RegExpValidator {
                        regExp: /^[a-zA-Z0-9_]{1,16}$/
                    }
                }

                LineEdit {
                    id: userPasswordInput
                    Layout.preferredWidth: 160
                    Layout.preferredHeight: 34
                    border_color: "#A9CCF5"
                    placeholderText: root.hasSelection ? (root.canViewPassword ? qsTr("请输入密码，不改可留空") : qsTr("无权限查看或修改密码")) : qsTr("请输入密码")
                    // 只有操作员进入页面时保持密文显示；管理员新增/修改都直接显示明文。
                    echoMode: root.canViewPassword ? TextInput.Normal : TextInput.Password
                    input_en: !root.formReadOnly && root.canViewPassword
                }

                RowLayout {
                    Layout.preferredWidth: 210
                    Layout.preferredHeight: 34
                    spacing: 8
                    Text {
                        text: qsTr("等级:")
                        font.pixelSize: 14
                        font.family: "Microsoft YaHei"
                        color: "#333333"
                        verticalAlignment: Text.AlignVCenter
                    }

                    ComboBox {
                        id: userRoleComboBox
                        Layout.fillWidth: true
                        Layout.preferredHeight: 34
                        model: [qsTr("操作员"), qsTr("管理员")]
                        font.pixelSize: 14
                        enabled: !root.formReadOnly

                        background: Rectangle {
                            radius: 4
                            color: "#FFFFFF"
                            border.color: "#D7E6F5"
                            border.width: 1
                        }

                        contentItem: Text {
                            text: userRoleComboBox.displayText
                            leftPadding: 12
                            rightPadding: userRoleComboBox.indicator.width + userRoleComboBox.spacing
                            verticalAlignment: Text.AlignVCenter
                            font: userRoleComboBox.font
                            color: "#333333"
                        }
                    }
                }

                Button {
                    id: addButton
                    Layout.preferredWidth: 108
                    Layout.preferredHeight: 34
                    // 未选中用户时处于新增态，只显示“添加”。
                    // 操作员可以看到按钮，但按钮保持禁用态。
                    visible: !root.hasSelection
                    enabled: root.canAddUser
                    text: qsTr("添加")
                    onClicked: root.addUser()

                    contentItem: Text {
                        text: addButton.text
                        color: "#FFFFFF"
                        font.pixelSize: 14
                        font.family: "Microsoft YaHei"
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }

                    background: Rectangle {
                        radius: 4
                        color: addButton.enabled ? "#3B82F6" : "#A9BBD3"
                    }
                }

                Item {
                    Layout.fillWidth: true
                }

                Button {
                    id: saveButton
                    Layout.preferredWidth: 108
                    Layout.preferredHeight: 34
                    // 选中已有用户后切到编辑/查看态，显示“保存”。
                    visible: root.hasSelection
                    enabled: root.canEditSelected
                    text: qsTr("保存")
                    onClicked: root.saveUser()

                    contentItem: Text {
                        text: saveButton.text
                        color: "#FFFFFF"
                        font.pixelSize: 14
                        font.family: "Microsoft YaHei"
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }

                    background: Rectangle {
                        radius: 4
                        color: saveButton.enabled ? "#F5A623" : "#A9BBD3"
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.leftMargin: 12
                Layout.rightMargin: 12
                color: "#FFFFFF"
                border.color: "#E2E8F0"
                border.width: 1

                Column {
                    anchors.fill: parent
                    spacing: 0

                    Rectangle {
                        width: parent.width
                        height: 40
                        color: "#F5F7FA"

                        Row {
                            anchors.fill: parent
                            spacing: 0

                            Repeater {
                                model: [
                                    {t: qsTr("用户名"), w: 0.34},
                                    {t: qsTr("密码"), w: 0.33},
                                    {t: qsTr("等级"), w: 0.33}
                                ]

                                delegate: Rectangle {
                                    width: parent.width * modelData.w
                                    height: parent.height
                                    color: "transparent"
                                    border.color: "#E2E8F0"
                                    border.width: 1

                                    Text {
                                        anchors.centerIn: parent
                                        text: modelData.t
                                        color: "#333333"
                                        font.pixelSize: 13
                                        font.family: "Microsoft YaHei"
                                    }
                                }
                            }
                        }
                    }

                    ListView {
                        id: userListView
                        width: parent.width
                        height: parent.height - 40
                        clip: true
                        model: user_list_model
                        boundsBehavior: Flickable.StopAtBounds

                        delegate: Rectangle {
                            width: userListView.width
                            height: 40
                            color: index === root.selectedRow ? "#D8EBFF" : "#FFFFFF"

                            Row {
                                anchors.fill: parent
                                spacing: 0

                                Rectangle {
                                    width: parent.width * 0.34
                                    height: parent.height
                                    color: "transparent"
                                    border.color: "#E2E8F0"
                                    border.width: 1

                                    Text {
                                        anchors.centerIn: parent
                                        text: model.username
                                        color: "#333333"
                                        font.pixelSize: 13
                                        font.family: "Microsoft YaHei"
                                    }

                                    MouseArea {
                                        anchors.fill: parent
                                        onClicked: root.toggleUserSelection(index)
                                    }
                                }

                                Rectangle {
                                    width: parent.width * 0.33
                                    height: parent.height
                                    color: "transparent"
                                    border.color: "#E2E8F0"
                                    border.width: 1

                                    Text {
                                        anchors.centerIn: parent
                                        text: root.canViewPassword ? model.password : "******"
                                        color: "#333333"
                                        font.pixelSize: 13
                                        font.family: "Microsoft YaHei"
                                    }

                                    MouseArea {
                                        anchors.fill: parent
                                        onClicked: root.toggleUserSelection(index)
                                    }
                                }

                                Rectangle {
                                    width: parent.width * 0.33
                                    height: parent.height
                                    color: "transparent"
                                    border.color: "#E2E8F0"
                                    border.width: 1

                                    Text {
                                        anchors.centerIn: parent
                                        text: root.roleText(model.lv)
                                        color: "#333333"
                                        font.pixelSize: 13
                                        font.family: "Microsoft YaHei"
                                    }

                                    MouseArea {
                                        anchors.fill: parent
                                        onClicked: root.toggleUserSelection(index)
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
