import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12

Rectangle {
    id: root

    property var seriesList: []
    property string title: qsTr("\u66f2\u7ebf\u989c\u8272\u5bf9\u7167")

    function seriesCount() {
        return seriesList && seriesList.length ? seriesList.length : 0
    }

    function seriesAt(index) {
        if (!seriesList || index < 0 || index >= seriesList.length)
            return null
        return seriesList[index]
    }

    radius: 6
    color: "#FFFFFF"
    border.color: "#DCE6F2"
    border.width: 1

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 10

        Text {
            Layout.fillWidth: true
            text: root.title
            font.pixelSize: 13
            font.family: "Microsoft YaHei"
            font.bold: true
            color: "#2F3A4A"
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            color: "#EEF3F8"
        }

        Flickable {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            contentWidth: width
            contentHeight: legendColumn.height

            Column {
                id: legendColumn
                width: parent.width
                spacing: 12

                Repeater {
                    model: root.seriesCount()

                    delegate: Row {
                        property var seriesData: root.seriesAt(index)
                        width: parent.width
                        spacing: 10

                        Item {
                            width: 34
                            height: 18

                            Rectangle {
                                anchors.verticalCenter: parent.verticalCenter
                                anchors.left: parent.left
                                width: 26
                                height: 3
                                radius: 2
                                color: seriesData && seriesData.color ? seriesData.color : "#2F7CF6"
                            }

                            Rectangle {
                                anchors.verticalCenter: parent.verticalCenter
                                anchors.right: parent.right
                                width: 8
                                height: 8
                                radius: 4
                                color: seriesData && seriesData.color ? seriesData.color : "#2F7CF6"
                            }
                        }

                        Column {
                            width: parent.width - 44
                            spacing: 2

                            Text {
                                width: parent.width
                                text: seriesData && seriesData.label
                                      ? seriesData.label
                                      : qsTr("\u672a\u547d\u540d\u5b9e\u9a8c")
                                font.pixelSize: 12
                                font.family: "Microsoft YaHei"
                                color: "#2F3A4A"
                                wrapMode: Text.WrapAnywhere
                            }

                            Text {
                                width: parent.width
                                text: seriesData && seriesData.has_data
                                      ? qsTr("\u66f2\u7ebf\u70b9\u6570: ") + String(seriesData.point_count || 0)
                                      : qsTr("\u5f53\u524d\u5b9e\u9a8c\u6682\u65e0\u53ef\u5bf9\u6bd4\u6570\u636e")
                                font.pixelSize: 11
                                font.family: "Microsoft YaHei"
                                color: seriesData && seriesData.has_data ? "#7A8CA5" : "#C45858"
                                wrapMode: Text.WrapAnywhere
                            }
                        }
                    }
                }
            }

            ScrollBar.vertical: ScrollBar {
                width: 10
            }
        }

        Text {
            Layout.fillWidth: true
            visible: !root.seriesList || root.seriesList.length === 0
            text: qsTr("\u6682\u65e0\u5b9e\u9a8c")
            font.pixelSize: 12
            font.family: "Microsoft YaHei"
            color: "#97A3B2"
            horizontalAlignment: Text.AlignHCenter
        }
    }
}
