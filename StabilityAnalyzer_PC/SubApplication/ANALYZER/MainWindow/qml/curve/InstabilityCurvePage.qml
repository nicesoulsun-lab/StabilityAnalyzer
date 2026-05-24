import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import "../component"
import ".."


//不稳定性曲线
Rectangle {
    id: instabilityPanel

    // 不稳定性页把最重的计算移回 C++/数据库层，
    // QML 侧只负责按模式懒加载结果和组装展示。
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
    property bool overviewLoading: false
    property bool customLoading: false
    property int overviewRequestId: 0
    property int customRequestId: 0
    readonly property bool reportDataReady: !overviewLoading && !customLoading && hasVisibleSeries()

    color: "#FFFFFF"

    function createEmptyInstabilitySeries(title, lowerBound, upperBound) {
        // 所有模式先使用同一种空结构，避免界面初次进入时反复清空。
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


    function resetInstabilityState() {
        overallLoaded = false
        localLoaded = false
        customLoaded = false
        overviewLoading = false
        customLoading = false
        overviewRequestId = 0
        customRequestId = 0
        overallSeries = createEmptyInstabilitySeries(qsTr("整体"), detailPage ? detailPage.minHeightValue : 0, detailPage ? detailPage.maxHeightValue : 0)
        bottomSeries = createEmptyInstabilitySeries(qsTr("底部"), detailPage ? detailPage.minHeightValue : 0, detailPage ? detailPage.minHeightValue : 0)
        middleSeries = createEmptyInstabilitySeries(qsTr("中部"), detailPage ? detailPage.minHeightValue : 0, detailPage ? detailPage.minHeightValue : 0)
        topSeries = createEmptyInstabilitySeries(qsTr("顶部"), detailPage ? detailPage.maxHeightValue : 0, detailPage ? detailPage.maxHeightValue : 0)
        customSeries = createEmptyInstabilitySeries(qsTr("自定义"), detailPage ? detailPage.minHeightValue : 0, detailPage ? detailPage.maxHeightValue : 0)
        radarPolygons = []
        radarMaxValue = 1
    }

    function cancelDetailRequests(reason) {
        if (!detail_ctrl)
            return

        if (overviewRequestId > 0)
            detail_ctrl.cancelInstabilityRequest(overviewRequestId)
        if (customRequestId > 0)
            detail_ctrl.cancelInstabilityRequest(customRequestId)

        if (overviewRequestId > 0 || customRequestId > 0) {
            console.log("[DetailInstability][release]",
                        "experimentId=", experimentData && experimentData.id !== undefined ? Number(experimentData.id) : 0,
                        "reason=", reason,
                        "overviewRequestId=", overviewRequestId,
                        "customRequestId=", customRequestId)
        }

        overviewRequestId = 0
        customRequestId = 0
        overviewLoading = false
        customLoading = false
    }

    function applyOverviewPayload(payload) {
        overallSeries = payload && payload.overallSeries ? payload.overallSeries : createEmptyInstabilitySeries(qsTr("整体"), detailPage.minHeightValue, detailPage.maxHeightValue)
        bottomSeries = payload && payload.bottomSeries ? payload.bottomSeries : createEmptyInstabilitySeries(qsTr("底部"), detailPage.minHeightValue, detailPage.minHeightValue)
        middleSeries = payload && payload.middleSeries ? payload.middleSeries : createEmptyInstabilitySeries(qsTr("中部"), detailPage.minHeightValue, detailPage.minHeightValue)
        topSeries = payload && payload.topSeries ? payload.topSeries : createEmptyInstabilitySeries(qsTr("顶部"), detailPage.maxHeightValue, detailPage.maxHeightValue)

        var radarData = payload && payload.radarData ? payload.radarData : ({})
        radarPolygons = radarData.polygons || []
        radarMaxValue = detailPage ? detailPage.toNumber(radarData.maxValue, 1) : 1

        overviewLoading = false
        overallLoaded = true
        localLoaded = true

        console.log("[DetailInstability][overview ready]",
                    "experimentId=", Number(experimentData.id),
                    "overallPoints=", overallSeries && overallSeries.points ? overallSeries.points.length : 0,
                    "bottomPoints=", bottomSeries && bottomSeries.points ? bottomSeries.points.length : 0,
                    "middlePoints=", middleSeries && middleSeries.points ? middleSeries.points.length : 0,
                    "topPoints=", topSeries && topSeries.points ? topSeries.points.length : 0,
                    "radarPolygons=", radarPolygons.length)
    }

    function applyCustomPayload(payload) {
        customSeries = payload && payload.series ? payload.series : createEmptyInstabilitySeries(qsTr("自定义"), customLowerBound, customUpperBound)
        customLoading = false
        customLoaded = true

        console.log("[DetailInstability][custom ready]",
                    "experimentId=", Number(experimentData.id),
                    "lowerMm=", customLowerBound,
                    "upperMm=", customUpperBound,
                    "pointCount=", customSeries && customSeries.points ? customSeries.points.length : 0)
    }

    function requestOverviewData() {
        if (!detailPage || !experimentData || experimentData.id === undefined || !detail_ctrl)
            return

        overviewLoading = true
        overviewRequestId = detail_ctrl.requestInstabilityOverview(Number(experimentData.id),
                                                                  detailPage.minHeightValue,
                                                                  detailPage.maxHeightValue)
        console.log("[DetailInstability][overview request]",
                    "experimentId=", Number(experimentData.id),
                    "requestId=", overviewRequestId,
                    "minHeightMm=", detailPage.minHeightValue,
                    "maxHeightMm=", detailPage.maxHeightValue)
        if (overviewRequestId <= 0)
            overviewLoading = false
    }

    function requestCustomData() {
        if (!detailPage || !experimentData || experimentData.id === undefined || !detail_ctrl)
            return

        customLowerBound = Math.max(detailPage.minHeightValue, Math.min(customLowerBound, detailPage.maxHeightValue))
        customUpperBound = Math.max(detailPage.minHeightValue, Math.min(customUpperBound, detailPage.maxHeightValue))

        customLoading = true
        customLoaded = false
        customRequestId = detail_ctrl.requestInstabilityCustomSeries(Number(experimentData.id),
                                                                     customLowerBound,
                                                                     customUpperBound)
        console.log("[DetailInstability][custom request]",
                    "experimentId=", Number(experimentData.id),
                    "requestId=", customRequestId,
                    "lowerMm=", customLowerBound,
                    "upperMm=", customUpperBound)
        if (customRequestId <= 0)
            customLoading = false
    }

    function loadOverallData() {
        if (!detailPage || !experimentData || experimentData.id === undefined || !detail_ctrl)
            return

        if (!overallLoaded && !overviewLoading)
            requestOverviewData()
    }

    function loadLocalData() {
        // 局部模式固定按底部/中部/顶部三段切分，结果会被后端按区间缓存。
        if (!detailPage || !experimentData || experimentData.id === undefined || localLoaded)
            return

        if (!overviewLoading && !overallLoaded)
            requestOverviewData()
    }
    function loadCustomData() {
        // 自定义模式只有点击"应用"后才重新取数，避免输入框编辑时频繁触发计算。
        if (!detailPage || !experimentData || experimentData.id === undefined || !detail_ctrl)
            return

        requestCustomData()
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

    function buildRadarOverview() {
        // 雷达图的每一层对应同一个时间点在四个区间上的 Ius 值，
        // 因此这里按时间索引把整体/底部/中部/顶部拼成一组 polygon。
        radarPolygons = []
        radarMaxValue = 1
        return
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
        else if (currentModeIndex === 2 && !customLoaded && !customLoading)
            loadCustomData()

        if (currentModeIndex === 3 && radarPolygons.length === 0)
            radarPolygons = []
    }

    function loadInstabilityData() {
        if (!detailPage)
            return

        cancelDetailRequests("reload")
        resetInstabilityState()
        customLowerBound = detailPage.floorToStep(detailPage.minHeightValue, 1)
        customUpperBound = detailPage.ceilToStep(detailPage.maxHeightValue, 1)

        requestOverviewData()
    }

    function prepareReportMode() {
        currentModeIndex = 0
        ensureModeData()
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
    Component.onDestruction: cancelDetailRequests("destroyed")

    Connections {
        target: detailPage
        onExperimentDataChanged: instabilityPanel.loadInstabilityData()
    }

    Connections {
        target: detail_ctrl
        ignoreUnknownSignals: true
        onInstabilityOverviewRequestFinished: {
            console.log("[DetailInstability][overview finished signal]",
                        "requestId=", requestId,
                        "activeRequestId=", instabilityPanel.overviewRequestId,
                        "experimentId=", experimentId,
                        "activeExperimentId=", instabilityPanel.experimentData && instabilityPanel.experimentData.id !== undefined ? Number(instabilityPanel.experimentData.id) : 0)
            if (requestId !== instabilityPanel.overviewRequestId)
                return
            if (!instabilityPanel.experimentData || Number(instabilityPanel.experimentData.id) !== Number(experimentId))
                return

            instabilityPanel.overviewRequestId = 0
            instabilityPanel.applyOverviewPayload(payload)
        }
        onInstabilityOverviewRequestFailed: {
            if (requestId !== instabilityPanel.overviewRequestId)
                return
            if (!instabilityPanel.experimentData || Number(instabilityPanel.experimentData.id) !== Number(experimentId))
                return

            instabilityPanel.overviewRequestId = 0
            instabilityPanel.overviewLoading = false
            console.log("[DetailInstability][overview failed]",
                        "experimentId=", experimentId,
                        "message=", message)
        }
        onInstabilityOverviewRequestCancelled: {
            if (requestId !== instabilityPanel.overviewRequestId)
                return

            instabilityPanel.overviewRequestId = 0
            instabilityPanel.overviewLoading = false
            console.log("[DetailInstability][overview cancelled]",
                        "experimentId=", experimentId,
                        "reason=", reason)
        }
        onInstabilityCustomSeriesRequestFinished: {
            console.log("[DetailInstability][custom finished signal]",
                        "requestId=", requestId,
                        "activeRequestId=", instabilityPanel.customRequestId,
                        "experimentId=", experimentId,
                        "activeExperimentId=", instabilityPanel.experimentData && instabilityPanel.experimentData.id !== undefined ? Number(instabilityPanel.experimentData.id) : 0)
            if (requestId !== instabilityPanel.customRequestId)
                return
            if (!instabilityPanel.experimentData || Number(instabilityPanel.experimentData.id) !== Number(experimentId))
                return

            instabilityPanel.customRequestId = 0
            instabilityPanel.applyCustomPayload(payload)
        }
        onInstabilityCustomSeriesRequestFailed: {
            if (requestId !== instabilityPanel.customRequestId)
                return
            if (!instabilityPanel.experimentData || Number(instabilityPanel.experimentData.id) !== Number(experimentId))
                return

            instabilityPanel.customRequestId = 0
            instabilityPanel.customLoading = false
            console.log("[DetailInstability][custom failed]",
                        "experimentId=", experimentId,
                        "message=", message)
        }
        onInstabilityCustomSeriesRequestCancelled: {
            if (requestId !== instabilityPanel.customRequestId)
                return

            instabilityPanel.customRequestId = 0
            instabilityPanel.customLoading = false
            console.log("[DetailInstability][custom cancelled]",
                        "experimentId=", experimentId,
                        "reason=", reason)
        }
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
                        // 总览模式使用雷达图展示同一时间点在四个高度区间的横向对比。
                        anchors.fill: parent
                        polygons: instabilityPanel.radarPolygons
                        maxValue: instabilityPanel.radarMaxValue
                        tickCount: 6
                        axisLabels: [qsTr("Ius - 整体"), qsTr("Ius - 底部"), qsTr("Ius - 中部"), qsTr("Ius - 顶部")]
                    }
                }
            }

            Item {
                anchors.fill: parent
                visible: instabilityPanel.overviewLoading || instabilityPanel.customLoading
                z: 10

                Rectangle {
                    anchors.fill: parent
                    color: instabilityPanel.hasVisibleSeries() ? "#66FFFFFF" : "#F8FBFF"
                }

                Column {
                    anchors.centerIn: parent
                    spacing: 8

                    BusyIndicator {
                        anchors.horizontalCenter: parent.horizontalCenter
                        running: parent.parent.visible
                        width: 36
                        height: 36
                    }

                    Text {
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: instabilityPanel.customLoading
                              ? qsTr("正在加载自定义不稳定性数据")
                              : qsTr("正在加载不稳定性曲线")
                        font.pixelSize: 12
                        font.family: "Microsoft YaHei"
                        color: "#4A5D75"
                    }
                }
            }
        }
    }
}
