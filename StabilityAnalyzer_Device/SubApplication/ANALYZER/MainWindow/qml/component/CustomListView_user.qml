import QtQuick 2.9
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3

Column {
    id: root
    width: 820
    height: 445
    spacing: 0

    // 琛ㄥご鏍囬鏁扮粍锛屼緥濡?["鐢ㄦ埛鍚?, "瀵嗙爜", "绛夌骇"]
    property var rowlist: [qsTr("鐢ㄦ埛鍚?), qsTr("瀵嗙爜"), qsTr("绛夌骇")]

    // 鏆撮湶 ListView
    property alias listView: list

    // 鐢ㄦ埛鍒楄〃妯″瀷锛堜粠澶栭儴浼犲叆锛?
    property var userListModel: user_list_model

    // 褰撳墠閫変腑琛?
    property int currentIndex: -1

    // 瀵嗙爜鏄惁浠ュ瘑鏂囧舰寮忔樉绀猴紙true=瀵嗘枃锛宖alse=鏄庢枃锛?
    property bool maskPassword: false

    // 閫変腑琛屼俊鍙凤紝鍙傛暟涓鸿绱㈠紩鍜岃琛岀殑涓変釜瀛楁
    signal rowSelected(int row, var col1, var col2, var col3)
    // 鍙栨秷閫変腑淇″彿
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
                                        ? (model.lv.trim() === "1" ? qsTr("绠＄悊鍛?)
                                            : model.lv.trim() === "0" ? qsTr("鎿嶄綔鍛?)
                                            : model.lv.trim())
                                        : (model.lv === 1 ? qsTr("绠＄悊鍛?)
                                            : model.lv === 0 ? qsTr("鎿嶄綔鍛?)
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
                                        ? (model.lv.trim() === "1" ? qsTr("绠＄悊鍛?)
                                            : model.lv.trim() === "0" ? qsTr("鎿嶄綔鍛?)
                                            : model.lv.trim())
                                        : (model.lv === 1 ? qsTr("绠＄悊鍛?)
                                            : model.lv === 0 ? qsTr("鎿嶄綔鍛?)
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
                            if (lv === 1 || lv === "1") return qsTr("绠＄悊鍛?)
                            if (lv === 0 || lv === "0") return qsTr("鎿嶄綔鍛?)
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
                                        ? (model.lv.trim() === "1" ? qsTr("绠＄悊鍛?)
                                            : model.lv.trim() === "0" ? qsTr("鎿嶄綔鍛?)
                                            : model.lv.trim())
                                        : (model.lv === 1 ? qsTr("绠＄悊鍛?)
                                            : model.lv === 0 ? qsTr("鎿嶄綔鍛?)
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

