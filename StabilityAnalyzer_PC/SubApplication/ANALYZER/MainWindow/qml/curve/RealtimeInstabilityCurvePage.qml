import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12

Rectangle {
    id: instabilityPanel
    color: "#FFFFFF"

    property var detailPage
    readonly property var experimentData: detailPage ? detailPage.experimentData : ({})
    property int currentModeIndex: 0
    property var modeTitles: [qsTr("\u6574\u4f53"), qsTr("\u5c40\u90e8"), qsTr("\u81ea\u5b9a\u4e49"), qsTr("\u603b\u89c8")]
    property var overallSeries: createEmptySeries(qsTr("\u6574\u4f53"), 0, 1)
    property var bottomSeries: createEmptySeries(qsTr("\u5e95\u90e8"), 0, 1)
    property var middleSeries: createEmptySeries(qsTr("\u4e2d\u90e8"), 0, 1)
    property var topSeries: createEmptySeries(qsTr("\u9876\u90e8"), 0, 1)
    property var customSeries: createEmptySeries(qsTr("\u81ea\u5b9a\u4e49"), 0, 1)
    property var radarPolygons: []
    property real radarMaxValue: 1
    property real customLowerBound: 0
    property real customUpperBound: 0
    property bool overallLoaded: false
    property bool localLoaded: false
    property bool customLoaded: false

    function createEmptySeries(title, lowerBound, upperBound) {
        return {
            title: title,
            rangeLabel: detailPage ? detailPage.formatNumber(lowerBound, 1) + " - " + detailPage.formatNumber(upperBound, 1) + " mm" : "",
            points: [],
            chartMinX: 0,
            chartMaxX: 1,
            chartMinY: 0,
            chartMaxY: 1,
            xAxisTickValues: [0, 1],
            yAxisLabels: detailPage ? detailPage.makeAxisLabels(0, 1, 6, 1) : [0, 1]
        }
    }

    function resetState() {
        overallLoaded = false
        localLoaded = false
        customLoaded = false
        overallSeries = createEmptySeries(qsTr("\u6574\u4f53"), detailPage ? detailPage.minHeightValue : 0, detailPage ? detailPage.maxHeightValue : 0)
        bottomSeries = createEmptySeries(qsTr("\u5e95\u90e8"), detailPage ? detailPage.minHeightValue : 0, detailPage ? detailPage.maxHeightValue : 0)
        middleSeries = createEmptySeries(qsTr("\u4e2d\u90e8"), detailPage ? detailPage.minHeightValue : 0, detailPage ? detailPage.maxHeightValue : 0)
        topSeries = createEmptySeries(qsTr("\u9876\u90e8"), detailPage ? detailPage.minHeightValue : 0, detailPage ? detailPage.maxHeightValue : 0)
        customSeries = createEmptySeries(qsTr("\u81ea\u5b9a\u4e49"), detailPage ? detailPage.minHeightValue : 0, detailPage ? detailPage.maxHeightValue : 0)
        radarPolygons = []
        radarMaxValue = 1
    }

    function activeSeriesList() {
        if (currentModeIndex === 0)
            return [overallSeries]
        if (currentModeIndex === 1)
            return [topSeries, middleSeries, bottomSeries]
        if (currentModeIndex === 2)
            return [customSeries]
        return [overallSeries, topSeries, middleSeries, bottomSeries]
    }

    function hasVisibleSeries() {
        var list = activeSeriesList()
        for (var i = 0; i < list.length; ++i) {
            if (list[i].points && list[i].points.length > 0)
                return true
        }
        return radarPolygons && radarPolygons.length > 0
    }

    function loadOverallData() {
        if (!detailPage || !realtime_ctrl || !experimentData || experimentData.id === undefined)
            return
        overallSeries = realtime_ctrl.getInstabilitySeriesChartData(Number(experimentData.id),
                                                                    detailPage.minHeightValue,
                                                                    detailPage.maxHeightValue,
                                                                    "overall",
                                                                    qsTr("\u6574\u4f53"))
        overallLoaded = true
        console.log("[RealtimeInstability][overall]",
                    "experimentId=", Number(experimentData.id),
                    "pointCount=", overallSeries && overallSeries.points ? overallSeries.points.length : 0)
    }

    function loadLocalData() {
        if (!detailPage || !realtime_ctrl || !experimentData || experimentData.id === undefined)
            return

        var totalMinHeight = detailPage.minHeightValue
        var totalMaxHeight = detailPage.maxHeightValue
        var sectionHeight = Math.max((totalMaxHeight - totalMinHeight) / 3.0, 0)
        var firstSplit = totalMinHeight + sectionHeight
        var secondSplit = totalMinHeight + sectionHeight * 2

        bottomSeries = realtime_ctrl.getInstabilitySeriesChartData(Number(experimentData.id), totalMinHeight, firstSplit, "bottom", qsTr("\u5e95\u90e8"))
        middleSeries = realtime_ctrl.getInstabilitySeriesChartData(Number(experimentData.id), firstSplit, secondSplit, "middle", qsTr("\u4e2d\u90e8"))
        topSeries = realtime_ctrl.getInstabilitySeriesChartData(Number(experimentData.id), secondSplit, totalMaxHeight, "top", qsTr("\u9876\u90e8"))
        localLoaded = true
        buildRadarOverview()
        console.log("[RealtimeInstability][local]",
                    "experimentId=", Number(experimentData.id),
                    "bottomPoints=", bottomSeries && bottomSeries.points ? bottomSeries.points.length : 0,
                    "middlePoints=", middleSeries && middleSeries.points ? middleSeries.points.length : 0,
                    "topPoints=", topSeries && topSeries.points ? topSeries.points.length : 0)
    }

    function loadCustomData() {
        if (!detailPage || !realtime_ctrl || !experimentData || experimentData.id === undefined)
            return

        customLowerBound = Math.max(detailPage.minHeightValue, Math.min(customLowerBound, detailPage.maxHeightValue))
        customUpperBound = Math.max(customLowerBound, Math.min(customUpperBound, detailPage.maxHeightValue))
        customSeries = realtime_ctrl.getInstabilitySeriesChartData(Number(experimentData.id),
                                                                   customLowerBound,
                                                                   customUpperBound,
                                                                   "custom",
                                                                   qsTr("\u81ea\u5b9a\u4e49"))
        customLoaded = true
        console.log("[RealtimeInstability][custom]",
                    "experimentId=", Number(experimentData.id),
                    "lowerMm=", customLowerBound,
                    "upperMm=", customUpperBound,
                    "pointCount=", customSeries && customSeries.points ? customSeries.points.length : 0)
    }

    function buildRadarOverview() {
        if (!detailPage || !realtime_ctrl || !experimentData || experimentData.id === undefined)
            return

        var radarData = realtime_ctrl.getInstabilityRadarChartData(Number(experimentData.id),
                                                                   detailPage.minHeightValue,
                                                                   detailPage.maxHeightValue)
        radarPolygons = radarData && radarData.polygons ? radarData.polygons : []
        radarMaxValue = radarData ? detailPage.toNumber(radarData.maxValue, 1) : 1
        console.log("[RealtimeInstability][radar]",
                    "experimentId=", Number(experimentData.id),
                    "polygonCount=", radarPolygons.length,
                    "maxValue=", radarMaxValue)
    }

    function ensureModeData() {
        if (!overallLoaded)
            loadOverallData()

        if (currentModeIndex === 1 || currentModeIndex === 3) {
            if (!localLoaded)
                loadLocalData()
        } else if (currentModeIndex === 2) {
            if (!customLoaded)
                loadCustomData()
        }

        if (currentModeIndex === 3 && radarPolygons.length === 0)
            buildRadarOverview()
    }

    function loadRealtimeData() {
        if (!detailPage || !realtime_ctrl || !experimentData || experimentData.id === undefined)
            return

        resetState()
        customLowerBound = detailPage.floorToStep(detailPage.minHeightValue, 1)
        customUpperBound = detailPage.ceilToStep(detailPage.maxHeightValue, 1)
        console.log("[RealtimeInstability][reload]",
                    "experimentId=", Number(experimentData.id),
                    "modeIndex=", currentModeIndex)
        ensureModeData()
    }

    function normalizedTextToNumber(textValue, fallback) {
        var parsed = Number(textValue)
        return isNaN(parsed) ? fallback : parsed
    }

    onDetailPageChanged: loadRealtimeData()
    onCurrentModeIndexChanged: ensureModeData()
    Component.onCompleted: loadRealtimeData()

    Connections {
        target: detailPage
        ignoreUnknownSignals: true
        onExperimentDataChanged: instabilityPanel.loadRealtimeData()
        onLightCurvesVersionChanged: instabilityPanel.loadRealtimeData()
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 18
        spacing: 16

        Row {
            spacing: 8

            Repeater {
                model: instabilityPanel.modeTitles

                delegate: Button {
                    id: instabilityModeButton
                    width: 88
                    height: 28
                    text: modelData
                    onClicked: instabilityPanel.currentModeIndex = index

                    contentItem: Text {
                        text: instabilityModeButton.text
                        font.pixelSize: 12
                        font.family: "Microsoft YaHei"
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        color: instabilityPanel.currentModeIndex === index ? "#FFFFFF" : "#4A89DC"
                    }

                    background: Rectangle {
                        color: instabilityPanel.currentModeIndex === index ? "#4A89DC" : "#FFFFFF"
                        border.color: "#4A89DC"
                        border.width: 1
                    }
                }
            }
        }

        Rectangle {
            id: customHeightArea
            visible: instabilityPanel.currentModeIndex === 2
            Layout.fillWidth: true
            Layout.preferredHeight: 48
            radius: 4
            color: "#FFFFFF"
            border.color: "#D8E4F0"
            border.width: 1

            Row {
                anchors.fill: parent
                anchors.leftMargin: 12
                anchors.rightMargin: 12
                spacing: 8

                Label {
                    anchors.verticalCenter: parent.verticalCenter
                    text: qsTr("\u9ad8\u5ea6\u4e0b\u9650:")
                    font.pixelSize: 12
                    font.family: "Microsoft YaHei"
                    color: "#2F3A4A"
                }

                TextField {
                    id: instabilityLowerField
                    anchors.verticalCenter: parent.verticalCenter
                    width: 56
                    height: 28
                    text: detailPage ? detailPage.formatNumber(instabilityPanel.customLowerBound, 0) : "0"
                    horizontalAlignment: Text.AlignHCenter
                    font.pixelSize: 12
                    font.family: "Microsoft YaHei"
                    validator: IntValidator {
                        bottom: detailPage ? Math.floor(detailPage.minHeightValue) : 0
                        top: detailPage ? Math.ceil(detailPage.maxHeightValue) : 100
                    }
                }

                Label {
                    anchors.verticalCenter: parent.verticalCenter
                    text: qsTr("\u9ad8\u5ea6\u4e0a\u9650:")
                    font.pixelSize: 12
                    font.family: "Microsoft YaHei"
                    color: "#2F3A4A"
                }

                TextField {
                    id: instabilityUpperField
                    anchors.verticalCenter: parent.verticalCenter
                    width: 56
                    height: 28
                    text: detailPage ? detailPage.formatNumber(instabilityPanel.customUpperBound, 0) : "0"
                    horizontalAlignment: Text.AlignHCenter
                    font.pixelSize: 12
                    font.family: "Microsoft YaHei"
                    validator: IntValidator {
                        bottom: detailPage ? Math.floor(detailPage.minHeightValue) : 0
                        top: detailPage ? Math.ceil(detailPage.maxHeightValue) : 100
                    }
                }

                Label {
                    anchors.verticalCenter: parent.verticalCenter
                    text: "mm"
                    font.pixelSize: 12
                    font.family: "Microsoft YaHei"
                    color: "#2F3A4A"
                }

                Button {
                    anchors.verticalCenter: parent.verticalCenter
                    width: 80
                    height: 28
                    text: qsTr("\u66f4\u65b0")
                    onClicked: {
                        instabilityPanel.customLowerBound = instabilityPanel.normalizedTextToNumber(instabilityLowerField.text, instabilityPanel.customLowerBound)
                        instabilityPanel.customUpperBound = instabilityPanel.normalizedTextToNumber(instabilityUpperField.text, instabilityPanel.customUpperBound)
                        if (instabilityPanel.customUpperBound < instabilityPanel.customLowerBound) {
                            var tempHeight = instabilityPanel.customLowerBound
                            instabilityPanel.customLowerBound = instabilityPanel.customUpperBound
                            instabilityPanel.customUpperBound = tempHeight
                        }
                        instabilityPanel.customLoaded = false
                        instabilityPanel.loadCustomData()
                        instabilityLowerField.text = detailPage.formatNumber(instabilityPanel.customLowerBound, 0)
                        instabilityUpperField.text = detailPage.formatNumber(instabilityPanel.customUpperBound, 0)
                    }

                    contentItem: Text {
                        text: parent.text
                        font.pixelSize: 12
                        font.family: "Microsoft YaHei"
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        color: "#FFFFFF"
                    }

                    background: Rectangle {
                        color: "#4A89DC"
                        radius: 4
                    }
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
                visible: !instabilityPanel.hasVisibleSeries()
                text: qsTr("\u6682\u65e0\u5b9e\u65f6\u4e0d\u7a33\u5b9a\u6027\u6570\u636e")
                font.family: "Microsoft YaHei"
                color: "#7A8CA5"
            }

            Item {
                anchors.fill: parent
                anchors.margins: 12
                visible: instabilityPanel.hasVisibleSeries()

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 12
                    visible: instabilityPanel.currentModeIndex !== 3

                    Repeater {
                        model: instabilityPanel.activeSeriesList()

                        delegate: Rectangle {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            Layout.preferredHeight: instabilityPanel.currentModeIndex === 1 ? 150 : 220
                            radius: 6
                            color: "#FFFFFF"
                            border.color: "#DCE6F2"
                            border.width: 1

                            Text {
                                anchors.left: parent.left
                                anchors.top: parent.top
                                anchors.leftMargin: 16
                                anchors.topMargin: 10
                                text: modelData.title + "  " + modelData.rangeLabel
                                font.pixelSize: 12
                                font.family: "Microsoft YaHei"
                                color: "#4A5D75"
                                font.bold: true
                            }

                            TrendChart {
                                anchors.fill: parent
                                anchors.margins: 10
                                anchors.topMargin: 34
                                leftMargin: 72
                                rightMargin: 22
                                topMargin: 18
                                yAxisTitleOffset: 64
                                dataPoints: modelData.points
                                lineColor: modelData.title === qsTr("\u5e95\u90e8") ? "#2F7CF6"
                                          : modelData.title === qsTr("\u4e2d\u90e8") ? "#21A366"
                                          : modelData.title === qsTr("\u9876\u90e8") ? "#F28C28"
                                          : "#2F7CF6"
                                minXValue: modelData.chartMinX
                                maxXValue: modelData.chartMaxX
                                minYValue: modelData.chartMinY
                                maxYValue: modelData.chartMaxY
                                xAxisTickValues: modelData.xAxisTickValues
                                yAxisLabels: modelData.yAxisLabels
                                yAxisTitle: "Ius"
                                xAxisTitle: qsTr("\u65f6\u95f4(min)")
                                formatXLabel: function(value) {
                                    return detailPage ? detailPage.formatNumber(value, 1) : Number(value).toFixed(1)
                                }
                            }
                        }
                    }
                }

                Item {
                    anchors.fill: parent
                    visible: instabilityPanel.currentModeIndex === 3

                    InstabilityRadarChart {
                        anchors.fill: parent
                        polygons: instabilityPanel.radarPolygons
                        maxValue: instabilityPanel.radarMaxValue
                        tickCount: 6
                        axisLabels: [qsTr("Ius - \u6574\u4f53"), qsTr("Ius - \u5e95\u90e8"), qsTr("Ius - \u4e2d\u90e8"), qsTr("Ius - \u9876\u90e8")]
                    }
                }
            }
        }
    }
}
