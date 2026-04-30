import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12

import "component"

Item {
    id: comparePage
    objectName: "ExperimentComparePage"

    property var experimentDataList: []
    property int currentTabIndex: 0

    signal backRequested()

    readonly property var compareTabs: [
        { title: qsTr("不稳定性") },
        { title: qsTr("均匀度") },
        { title: qsTr("光强平均值") }
    ]

    readonly property var experimentIds: {
        var ids = []
        for (var i = 0; i < experimentDataList.length; ++i) {
            if (experimentDataList[i] && experimentDataList[i].id !== undefined)
                ids.push(Number(experimentDataList[i].id))
        }
        return ids
    }

    readonly property int experimentCount: experimentIds.length

    readonly property real globalMinHeightValue: {
        var minValue = Number.POSITIVE_INFINITY
        for (var i = 0; i < experimentDataList.length; ++i) {
            var range = experimentHeightRange(experimentDataList[i])
            minValue = Math.min(minValue, range.minValue)
        }
        return isFinite(minValue) ? minValue : 0
    }

    readonly property real globalMaxHeightValue: {
        var maxValue = Number.NEGATIVE_INFINITY
        for (var i = 0; i < experimentDataList.length; ++i) {
            var range = experimentHeightRange(experimentDataList[i])
            maxValue = Math.max(maxValue, range.maxValue)
        }
        return isFinite(maxValue) ? maxValue : 55
    }

    function toNumber(value, fallback) {
        var parsed = Number(value)
        return isNaN(parsed) ? fallback : parsed
    }

    function formatNumber(value, digits) {
        var parsed = Number(value)
        if (isNaN(parsed))
            return "--"
        return parsed.toFixed(digits === undefined ? 1 : digits)
    }

    function experimentHeightRange(experimentData) {
        var startMm = toNumber(experimentData && experimentData.scan_range_start, 0)
        var endMm = toNumber(experimentData && experimentData.scan_range_end, 55)
        var stepUm = toNumber(experimentData && experimentData.scan_step, 20)
        var effectiveEndMm = endMm
        if (stepUm > 0 && endMm > startMm)
            effectiveEndMm = endMm - stepUm / 1000.0

        var minValue = Math.min(startMm, endMm)
        var maxValue = Math.max(startMm, effectiveEndMm)
        if (maxValue <= minValue)
            maxValue = minValue + 10
        return {
            minValue: minValue,
            maxValue: maxValue
        }
    }

    function currentTabSource() {
        if (currentTabIndex === 1)
            return "compare/UniformityComparePage.qml"
        if (currentTabIndex === 2)
            return "compare/LightIntensityAverageComparePage.qml"
        return "compare/InstabilityComparePage.qml"
    }

    function initializeCompareTabItem(tabItem) {
        if (!tabItem)
            return

        try {
            tabItem.comparePage = comparePage
            console.log("[ComparePage][tab init]",
                        "tabIndex=", currentTabIndex,
                        "source=", currentTabSource(),
                        "experimentIds=", experimentIds)
        } catch (error) {
            console.log("[ComparePage][tab init] assign comparePage failed",
                        "tabIndex=", currentTabIndex,
                        "source=", currentTabSource(),
                        "error=", error)
        }
    }

    onCurrentTabIndexChanged: {
        console.log("[ComparePage][tab changed]",
                    "tabIndex=", currentTabIndex,
                    "source=", currentTabSource(),
                    "experimentIds=", experimentIds)
    }
    Component.onCompleted: {
        console.log("[ComparePage][completed]",
                    "tabIndex=", currentTabIndex,
                    "source=", currentTabSource(),
                    "experimentIds=", experimentIds)
    }

    Rectangle {
        anchors.fill: parent
        color: "#FFFFFF"

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 16
            spacing: 14

            RowLayout {
                Layout.fillWidth: true
                spacing: 12

                Text {
                    text: qsTr("数据对比")
                    font.pixelSize: 18
                    font.family: "Microsoft YaHei"
                    font.bold: true
                    color: "#2F3A4A"
                }

                Text {
                    text: comparePage.experimentCount > 0
                          ? qsTr("已选 %1 个实验").arg(comparePage.experimentCount)
                          : qsTr("未选择实验")
                    font.pixelSize: 12
                    font.family: "Microsoft YaHei"
                    color: "#7A8CA5"
                }

                Item { Layout.fillWidth: true }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 8

                Repeater {
                    model: comparePage.compareTabs

                    delegate: Button {
                        id: tabButton
                        width: 108
                        height: 30
                        text: modelData.title
                        onClicked: comparePage.currentTabIndex = index

                        contentItem: Text {
                            text: tabButton.text
                            font.pixelSize: 12
                            font.family: "Microsoft YaHei"
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                            color: comparePage.currentTabIndex === index ? "#FFFFFF" : "#4A89DC"
                        }

                        background: Rectangle {
                            color: comparePage.currentTabIndex === index ? "#4A89DC" : "#FFFFFF"
                            border.color: "#4A89DC"
                            border.width: 1
                        }
                    }
                }

                Item { Layout.fillWidth: true }

                IconButton {
                    id: backButton
                    Layout.preferredWidth: 100
                    Layout.preferredHeight: 28
                    Layout.rightMargin: 20
                    Layout.alignment: Qt.AlignVCenter
                    text: qsTr("返回记录列表")
                    onClicked: comparePage.backRequested()

                    contentItem: Text {
                        text: backButton.text
                        color: "#2F7CF6"
                        font.pixelSize: 13
                        font.family: "Microsoft YaHei"
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }

                    background: Rectangle {
                        radius: 4
                        color: "#EEF5FF"
                        border.color: "#C9DBF8"
                        border.width: 1
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                radius: 6
                color: "#F8FBFF"
                border.color: "#D8E4F0"
                border.width: 1

                Text {
                    anchors.centerIn: parent
                    visible: comparePage.experimentCount < 2
                    text: qsTr("请至少选择两个实验后再进行对比")
                    font.pixelSize: 15
                    font.family: "Microsoft YaHei"
                    color: "#7A8CA5"
                }

                Loader {
                    id: compareTabLoader
                    anchors.fill: parent
                    anchors.margins: 12
                    visible: comparePage.experimentCount >= 2
                    source: comparePage.currentTabSource()
                    onLoaded: comparePage.initializeCompareTabItem(item)
                    onStatusChanged: {
                        console.log("[ComparePage][loader status]",
                                    "status=", status,
                                    "source=", source,
                                    "visible=", visible)
                    }
                }
            }
        }
    }
}
