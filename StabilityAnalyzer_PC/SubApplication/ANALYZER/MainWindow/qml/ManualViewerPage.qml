import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12

Item {
    id: manualPage
    objectName: "ManualViewerPage"

    signal backRequested()

    // 页数较多时使用 ListView，只创建可视区域附近的页面，减少内存占用。
    property real manualPageRatio: 1680 / 1200
    property var manualImages: [
        "qrc:/manual/manual_page_1.svg",
        "qrc:/manual/manual_page_2.svg"
    ]
    property int manualRasterWidth: 960

    Rectangle {
        anchors.fill: parent
        color: "#F7FAFD"

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 12
            spacing: 12

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 52
                radius: 6
                color: "#FFFFFF"
                border.color: "#D9E6F2"
                border.width: 1

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 12
                    anchors.rightMargin: 12
                    spacing: 12

                    Button {
                        id: backButton
                        Layout.preferredWidth: 92
                        Layout.preferredHeight: 34
                        text: qsTr("返回")
                        onClicked: manualPage.backRequested()

                        contentItem: Text {
                            text: backButton.text
                            color: "#005BAC"
                            font.pixelSize: 14
                            font.family: "Microsoft YaHei"
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }

                        background: Rectangle {
                            radius: 4
                            color: "#EAF4FF"
                            border.color: "#A9CCF5"
                            border.width: 1
                        }
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 2

                        Text {
                            text: qsTr("操作说明书")
                            font.pixelSize: 18
                            font.family: "Microsoft YaHei"
                            color: "#24364B"
                        }
                    }
                }
            }

            ListView {
                id: manualListView
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                spacing: 18
                model: manualPage.manualImages
                boundsBehavior: Flickable.StopAtBounds
                cacheBuffer: 0

                ScrollBar.vertical: ScrollBar {
                    policy: ScrollBar.AsNeeded
                }

                delegate: Rectangle {
                    width: manualListView.width
                    height: imageItem.height + 52
                    radius: 8
                    color: "#FFFFFF"
                    border.color: "#D9E6F2"
                    border.width: 1

                    Column {
                        anchors.fill: parent
                        anchors.margins: 14
                        spacing: 10

                        Text {
                            text: qsTr("第 %1 页").arg(index + 1)
                            font.pixelSize: 13
                            font.family: "Microsoft YaHei"
                            color: "#4A5D75"
                        }

                        Image {
                            id: imageItem
                            width: parent.width
                            height: width * manualPage.manualPageRatio
                            fillMode: Image.PreserveAspectFit
                            source: modelData
                            sourceSize.width: Math.min(width, manualPage.manualRasterWidth)
                            asynchronous: true
                            cache: false
                        }

                        Rectangle {
                            visible: imageItem.status === Image.Error
                            width: parent.width
                            height: 260
                            radius: 6
                            color: "#F9FBFD"
                            border.color: "#D9E6F2"
                            border.width: 1

                            Text {
                                anchors.centerIn: parent
                                text: qsTr("说明书图片加载失败")
                                font.pixelSize: 14
                                font.family: "Microsoft YaHei"
                                color: "#7A8CA5"
                            }
                        }
                    }
                }
            }
        }
    }
}
