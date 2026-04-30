import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12

import "../component"

Rectangle {
    id: instabilityPanel

    property var comparePage
    readonly property var experimentIds: comparePage ? comparePage.experimentIds : []
    property int currentModeIndex: 0
    property var modeTitles: [qsTr("整体"), qsTr("局部"), qsTr("自定义"), qsTr("总览")]
    property var overallChart: createEmptyChart(qsTr("整体"))
    property var bottomChart: createEmptyChart(qsTr("底部"))
    property var middleChart: createEmptyChart(qsTr("中部"))
    property var topChart: createEmptyChart(qsTr("顶部"))
    property var customChart: createEmptyChart(qsTr("自定义"))
    property real customLowerBound: comparePage ? comparePage.globalMinHeightValue : 0
    property real customUpperBound: comparePage ? comparePage.globalMaxHeightValue : 55
    property bool overviewLoaded: false
    property bool customLoaded: false
    property bool overviewLoading: false
    property bool customLoading: false
    property int overviewRequestId: 0
    property int customRequestId: 0

    color: "#FFFFFF"

    function createEmptyChart(title) {
        return {
            title: title,
            rangeLabel: "",
            seriesList: [],
            chartMinX: 0,
            chartMaxX: 1,
            chartMinY: 0,
            chartMaxY: 1,
            xAxisTickValues: [0, 1],
            yAxisLabels: ["1.0", "0.8", "0.6", "0.4", "0.2", "0.0"]
        }
    }

    function hasChartData(chart) {
        if (!chart || !chart.seriesList)
            return false

        for (var i = 0; i < chart.seriesList.length; ++i) {
            if (chart.seriesList[i].has_data)
                return true
        }
        return false
    }

    function activeLegendSeries() {
        var charts = [overallChart, topChart, middleChart, bottomChart, customChart]
        for (var i = 0; i < charts.length; ++i) {
            if (charts[i] && charts[i].seriesList && charts[i].seriesList.length > 0)
                return charts[i].seriesList
        }
        return []
    }

    function resetInstabilityState() {
        overallChart = createEmptyChart(qsTr("整体"))
        bottomChart = createEmptyChart(qsTr("底部"))
        middleChart = createEmptyChart(qsTr("中部"))
        topChart = createEmptyChart(qsTr("顶部"))
        customChart = createEmptyChart(qsTr("自定义"))
        overviewLoaded = false
        customLoaded = false
        overviewLoading = false
        customLoading = false
        overviewRequestId = 0
        customRequestId = 0
    }

    function releaseRequests(reason) {
        if (!compare_ctrl)
            return

        if (overviewRequestId > 0)
            compare_ctrl.cancelInstabilityCompareRequest(overviewRequestId)
        if (customRequestId > 0)
            compare_ctrl.cancelInstabilityCompareRequest(customRequestId)

        if (overviewRequestId > 0 || customRequestId > 0) {
            console.log("[CompareInstability][release]",
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
        overallChart = payload && payload.overallChart ? payload.overallChart : createEmptyChart(qsTr("整体"))
        bottomChart = payload && payload.bottomChart ? payload.bottomChart : createEmptyChart(qsTr("底部"))
        middleChart = payload && payload.middleChart ? payload.middleChart : createEmptyChart(qsTr("中部"))
        topChart = payload && payload.topChart ? payload.topChart : createEmptyChart(qsTr("顶部"))
        overviewLoading = false
        overviewLoaded = true

        console.log("[CompareInstability][overview ready]",
                    "experimentCount=", experimentIds.length,
                    "overallSeries=", overallChart && overallChart.seriesList ? overallChart.seriesList.length : 0,
                    "topSeries=", topChart && topChart.seriesList ? topChart.seriesList.length : 0,
                    "middleSeries=", middleChart && middleChart.seriesList ? middleChart.seriesList.length : 0,
                    "bottomSeries=", bottomChart && bottomChart.seriesList ? bottomChart.seriesList.length : 0)
    }

    function applyCustomPayload(payload) {
        customChart = payload && payload.chart ? payload.chart : createEmptyChart(qsTr("自定义"))
        customLowerBound = comparePage ? comparePage.toNumber(payload.lowerMm, customLowerBound) : customLowerBound
        customUpperBound = comparePage ? comparePage.toNumber(payload.upperMm, customUpperBound) : customUpperBound
        customLowerField.text = comparePage ? comparePage.formatNumber(customLowerBound, 0) : String(customLowerBound)
        customUpperField.text = comparePage ? comparePage.formatNumber(customUpperBound, 0) : String(customUpperBound)
        customLoading = false
        customLoaded = true

        console.log("[CompareInstability][custom ready]",
                    "experimentCount=", experimentIds.length,
                    "lowerMm=", customLowerBound,
                    "upperMm=", customUpperBound,
                    "series=", customChart && customChart.seriesList ? customChart.seriesList.length : 0)
    }

    function requestOverviewData() {
        if (!comparePage || !compare_ctrl || experimentIds.length < 2)
            return

        overviewLoading = true
        overviewRequestId = compare_ctrl.requestInstabilityCompareOverview(experimentIds)
        console.log("[CompareInstability][overview request]",
                    "requestId=", overviewRequestId,
                    "experimentIds=", experimentIds)
        if (overviewRequestId <= 0)
            overviewLoading = false
    }

    function requestCustomData() {
        if (!comparePage || !compare_ctrl || experimentIds.length < 2)
            return

        customLowerBound = Math.max(comparePage.globalMinHeightValue,
                                    Math.min(customLowerBound, comparePage.globalMaxHeightValue))
        customUpperBound = Math.max(customLowerBound,
                                    Math.min(customUpperBound, comparePage.globalMaxHeightValue))

        customLoading = true
        customRequestId = compare_ctrl.requestInstabilityCompareCustom(experimentIds,
                                                                       customLowerBound,
                                                                       customUpperBound)
        console.log("[CompareInstability][custom request]",
                    "requestId=", customRequestId,
                    "experimentIds=", experimentIds,
                    "lowerMm=", customLowerBound,
                    "upperMm=", customUpperBound)
        if (customRequestId <= 0)
            customLoading = false
    }

    function ensureModeData() {
        if ((currentModeIndex === 0 || currentModeIndex === 1 || currentModeIndex === 3)
                && !overviewLoaded && !overviewLoading) {
            requestOverviewData()
        }

        if (currentModeIndex === 2 && !customLoaded && !customLoading) {
            requestCustomData()
        }
    }

    function applyCustomRange() {
        customLoaded = false
        requestCustomData()
    }

    function loadInstabilityData() {
        releaseRequests("reload")
        resetInstabilityState()
        if (!comparePage)
            return

        customLowerBound = comparePage.globalMinHeightValue
        customUpperBound = comparePage.globalMaxHeightValue
        customLowerField.text = comparePage.formatNumber(customLowerBound, 0)
        customUpperField.text = comparePage.formatNumber(customUpperBound, 0)
        requestOverviewData()
    }

    function activeLoading() {
        return currentModeIndex === 2 ? customLoading : overviewLoading
    }

    function hasVisibleData() {
        if (currentModeIndex === 0)
            return hasChartData(overallChart)
        if (currentModeIndex === 1)
            return hasChartData(topChart) || hasChartData(middleChart) || hasChartData(bottomChart)
        if (currentModeIndex === 2)
            return hasChartData(customChart)
        return hasChartData(overallChart) || hasChartData(topChart) || hasChartData(middleChart) || hasChartData(bottomChart)
    }

    onComparePageChanged: {
        console.log("[CompareInstability][comparePage changed]",
                    "hasComparePage=", !!comparePage,
                    "experimentIds=", experimentIds)
        if (comparePage)
            loadInstabilityData()
    }
    onExperimentIdsChanged: {
        console.log("[CompareInstability][experimentIds changed]",
                    "experimentIds=", experimentIds,
                    "overviewLoaded=", overviewLoaded,
                    "overviewLoading=", overviewLoading)
        if (comparePage && experimentIds.length >= 2)
            loadInstabilityData()
    }
    onCurrentModeIndexChanged: {
        console.log("[CompareInstability][mode changed]",
                    "modeIndex=", currentModeIndex,
                    "overviewLoaded=", overviewLoaded,
                    "customLoaded=", customLoaded,
                    "overviewLoading=", overviewLoading,
                    "customLoading=", customLoading,
                    "hasVisibleData=", hasVisibleData())
        ensureModeData()
    }
    Component.onCompleted: {
        console.log("[CompareInstability][completed]",
                    "experimentIds=", experimentIds)
        if (comparePage)
            loadInstabilityData()
    }
    Component.onDestruction: releaseRequests("destroyed")

    Connections {
        target: comparePage
        ignoreUnknownSignals: true
        onExperimentDataListChanged: {
            console.log("[CompareInstability][experimentDataList changed]",
                        "experimentIds=", instabilityPanel.experimentIds)
            instabilityPanel.loadInstabilityData()
        }
    }

    Connections {
        target: compare_ctrl
        ignoreUnknownSignals: true

        onInstabilityCompareOverviewRequestFinished: {
            if (requestId !== instabilityPanel.overviewRequestId) {
                console.log("[CompareInstability][overview finished ignored]",
                            "signalRequestId=", requestId,
                            "activeRequestId=", instabilityPanel.overviewRequestId)
                return
            }

            instabilityPanel.overviewRequestId = 0
            instabilityPanel.applyOverviewPayload(payload)
        }
        onInstabilityCompareOverviewRequestFailed: {
            if (requestId !== instabilityPanel.overviewRequestId) {
                console.log("[CompareInstability][overview failed ignored]",
                            "signalRequestId=", requestId,
                            "activeRequestId=", instabilityPanel.overviewRequestId)
                return
            }

            instabilityPanel.overviewRequestId = 0
            instabilityPanel.overviewLoading = false
            console.log("[CompareInstability][overview failed]",
                        "message=", message)
        }
        onInstabilityCompareOverviewRequestCancelled: {
            if (requestId !== instabilityPanel.overviewRequestId) {
                console.log("[CompareInstability][overview cancelled ignored]",
                            "signalRequestId=", requestId,
                            "activeRequestId=", instabilityPanel.overviewRequestId)
                return
            }

            instabilityPanel.overviewRequestId = 0
            instabilityPanel.overviewLoading = false
            console.log("[CompareInstability][overview cancelled]",
                        "reason=", reason)
        }
        onInstabilityCompareCustomRequestFinished: {
            if (requestId !== instabilityPanel.customRequestId) {
                console.log("[CompareInstability][custom finished ignored]",
                            "signalRequestId=", requestId,
                            "activeRequestId=", instabilityPanel.customRequestId)
                return
            }

            instabilityPanel.customRequestId = 0
            instabilityPanel.applyCustomPayload(payload)
        }
        onInstabilityCompareCustomRequestFailed: {
            if (requestId !== instabilityPanel.customRequestId) {
                console.log("[CompareInstability][custom failed ignored]",
                            "signalRequestId=", requestId,
                            "activeRequestId=", instabilityPanel.customRequestId)
                return
            }

            instabilityPanel.customRequestId = 0
            instabilityPanel.customLoading = false
            console.log("[CompareInstability][custom failed]",
                        "message=", message)
        }
        onInstabilityCompareCustomRequestCancelled: {
            if (requestId !== instabilityPanel.customRequestId) {
                console.log("[CompareInstability][custom cancelled ignored]",
                            "signalRequestId=", requestId,
                            "activeRequestId=", instabilityPanel.customRequestId)
                return
            }

            instabilityPanel.customRequestId = 0
            instabilityPanel.customLoading = false
            console.log("[CompareInstability][custom cancelled]",
                        "reason=", reason)
        }
    }

    RowLayout {
        anchors.fill: parent
        spacing: 14

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            ColumnLayout {
                anchors.fill: parent
                spacing: 14

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
                    Layout.fillWidth: true
                    Layout.preferredHeight: 48
                    visible: instabilityPanel.currentModeIndex === 2
                    radius: 4
                    color: "#FFFFFF"
                    border.color: "#D8E4F0"
                    border.width: 1

                    Row {
                        anchors.fill: parent
                        anchors.leftMargin: 12
                        anchors.rightMargin: 12
                        spacing: 8

                        Text {
                            anchors.verticalCenter: parent.verticalCenter
                            text: qsTr("高度区间")
                            font.pixelSize: 12
                            font.family: "Microsoft YaHei"
                            color: "#2F3A4A"
                        }

                        LineEdit {
                            id: customLowerField
                            anchors.verticalCenter: parent.verticalCenter
                            width: 60
                            height: 30
                            text: comparePage ? comparePage.formatNumber(instabilityPanel.customLowerBound, 0) : "0"
                            horizontalAlignment: Text.AlignHCenter
                            validator: IntValidator {
                                bottom: comparePage ? Math.floor(comparePage.globalMinHeightValue) : 0
                                top: comparePage ? Math.ceil(comparePage.globalMaxHeightValue) : 100
                            }
                        }

                        Text {
                            anchors.verticalCenter: parent.verticalCenter
                            text: "mm"
                            font.pixelSize: 12
                            font.family: "Microsoft YaHei"
                            color: "#2F3A4A"
                        }

                        Text {
                            anchors.verticalCenter: parent.verticalCenter
                            text: "~"
                            font.pixelSize: 13
                            font.family: "Microsoft YaHei"
                            color: "#2F3A4A"
                        }

                        LineEdit {
                            id: customUpperField
                            anchors.verticalCenter: parent.verticalCenter
                            width: 60
                            height: 30
                            text: comparePage ? comparePage.formatNumber(instabilityPanel.customUpperBound, 0) : "55"
                            horizontalAlignment: Text.AlignHCenter
                            validator: IntValidator {
                                bottom: comparePage ? Math.floor(comparePage.globalMinHeightValue) : 0
                                top: comparePage ? Math.ceil(comparePage.globalMaxHeightValue) : 100
                            }
                        }

                        Text {
                            anchors.verticalCenter: parent.verticalCenter
                            text: "mm"
                            font.pixelSize: 12
                            font.family: "Microsoft YaHei"
                            color: "#2F3A4A"
                        }

                        IconButton {
                            anchors.verticalCenter: parent.verticalCenter
                            width: 92
                            height: 30
                            button_text: qsTr("应用")
                            button_color: "#4A89DC"
                            text_color: "#FFFFFF"
                            pixelSize: 12
                            onClicked: {
                                var lowerValue = comparePage.toNumber(customLowerField.text, instabilityPanel.customLowerBound)
                                var upperValue = comparePage.toNumber(customUpperField.text, instabilityPanel.customUpperBound)
                                if (upperValue < lowerValue) {
                                    var tempValue = lowerValue
                                    lowerValue = upperValue
                                    upperValue = tempValue
                                }

                                instabilityPanel.customLowerBound = lowerValue
                                instabilityPanel.customUpperBound = upperValue
                                instabilityPanel.applyCustomRange()
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

                    Column {
                        anchors.centerIn: parent
                        spacing: 12
                        visible: instabilityPanel.activeLoading()
                        z: 10

                        BusyIndicator {
                            anchors.horizontalCenter: parent.horizontalCenter
                            running: instabilityPanel.activeLoading()
                        }

                        Text {
                            anchors.horizontalCenter: parent.horizontalCenter
                            text: instabilityPanel.currentModeIndex === 2
                                  ? qsTr("正在加载自定义不稳定性对比数据，请稍候")
                                  : qsTr("正在加载不稳定性对比数据，请稍候")
                            font.pixelSize: 15
                            font.family: "Microsoft YaHei"
                            color: "#7A8CA5"
                        }
                    }

                    Text {
                        anchors.centerIn: parent
                        visible: !instabilityPanel.activeLoading() && !instabilityPanel.hasVisibleData()
                        text: qsTr("所选实验暂无可对比的不稳定性数据")
                        font.pixelSize: 15
                        font.family: "Microsoft YaHei"
                        color: "#7A8CA5"
                    }

                    Item {
                        anchors.fill: parent
                        anchors.margins: 10
                        visible: instabilityPanel.hasVisibleData()

                        CompareTrendChart {
                            anchors.fill: parent
                            visible: instabilityPanel.currentModeIndex === 0
                            chartTitle: instabilityPanel.overallChart && instabilityPanel.overallChart.title ? instabilityPanel.overallChart.title : qsTr("整体")
                            seriesList: instabilityPanel.overallChart && instabilityPanel.overallChart.seriesList ? instabilityPanel.overallChart.seriesList : []
                            minXValue: comparePage.toNumber(instabilityPanel.overallChart.chartMinX, 0)
                            maxXValue: comparePage.toNumber(instabilityPanel.overallChart.chartMaxX, 1)
                            minYValue: comparePage.toNumber(instabilityPanel.overallChart.chartMinY, 0)
                            maxYValue: comparePage.toNumber(instabilityPanel.overallChart.chartMaxY, 1)
                            xAxisTickValues: instabilityPanel.overallChart && instabilityPanel.overallChart.xAxisTickValues ? instabilityPanel.overallChart.xAxisTickValues : [0, 1]
                            yAxisLabels: instabilityPanel.overallChart && instabilityPanel.overallChart.yAxisLabels ? instabilityPanel.overallChart.yAxisLabels : ["1.0", "0.0"]
                            yAxisTitle: "Ius"
                            xAxisTitle: qsTr("时间(min)")
                            formatXLabel: function(value) { return comparePage.formatNumber(value, 1) }
                        }

                        ColumnLayout {
                            anchors.fill: parent
                            spacing: 12
                            visible: instabilityPanel.currentModeIndex === 1

                            CompareTrendChart {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                Layout.preferredHeight: 150
                                chartTitle: instabilityPanel.topChart && instabilityPanel.topChart.title ? instabilityPanel.topChart.title : qsTr("顶部")
                                seriesList: instabilityPanel.topChart && instabilityPanel.topChart.seriesList ? instabilityPanel.topChart.seriesList : []
                                minXValue: comparePage.toNumber(instabilityPanel.topChart.chartMinX, 0)
                                maxXValue: comparePage.toNumber(instabilityPanel.topChart.chartMaxX, 1)
                                minYValue: comparePage.toNumber(instabilityPanel.topChart.chartMinY, 0)
                                maxYValue: comparePage.toNumber(instabilityPanel.topChart.chartMaxY, 1)
                                xAxisTickValues: instabilityPanel.topChart && instabilityPanel.topChart.xAxisTickValues ? instabilityPanel.topChart.xAxisTickValues : [0, 1]
                                yAxisLabels: instabilityPanel.topChart && instabilityPanel.topChart.yAxisLabels ? instabilityPanel.topChart.yAxisLabels : ["1.0", "0.0"]
                                yAxisTitle: "Ius"
                                showXAxisLabels: false
                                showXAxisTitle: false
                                formatXLabel: function(value) { return comparePage.formatNumber(value, 1) }
                            }

                            CompareTrendChart {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                Layout.preferredHeight: 150
                                chartTitle: instabilityPanel.middleChart && instabilityPanel.middleChart.title ? instabilityPanel.middleChart.title : qsTr("中部")
                                seriesList: instabilityPanel.middleChart && instabilityPanel.middleChart.seriesList ? instabilityPanel.middleChart.seriesList : []
                                minXValue: comparePage.toNumber(instabilityPanel.middleChart.chartMinX, 0)
                                maxXValue: comparePage.toNumber(instabilityPanel.middleChart.chartMaxX, 1)
                                minYValue: comparePage.toNumber(instabilityPanel.middleChart.chartMinY, 0)
                                maxYValue: comparePage.toNumber(instabilityPanel.middleChart.chartMaxY, 1)
                                xAxisTickValues: instabilityPanel.middleChart && instabilityPanel.middleChart.xAxisTickValues ? instabilityPanel.middleChart.xAxisTickValues : [0, 1]
                                yAxisLabels: instabilityPanel.middleChart && instabilityPanel.middleChart.yAxisLabels ? instabilityPanel.middleChart.yAxisLabels : ["1.0", "0.0"]
                                yAxisTitle: "Ius"
                                showXAxisLabels: false
                                showXAxisTitle: false
                                formatXLabel: function(value) { return comparePage.formatNumber(value, 1) }
                            }

                            CompareTrendChart {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                Layout.preferredHeight: 150
                                chartTitle: instabilityPanel.bottomChart && instabilityPanel.bottomChart.title ? instabilityPanel.bottomChart.title : qsTr("底部")
                                seriesList: instabilityPanel.bottomChart && instabilityPanel.bottomChart.seriesList ? instabilityPanel.bottomChart.seriesList : []
                                minXValue: comparePage.toNumber(instabilityPanel.bottomChart.chartMinX, 0)
                                maxXValue: comparePage.toNumber(instabilityPanel.bottomChart.chartMaxX, 1)
                                minYValue: comparePage.toNumber(instabilityPanel.bottomChart.chartMinY, 0)
                                maxYValue: comparePage.toNumber(instabilityPanel.bottomChart.chartMaxY, 1)
                                xAxisTickValues: instabilityPanel.bottomChart && instabilityPanel.bottomChart.xAxisTickValues ? instabilityPanel.bottomChart.xAxisTickValues : [0, 1]
                                yAxisLabels: instabilityPanel.bottomChart && instabilityPanel.bottomChart.yAxisLabels ? instabilityPanel.bottomChart.yAxisLabels : ["1.0", "0.0"]
                                yAxisTitle: "Ius"
                                xAxisTitle: qsTr("时间(min)")
                                formatXLabel: function(value) { return comparePage.formatNumber(value, 1) }
                            }
                        }

                        CompareTrendChart {
                            anchors.fill: parent
                            visible: instabilityPanel.currentModeIndex === 2
                            chartTitle: instabilityPanel.customChart && instabilityPanel.customChart.title ? instabilityPanel.customChart.title : qsTr("自定义")
                            chartSubtitle: instabilityPanel.customChart && instabilityPanel.customChart.rangeLabel ? instabilityPanel.customChart.rangeLabel : ""
                            seriesList: instabilityPanel.customChart && instabilityPanel.customChart.seriesList ? instabilityPanel.customChart.seriesList : []
                            minXValue: comparePage.toNumber(instabilityPanel.customChart.chartMinX, 0)
                            maxXValue: comparePage.toNumber(instabilityPanel.customChart.chartMaxX, 1)
                            minYValue: comparePage.toNumber(instabilityPanel.customChart.chartMinY, 0)
                            maxYValue: comparePage.toNumber(instabilityPanel.customChart.chartMaxY, 1)
                            xAxisTickValues: instabilityPanel.customChart && instabilityPanel.customChart.xAxisTickValues ? instabilityPanel.customChart.xAxisTickValues : [0, 1]
                            yAxisLabels: instabilityPanel.customChart && instabilityPanel.customChart.yAxisLabels ? instabilityPanel.customChart.yAxisLabels : ["1.0", "0.0"]
                            yAxisTitle: "Ius"
                            xAxisTitle: qsTr("时间(min)")
                            formatXLabel: function(value) { return comparePage.formatNumber(value, 1) }
                        }

                        GridLayout {
                            anchors.fill: parent
                            columns: 2
                            rowSpacing: 12
                            columnSpacing: 12
                            visible: instabilityPanel.currentModeIndex === 3

                            CompareTrendChart {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                chartTitle: instabilityPanel.overallChart && instabilityPanel.overallChart.title ? instabilityPanel.overallChart.title : qsTr("整体")
                                seriesList: instabilityPanel.overallChart && instabilityPanel.overallChart.seriesList ? instabilityPanel.overallChart.seriesList : []
                                minXValue: comparePage.toNumber(instabilityPanel.overallChart.chartMinX, 0)
                                maxXValue: comparePage.toNumber(instabilityPanel.overallChart.chartMaxX, 1)
                                minYValue: comparePage.toNumber(instabilityPanel.overallChart.chartMinY, 0)
                                maxYValue: comparePage.toNumber(instabilityPanel.overallChart.chartMaxY, 1)
                                xAxisTickValues: instabilityPanel.overallChart && instabilityPanel.overallChart.xAxisTickValues ? instabilityPanel.overallChart.xAxisTickValues : [0, 1]
                                yAxisLabels: instabilityPanel.overallChart && instabilityPanel.overallChart.yAxisLabels ? instabilityPanel.overallChart.yAxisLabels : ["1.0", "0.0"]
                                yAxisTitle: "Ius"
                                showXAxisLabels: false
                                showXAxisTitle: false
                                formatXLabel: function(value) { return comparePage.formatNumber(value, 1) }
                            }

                            CompareTrendChart {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                chartTitle: instabilityPanel.topChart && instabilityPanel.topChart.title ? instabilityPanel.topChart.title : qsTr("顶部")
                                seriesList: instabilityPanel.topChart && instabilityPanel.topChart.seriesList ? instabilityPanel.topChart.seriesList : []
                                minXValue: comparePage.toNumber(instabilityPanel.topChart.chartMinX, 0)
                                maxXValue: comparePage.toNumber(instabilityPanel.topChart.chartMaxX, 1)
                                minYValue: comparePage.toNumber(instabilityPanel.topChart.chartMinY, 0)
                                maxYValue: comparePage.toNumber(instabilityPanel.topChart.chartMaxY, 1)
                                xAxisTickValues: instabilityPanel.topChart && instabilityPanel.topChart.xAxisTickValues ? instabilityPanel.topChart.xAxisTickValues : [0, 1]
                                yAxisLabels: instabilityPanel.topChart && instabilityPanel.topChart.yAxisLabels ? instabilityPanel.topChart.yAxisLabels : ["1.0", "0.0"]
                                yAxisTitle: "Ius"
                                showXAxisLabels: false
                                showXAxisTitle: false
                                formatXLabel: function(value) { return comparePage.formatNumber(value, 1) }
                            }

                            CompareTrendChart {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                chartTitle: instabilityPanel.middleChart && instabilityPanel.middleChart.title ? instabilityPanel.middleChart.title : qsTr("中部")
                                seriesList: instabilityPanel.middleChart && instabilityPanel.middleChart.seriesList ? instabilityPanel.middleChart.seriesList : []
                                minXValue: comparePage.toNumber(instabilityPanel.middleChart.chartMinX, 0)
                                maxXValue: comparePage.toNumber(instabilityPanel.middleChart.chartMaxX, 1)
                                minYValue: comparePage.toNumber(instabilityPanel.middleChart.chartMinY, 0)
                                maxYValue: comparePage.toNumber(instabilityPanel.middleChart.chartMaxY, 1)
                                xAxisTickValues: instabilityPanel.middleChart && instabilityPanel.middleChart.xAxisTickValues ? instabilityPanel.middleChart.xAxisTickValues : [0, 1]
                                yAxisLabels: instabilityPanel.middleChart && instabilityPanel.middleChart.yAxisLabels ? instabilityPanel.middleChart.yAxisLabels : ["1.0", "0.0"]
                                yAxisTitle: "Ius"
                                xAxisTitle: qsTr("时间(min)")
                                formatXLabel: function(value) { return comparePage.formatNumber(value, 1) }
                            }

                            CompareTrendChart {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                chartTitle: instabilityPanel.bottomChart && instabilityPanel.bottomChart.title ? instabilityPanel.bottomChart.title : qsTr("底部")
                                seriesList: instabilityPanel.bottomChart && instabilityPanel.bottomChart.seriesList ? instabilityPanel.bottomChart.seriesList : []
                                minXValue: comparePage.toNumber(instabilityPanel.bottomChart.chartMinX, 0)
                                maxXValue: comparePage.toNumber(instabilityPanel.bottomChart.chartMaxX, 1)
                                minYValue: comparePage.toNumber(instabilityPanel.bottomChart.chartMinY, 0)
                                maxYValue: comparePage.toNumber(instabilityPanel.bottomChart.chartMaxY, 1)
                                xAxisTickValues: instabilityPanel.bottomChart && instabilityPanel.bottomChart.xAxisTickValues ? instabilityPanel.bottomChart.xAxisTickValues : [0, 1]
                                yAxisLabels: instabilityPanel.bottomChart && instabilityPanel.bottomChart.yAxisLabels ? instabilityPanel.bottomChart.yAxisLabels : ["1.0", "0.0"]
                                yAxisTitle: "Ius"
                                xAxisTitle: qsTr("时间(min)")
                                formatXLabel: function(value) { return comparePage.formatNumber(value, 1) }
                            }
                        }
                    }
                }
            }
        }

        CompareLegendPanel {
            Layout.preferredWidth: 228
            Layout.fillHeight: true
            seriesList: instabilityPanel.activeLegendSeries()
        }
    }
}
