import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12

Column {
    id: root
    width: 820
    height: 445
    spacing: 0

    // 表头标题数组，例如 ["用户名", "密码", "等级"]
    property var rowlist: [qsTr("用户名"), qsTr("密码"), qsTr("等级")]

    // 暴露 ListView
    property alias listView: list

    // 用户列表模型（从外部传入）
    property var userListModel: user_list_model

    // 当前选中行
    property int currentIndex: -1

    // 密码是否以密文形式显示（true=密文，false=明文）
    property bool maskPassword: false

    // 选中行信号，参数为行索引和该行的三个字段
    signal rowSelected(int row, var col1, var col2, var col3)
    // 取消选中信号
    signal rowDeselected()

    Rectangle {
        id: headerRow
        width: parent.width
        height: 32
        color: "#D0E1F1"
        border.color: "#B0C4DE"
        border.width: 1

        Row {
            id: headerContent
            anchors.fill: parent
            spacing: 0

            Repeater {
                model: root.rowlist.length
                delegate: Rectangle {
                    width: parent.width / root.rowlist.length
                    height: parent.height
                    color: "#D0E1F1"
                    border.color: "#B0C4DE"
                    border.width: 1

                    Text {
                        anchors.fill: parent
                        verticalAlignment: Text.AlignVCenter
                        horizontalAlignment: Text.AlignHCenter
                        text: root.rowlist[index]
                        font.pointSize: 13
                        color: "#5F85A8"
                        elide: Text.ElideRight
                        clip: true
                        font.bold: true
                    }
                }
            }
        }
    }

    Rectangle {
        id: listview
        width: parent.width
        height: parent.height - headerRow.height
        color: "white"
        border.color: "#B0C4DE"
        border.width: 1

        MouseArea {
            anchors.fill: parent
            z: 1
            onClicked: {
                if (root.currentIndex !== -1) {
                    root.currentIndex = -1;
                    root.rowDeselected();
                }
            }
        }

        ListView {
            id: list
            anchors.fill: parent
            anchors.margins: 0
            model: userListModel
            interactive: true
            focus: true
            clip: true
            z: 2
            currentIndex: root.currentIndex
            boundsBehavior: Flickable.StopAtBounds

            delegate: Row {
                width: list.width
                height: 32

                Rectangle {
                    width: list.width / 3
                    height: parent.height
                    color: index === root.currentIndex ? "#A0C4E4" : "white"
                    border.color: "#B0C4DE"
                    border.width: 1

                    Text {
                        anchors.centerIn: parent
                        text: model.username
                        font.pointSize: 12
                        color: "black"
                        elide: Text.ElideRight
                        clip: true
                    }
                    MouseArea {
                        anchors.fill: parent
                        acceptedButtons: Qt.LeftButton
                        propagateComposedEvents: false
                        onClicked: {
                            if (root.currentIndex === index) {
                                root.currentIndex = -1;
                                root.rowDeselected();
                            } else {
                                root.currentIndex = index;
                                root.rowSelected(
                                    index,
                                    model.username,
                                    root.maskPassword ? "******" : model.password,
                                    typeof model.lv === "string"
                                        ? (model.lv.trim() === "1" ? qsTr("管理员")
                                            : model.lv.trim() === "0" ? qsTr("操作员")
                                            : model.lv.trim())
                                        : (model.lv === 1 ? qsTr("管理员")
                                            : model.lv === 0 ? qsTr("操作员")
                                            : model.lv)
                                );
                            }
                        }
                    }
                }
                Rectangle {
                    width: list.width / 3
                    height: parent.height
                    color: index === root.currentIndex ? "#A0C4E4" : "white"
                    border.color: "#B0C4DE"
                    border.width: 1

                    Text {
                        anchors.centerIn: parent
                        text: root.maskPassword ? "******" : model.password
                        font.pointSize: 12
                        color: "black"
                        elide: Text.ElideRight
                        clip: true
                    }
                    MouseArea {
                        anchors.fill: parent
                        acceptedButtons: Qt.LeftButton
                        propagateComposedEvents: false
                        onClicked: {
                            if (root.currentIndex === index) {
                                root.currentIndex = -1;
                                root.rowDeselected();
                            } else {
                                root.currentIndex = index;
                                root.rowSelected(
                                    index,
                                    model.username,
                                    root.maskPassword ? "******" : model.password,
                                    typeof model.lv === "string"
                                        ? (model.lv.trim() === "1" ? qsTr("管理员")
                                            : model.lv.trim() === "0" ? qsTr("操作员")
                                            : model.lv.trim())
                                        : (model.lv === 1 ? qsTr("管理员")
                                            : model.lv === 0 ? qsTr("操作员")
                                            : model.lv)
                                );
                            }
                        }
                    }
                }
                Rectangle {
                    width: list.width / 3
                    height: parent.height
                    color: index === root.currentIndex ? "#A0C4E4" : "white"
                    border.color: "#B0C4DE"
                    border.width: 1

                    Text {
                        anchors.centerIn: parent
                        text: {
                            var lv = model.lv
                            if (typeof lv === "string") lv = lv.trim()
                            if (lv === 1 || lv === "1") return qsTr("管理员")
                            if (lv === 0 || lv === "0") return qsTr("操作员")
                            return lv
                        }
                        font.pointSize: 12
                        color: "black"
                        elide: Text.ElideRight
                        clip: true
                    }
                    MouseArea {
                        anchors.fill: parent
                        acceptedButtons: Qt.LeftButton
                        propagateComposedEvents: false
                        onClicked: {
                            if (root.currentIndex === index) {
                                root.currentIndex = -1;
                                root.rowDeselected();
                            } else {
                                root.currentIndex = index;
                                root.rowSelected(
                                    index,
                                    model.username,
                                    root.maskPassword ? "******" : model.password,
                                    typeof model.lv === "string"
                                        ? (model.lv.trim() === "1" ? qsTr("管理员")
                                            : model.lv.trim() === "0" ? qsTr("操作员")
                                            : model.lv.trim())
                                        : (model.lv === 1 ? qsTr("管理员")
                                            : model.lv === 0 ? qsTr("操作员")
                                            : model.lv)
                                );
                            }
                        }
                    }
                }
            }
        }
    }
}
