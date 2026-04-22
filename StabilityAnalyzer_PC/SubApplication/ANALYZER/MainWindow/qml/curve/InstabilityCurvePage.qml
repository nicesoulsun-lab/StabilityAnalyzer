import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import "../component"
import ".."


//不稳定性曲线
Rectangle {
    id: instabilityPanel

    // 不稳定性页把最重的计算移回 C++/数据库层，
    // QML 侧只负责按模式懒加载结果和组织展示。
    property var detailPage
    readonly property var experimentData: detailPage ? detailPage.experimentData : ({})
    property int currentModeIndex: 0
    property var modeTitles: [qsTr("整体"), qsTr("局部"), qsTr("自定义"), qsTr("总览")]
    property var overallSeries: createEmptyInstabilitySeries(qsTr("整体"), detailPage ? detailPage.minHeightValue : 0, detailPage ? detailPage.maxHeightValue : 0)
    property var bottomSeries: createEmptyInstabilitySeries(qsTr("底部"), detailPage ? detailPage.minHeightValue : 0, detailPage ? detailPage.minHeightValue : 0)
    property var middleSeries: createEmptyInstabilitySeries(qsTr("中部"), detailPage ? detailPage.minHeightValue : 0, detailPage ? detailPage.minHeightValue : 0)
    property var topSeries: createEmptyInstabilitySeries(qsTr("顶部"), detailPage ? detailPage.minHeightValue : 0, detailPage ? detailPage.maxHeightValue : 0)
    property var customSeries: createEmptyInstabilitySeries(qsTr("自定义"), detailPage ? detailPage.minHeightValue : 0, detailPage ? detailPage.maxHeightValue : 0)
    property var radarPolygons: []
    property real radarMaxValue: 1
    property real customLowerBound: 0
    property real customUpperBound: 0
    property bool overallLoaded: false
    property bool localLoaded: false
    property bool customLoaded: false

    color: "#FFFFFF"

    function createEmptyInstabilitySeries(title, lowerBound, upperBound) {
        // 所有模式先使用同一种空结构，避免界面初次进入时反复判空。
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

    function buildInstabilitySeriesFromRows(title, rows, lowerBound, upperBound) {
        // 后端已经给出每次扫描的结果，这里只做排序后的折线点和坐标轴整理。
        var safeLower = Math.max(detailPage.minHeightValue, Math.min(lowerBound, upperBound))
        var safeUpper = Math.min(detailPage.maxHeightValue, Math.max(lowerBound, upperBound))
        var emptySeries = createEmptyInstabilitySeries(title, safeLower, safeUpper)
        if (!rows || rows.length === 0 || safeUpper <= safeLower)
            return emptySeries

        var localPoints = []
        var maxY = 0
        for (var i = 0; i < rows.length; ++i) {
            var xValue = detailPage.toNumber(rows[i].scan_elapsed_ms, 0) / 60000.0
            var instabilityValue = detailPage.toNumber(rows[i].instability_value, 0)
            localPoints.push({ x: xValue, y: instabilityValue })
            maxY = Math.max(maxY, instabilityValue)
        }

        emptySeries.points = localPoints
        emptySeries.chartMinX = localPoints.length > 0 ? localPoints[0].x : 0
        emptySeries.chartMaxX = localPoints.length > 1 ? localPoints[localPoints.length - 1].x : emptySeries.chartMinX + 1
        if (emptySeries.chartMaxX <= emptySeries.chartMinX)
            emptySeries.chartMaxX = emptySeries.chartMinX + 1
        emptySeries.chartMinY = 0
        emptySeries.chartMaxY = Math.max(1, maxY * 1.12)
        emptySeries.xAxisTickValues = detailPage.buildTimeTicks(emptySeries.chartMinX, emptySeries.chartMaxX, 6)
        emptySeries.yAxisLabels = detailPage.makeAxisLabels(emptySeries.chartMinY, emptySeries.chartMaxY, 6, 1)
        return emptySeries
    }

    function loadOverallData() {
        if (!detailPage || !experimentData || experimentData.id === undefined || !data_ctrl)
            return

        var rows = data_ctrl.getInstabilityCurveData(Number(experimentData.id))
        overallSeries = buildInstabilitySeriesFromRows(qsTr("整体"), rows, detailPage.minHeightValue, detailPage.maxHeightValue)
        overallLoaded = true
    }

    function loadLocalData() {
        // 局部模式固定按底/中/顶三段切分，结果会被后端按区间缓存。
        if (!detailPage || !experimentData || experimentData.id === undefined || !data_ctrl || localLoaded)
            return

        var totalMinHeight = detailPage.minHeightValue
        var totalMaxHeight = detailPage.maxHeightValue
        var sectionHeight = Math.max((totalMaxHeight - totalMinHeight) / 3.0, 0)
        var firstSplit = totalMinHeight + sectionHeight
        var secondSplit = totalMinHeight + sectionHeight * 2

        var bottomRows = data_ctrl.getInstabilityCurveDataByHeightRange(Number(experimentData.id), totalMinHeight, firstSplit, "bottom")
        var middleRows = data_ctrl.getInstabilityCurveDataByHeightRange(Number(experimentData.id), firstSplit, secondSplit, "middle")
        var topRows = data_ctrl.getInstabilityCurveDataByHeightRange(Number(experimentData.id), secondSplit, totalMaxHeight, "top")

        bottomSeries = buildInstabilitySeriesFromRows(qsTr("底部"), bottomRows, totalMinHeight, firstSplit)
        middleSeries = buildInstabilitySeriesFromRows(qsTr("中部"), middleRows, firstSplit, secondSplit)
        topSeries = buildInstabilitySeriesFromRows(qsTr("顶部"), topRows, secondSplit, totalMaxHeight)
        localLoaded = true
        buildRadarOverview()
    }

    function loadCustomData() {
        // 自定义模式只有点击“应用”后才重新取数，避免输入框编辑时频繁触发计算。
        if (!detailPage || !experimentData || experimentData.id === undefined || !data_ctrl)
            return

        customLowerBound = Math.max(detailPage.minHeightValue, Math.min(customLowerBound, detailPage.maxHeightValue))
        customUpperBound = Math.max(detailPage.minHeightValue, Math.min(customUpperBound, detailPage.maxHeightValue))
        var rows = data_ctrl.getInstabilityCurveDataByHeightRange(Number(experimentData.id), customLowerBound, customUpperBound, "custom")
        customSeries = buildInstabilitySeriesFromRows(qsTr("自定义"), rows, customLowerBound, customUpperBound)
        customLoaded = true
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
        var seriesList = activeSeriesList()
        for (var i = 0; i < seriesList.length; ++i) {
            if (seriesList[i].points && seriesList[i].points.length > 0)
                return true
        }
        return false
    }

    function radarColor(index, total) {
        if (total <= 1)
            return Qt.hsla(0.65, 0.9, 0.45, 0.95)
        var ratio = index / Math.max(total - 1, 1)
        return Qt.hsla(0.65 * (1.0 - ratio), 0.9, 0.48, 0.95)
    }

    function buildRadarOverview() {
        // 雷达图的每一层对应同一个时间点在四个区间上的 Ius 值，
        // 因此这里按时间索引把整体/底部/中部/顶部拼成一组 polygon。
        radarPolygons = []
        radarMaxValue = 1

        var pointCount = Math.min(overallSeries.points ? overallSeries.points.length : 0,
                                  bottomSeries.points ? bottomSeries.points.length : 0,
                                  middleSeries.points ? middleSeries.points.length : 0,
                                  topSeries.points ? topSeries.points.length : 0)
        if (pointCount <= 0)
            return

        var polygons = []
        var maxValue = 0
        for (var i = 0; i < pointCount; ++i) {
            var values = [
                detailPage.toNumber(overallSeries.points[i].y, 0),
                detailPage.toNumber(bottomSeries.points[i].y, 0),
                detailPage.toNumber(middleSeries.points[i].y, 0),
                detailPage.toNumber(topSeries.points[i].y, 0)
            ]
            maxValue = Math.max(maxValue, values[0], values[1], values[2], values[3])
            polygons.push({
                values: values,
                label: detailPage.formatElapsedTime(detailPage.toNumber(overallSeries.points[i].x, 0) * 60000),
                color: radarColor(i, pointCount)
            })
        }

        radarPolygons = polygons
        radarMaxValue = Math.max(1, Math.ceil(maxValue))
    }

    function applyCustomRange() {
        loadCustomData()
    }

    function ensureModeData() {
        // 首次进入页面只加载整体；
        // 其他模式在真正切换过去时再补数据，避免进页卡顿。
        if (!overallLoaded)
            loadOverallData()

        if (currentModeIndex === 1 || currentModeIndex === 3)
            loadLocalData()
        else if (currentModeIndex === 2 && !customLoaded)
            loadCustomData()

        if (currentModeIndex === 3 && radarPolygons.length === 0)
            buildRadarOverview()
    }

    function loadInstabilityData() {
        if (!detailPage)
            return

        overallLoaded = false
        localLoaded = false
        customLoaded = false
        overallSeries = createEmptyInstabilitySeries(qsTr("整体"), detailPage.minHeightValue, detailPage.maxHeightValue)
        bottomSeries = createEmptyInstabilitySeries(qsTr("底部"), detailPage.minHeightValue, detailPage.minHeightValue)
        middleSeries = createEmptyInstabilitySeries(qsTr("中部"), detailPage.minHeightValue, detailPage.minHeightValue)
        topSeries = createEmptyInstabilitySeries(qsTr("顶部"), detailPage.maxHeightValue, detailPage.maxHeightValue)
        customSeries = createEmptyInstabilitySeries(qsTr("自定义"), detailPage.minHeightValue, detailPage.maxHeightValue)
        radarPolygons = []
        radarMaxValue = 1

        customLowerBound = detailPage.floorToStep(detailPage.minHeightValue, 1)
        customUpperBound = detailPage.ceilToStep(detailPage.maxHeightValue, 1)
        loadOverallData()
    }

    function normalizedTextToNumber(textValue, fallback) {
        var parsed = Number(textValue)
        return isNaN(parsed) ? fallback : parsed
    }

    onDetailPageChanged: loadInstabilityData()
    onCurrentModeIndexChanged: ensureModeData()
    Component.onCompleted: {
        if (detailPage)
            loadInstabilityData()
    }

    Connections {
        target: detailPage
        function onExperimentDataChanged() { instabilityPanel.loadInstabilityData() }
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
                    text: qsTr("高度下限:")
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
                    text: qsTr("高度上限:")
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

                IconButton {
                    anchors.verticalCenter: parent.verticalCenter
                    width: 80
                    height: 28
                    button_text: qsTr("应用")
                    button_color: "#4A89DC"
                    text_color: "#FFFFFF"
                    pixelSize: 12
                    family: "Microsoft YaHei"
                    button_radius: 4
                    onClicked: {
                        instabilityPanel.customLowerBound = instabilityPanel.normalizedTextToNumber(instabilityLowerField.text, instabilityPanel.customLowerBound)
                        instabilityPanel.customUpperBound = instabilityPanel.normalizedTextToNumber(instabilityUpperField.text, instabilityPanel.customUpperBound)
                        if (instabilityPanel.customUpperBound < instabilityPanel.customLowerBound) {
                            var tempHeight = instabilityPanel.customLowerBound
                            instabilityPanel.customLowerBound = instabilityPanel.customUpperBound
                            instabilityPanel.customUpperBound = tempHeight
                        }
                        instabilityPanel.applyCustomRange()
                        instabilityLowerField.text = detailPage.formatNumber(instabilityPanel.customLowerBound, 0)
                        instabilityUpperField.text = detailPage.formatNumber(instabilityPanel.customUpperBound, 0)
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
                text: qsTr("数据库中暂无该实验的不稳定性曲线数据")
                font.pixelSize: 15
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
                                lineColor: modelData.title === qsTr("底部") ? "#2F7CF6"
                                          : modelData.title === qsTr("中部") ? "#21A366"
                                          : modelData.title === qsTr("顶部") ? "#F28C28"
                                          : "#2F7CF6"
                                minXValue: modelData.chartMinX
                                maxXValue: modelData.chartMaxX
                                minYValue: modelData.chartMinY
                                maxYValue: modelData.chartMaxY
                                xAxisTickValues: modelData.xAxisTickValues
                                yAxisLabels: modelData.yAxisLabels
                                yAxisTitle: "Ius"
                                xAxisTitle: qsTr("时间(min)")
                                formatXLabel: function(value) { return detailPage.formatNumber(value, 1) }
                            }
                        }
                    }
                }

                Item {
                    anchors.fill: parent
                    visible: instabilityPanel.currentModeIndex === 3

                    InstabilityRadarChart {
                        // 总览模式使用雷达图展示同一时间点在四个高度区间的横向对比。
                        anchors.fill: parent
                        polygons: instabilityPanel.radarPolygons
                        maxValue: instabilityPanel.radarMaxValue
                        tickCount: 6
                        axisLabels: [qsTr("Ius - 整体"), qsTr("Ius - 底部"), qsTr("Ius - 中部"), qsTr("Ius - 顶部")]
                    }
                }
            }
        }
    }
}
