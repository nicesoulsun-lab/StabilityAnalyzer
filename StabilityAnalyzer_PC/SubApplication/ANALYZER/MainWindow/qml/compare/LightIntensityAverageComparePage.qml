import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12

Rectangle {
    id: avgPanel

    property var comparePage
    readonly property var experimentIds: comparePage ? comparePage.experimentIds : []
    property int currentModeIndex: 0
    property var modeTitles: [qsTr("背射光"), qsTr("透射光"), qsTr("背射光/透射光")]
    property var bsChart: createEmptyChart(qsTr("背射光"))
    property var tChart: createEmptyChart(qsTr("透射光"))
    property bool loading: false
    property int requestId: 0

    color: "#FFFFFF"

    function createEmptyChart(title) {
        return {
            title: title,
            seriesList: [],
            chartMinX: 0,
            chartMaxX: 1,
            chartMinY: 0,
            chartMaxY: 1,
            xAxisTickValues: [0, 1],
            yAxisLabels: ["1.0", "0.0"]
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
        if (bsChart && bsChart.seriesList && bsChart.seriesList.length > 0)
            return bsChart.seriesList
        if (tChart && tChart.seriesList && tChart.seriesList.length > 0)
            return tChart.seriesList
        return []
    }

    function chartPointCounts(chart) {
        var counts = []
        var list = chart && chart.seriesList ? chart.seriesList : []
        for (var i = 0; i < list.length; ++i) {
            var series = list[i]
            if (series.point_count !== undefined)
                counts.push(series.point_count)
            else if (series.points)
                counts.push(series.points.length)
            else
                counts.push(0)
        }
        return counts
    }

    function releaseCompareRequest(reason) {
        if (!compare_ctrl)
            return

        if (requestId > 0) {
            compare_ctrl.cancelLightIntensityAverageCompareRequest(requestId)
            console.log("[CompareAverage][release]",
                        "reason=", reason,
                        "requestId=", requestId)
        }

        requestId = 0
        loading = false
    }

    function applyPayload(payload) {
        bsChart = payload && payload.bsChart ? payload.bsChart : createEmptyChart(qsTr("背射光"))
        tChart = payload && payload.tChart ? payload.tChart : createEmptyChart(qsTr("透射光"))
        loading = false

        console.log("[CompareAverage][ready]",
                    "experimentCount=", experimentIds.length,
                    "bsSeries=", bsChart && bsChart.seriesList ? bsChart.seriesList.length : 0,
                    "tSeries=", tChart && tChart.seriesList ? tChart.seriesList.length : 0,
                    "bsPointCounts=", chartPointCounts(bsChart),
                    "tPointCounts=", chartPointCounts(tChart),
                    "bsHasData=", hasChartData(bsChart),
                    "tHasData=", hasChartData(tChart))
    }

    function loadAverageData() {
        releaseCompareRequest("reload")
        bsChart = createEmptyChart(qsTr("背射光"))
        tChart = createEmptyChart(qsTr("透射光"))

        if (!comparePage || !compare_ctrl || experimentIds.length < 2) {
            console.log("[CompareAverage][request skipped]",
                        "hasComparePage=", !!comparePage,
                        "hasCtrl=", !!compare_ctrl,
                        "experimentIds=", experimentIds)
            return
        }

        loading = true
        requestId = compare_ctrl.requestLightIntensityAverageCompare(experimentIds)
        console.log("[CompareAverage][request]",
                    "requestId=", requestId,
                    "experimentIds=", experimentIds)
        if (requestId <= 0)
            loading = false
    }

    onComparePageChanged: {
        console.log("[CompareAverage][comparePage changed]",
                    "hasComparePage=", !!comparePage,
                    "experimentIds=", experimentIds)
        if (comparePage)
            loadAverageData()
    }
    onExperimentIdsChanged: {
        console.log("[CompareAverage][experimentIds changed]",
                    "experimentIds=", experimentIds,
                    "loading=", loading)
        if (comparePage && experimentIds.length >= 2)
            loadAverageData()
    }
    onCurrentModeIndexChanged: {
        console.log("[CompareAverage][mode changed]",
                    "modeIndex=", currentModeIndex,
                    "loading=", loading,
                    "bsHasData=", hasChartData(bsChart),
                    "tHasData=", hasChartData(tChart))
    }
    Component.onCompleted: {
        console.log("[CompareAverage][completed]",
                    "experimentIds=", experimentIds)
        if (comparePage)
            loadAverageData()
    }
    Component.onDestruction: releaseCompareRequest("destroyed")

    Connections {
        target: comparePage
        ignoreUnknownSignals: true
        onExperimentDataListChanged: {
            console.log("[CompareAverage][experimentDataList changed]",
                        "experimentIds=", avgPanel.experimentIds)
            avgPanel.loadAverageData()
        }
    }

    Connections {
        target: compare_ctrl
        ignoreUnknownSignals: true

        onLightIntensityAverageCompareRequestFinished: {
            if (requestId !== avgPanel.requestId) {
                console.log("[CompareAverage][finished ignored]",
                            "signalRequestId=", requestId,
                            "activeRequestId=", avgPanel.requestId)
                return
            }

            avgPanel.requestId = 0
            avgPanel.applyPayload(payload)
        }
        onLightIntensityAverageCompareRequestFailed: {
            if (requestId !== avgPanel.requestId) {
                console.log("[CompareAverage][failed ignored]",
                            "signalRequestId=", requestId,
                            "activeRequestId=", avgPanel.requestId)
                return
            }

            avgPanel.requestId = 0
            avgPanel.loading = false
            console.log("[CompareAverage][failed]",
                        "message=", message)
        }
        onLightIntensityAverageCompareRequestCancelled: {
            if (requestId !== avgPanel.requestId) {
                console.log("[CompareAverage][cancelled ignored]",
                            "signalRequestId=", requestId,
                            "activeRequestId=", avgPanel.requestId)
                return
            }

            avgPanel.requestId = 0
            avgPanel.loading = false
            console.log("[CompareAverage][cancelled]",
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
                        model: avgPanel.modeTitles

                        delegate: Button {
                            id: averageModeButton
                            width: 116
                            height: 28
                            text: modelData
                            onClicked: avgPanel.currentModeIndex = index

                            contentItem: Text {
                                text: averageModeButton.text
                                font.pixelSize: 12
                                font.family: "Microsoft YaHei"
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                                color: avgPanel.currentModeIndex === index ? "#FFFFFF" : "#4A89DC"
                            }

                            background: Rectangle {
                                color: avgPanel.currentModeIndex === index ? "#4A89DC" : "#FFFFFF"
                                border.color: "#4A89DC"
                                border.width: 1
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
                        visible: avgPanel.loading
                        z: 10

                        BusyIndicator {
                            anchors.horizontalCenter: parent.horizontalCenter
                            running: avgPanel.loading
                        }

                        Text {
                            anchors.horizontalCenter: parent.horizontalCenter
                            text: qsTr("正在加载光强平均值对比数据，请稍候")
                            font.pixelSize: 15
                            font.family: "Microsoft YaHei"
                            color: "#7A8CA5"
                        }
                    }

                    Text {
                        anchors.centerIn: parent
                        visible: !avgPanel.loading
                                 && !avgPanel.hasChartData(avgPanel.bsChart)
                                 && !avgPanel.hasChartData(avgPanel.tChart)
                        text: qsTr("所选实验暂无可对比的光强平均值数据")
                        font.pixelSize: 15
                        font.family: "Microsoft YaHei"
                        color: "#7A8CA5"
                    }

                    Item {
                        anchors.fill: parent
                        anchors.margins: 10
                        visible: avgPanel.hasChartData(avgPanel.bsChart)
                                 || avgPanel.hasChartData(avgPanel.tChart)

                        ColumnLayout {
                            anchors.fill: parent
                            spacing: 12
                            visible: avgPanel.currentModeIndex === 2

                            CompareTrendChart {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                Layout.preferredHeight: 180
                                chartTitle: avgPanel.tChart && avgPanel.tChart.title ? avgPanel.tChart.title : qsTr("透射光")
                                seriesList: avgPanel.tChart && avgPanel.tChart.seriesList ? avgPanel.tChart.seriesList : []
                                minXValue: comparePage.toNumber(avgPanel.tChart.chartMinX, 0)
                                maxXValue: comparePage.toNumber(avgPanel.tChart.chartMaxX, 1)
                                minYValue: comparePage.toNumber(avgPanel.tChart.chartMinY, 0)
                                maxYValue: comparePage.toNumber(avgPanel.tChart.chartMaxY, 1)
                                xAxisTickValues: avgPanel.tChart && avgPanel.tChart.xAxisTickValues ? avgPanel.tChart.xAxisTickValues : [0, 1]
                                yAxisLabels: avgPanel.tChart && avgPanel.tChart.yAxisLabels ? avgPanel.tChart.yAxisLabels : ["1.0", "0.0"]
                                yAxisTitle: "Avg\n(T)"
                                showXAxisLabels: false
                                showXAxisTitle: false
                                formatXLabel: function(value) { return comparePage.formatNumber(value, 1) }
                            }

                            CompareTrendChart {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                Layout.preferredHeight: 180
                                chartTitle: avgPanel.bsChart && avgPanel.bsChart.title ? avgPanel.bsChart.title : qsTr("背射光")
                                seriesList: avgPanel.bsChart && avgPanel.bsChart.seriesList ? avgPanel.bsChart.seriesList : []
                                minXValue: comparePage.toNumber(avgPanel.bsChart.chartMinX, 0)
                                maxXValue: comparePage.toNumber(avgPanel.bsChart.chartMaxX, 1)
                                minYValue: comparePage.toNumber(avgPanel.bsChart.chartMinY, 0)
                                maxYValue: comparePage.toNumber(avgPanel.bsChart.chartMaxY, 1)
                                xAxisTickValues: avgPanel.bsChart && avgPanel.bsChart.xAxisTickValues ? avgPanel.bsChart.xAxisTickValues : [0, 1]
                                yAxisLabels: avgPanel.bsChart && avgPanel.bsChart.yAxisLabels ? avgPanel.bsChart.yAxisLabels : ["1.0", "0.0"]
                                yAxisTitle: "Avg\n(BS)"
                                xAxisTitle: qsTr("时间(min)")
                                formatXLabel: function(value) { return comparePage.formatNumber(value, 1) }
                            }
                        }

                        CompareTrendChart {
                            anchors.fill: parent
                            visible: avgPanel.currentModeIndex !== 2
                            chartTitle: avgPanel.currentModeIndex === 0
                                        ? (avgPanel.bsChart && avgPanel.bsChart.title ? avgPanel.bsChart.title : qsTr("背射光"))
                                        : (avgPanel.tChart && avgPanel.tChart.title ? avgPanel.tChart.title : qsTr("透射光"))
                            seriesList: avgPanel.currentModeIndex === 0
                                        ? (avgPanel.bsChart && avgPanel.bsChart.seriesList ? avgPanel.bsChart.seriesList : [])
                                        : (avgPanel.tChart && avgPanel.tChart.seriesList ? avgPanel.tChart.seriesList : [])
                            minXValue: avgPanel.currentModeIndex === 0
                                       ? comparePage.toNumber(avgPanel.bsChart.chartMinX, 0)
                                       : comparePage.toNumber(avgPanel.tChart.chartMinX, 0)
                            maxXValue: avgPanel.currentModeIndex === 0
                                       ? comparePage.toNumber(avgPanel.bsChart.chartMaxX, 1)
                                       : comparePage.toNumber(avgPanel.tChart.chartMaxX, 1)
                            minYValue: avgPanel.currentModeIndex === 0
                                       ? comparePage.toNumber(avgPanel.bsChart.chartMinY, 0)
                                       : comparePage.toNumber(avgPanel.tChart.chartMinY, 0)
                            maxYValue: avgPanel.currentModeIndex === 0
                                       ? comparePage.toNumber(avgPanel.bsChart.chartMaxY, 1)
                                       : comparePage.toNumber(avgPanel.tChart.chartMaxY, 1)
                            xAxisTickValues: avgPanel.currentModeIndex === 0
                                             ? (avgPanel.bsChart && avgPanel.bsChart.xAxisTickValues ? avgPanel.bsChart.xAxisTickValues : [0, 1])
                                             : (avgPanel.tChart && avgPanel.tChart.xAxisTickValues ? avgPanel.tChart.xAxisTickValues : [0, 1])
                            yAxisLabels: avgPanel.currentModeIndex === 0
                                         ? (avgPanel.bsChart && avgPanel.bsChart.yAxisLabels ? avgPanel.bsChart.yAxisLabels : ["1.0", "0.0"])
                                         : (avgPanel.tChart && avgPanel.tChart.yAxisLabels ? avgPanel.tChart.yAxisLabels : ["1.0", "0.0"])
                            yAxisTitle: avgPanel.currentModeIndex === 0 ? "Avg\n(BS)" : "Avg\n(T)"
                            xAxisTitle: qsTr("时间(min)")
                            formatXLabel: function(value) { return comparePage.formatNumber(value, 1) }
                        }
                    }
                }
            }
        }

        CompareLegendPanel {
            Layout.preferredWidth: 228
            Layout.fillHeight: true
            seriesList: avgPanel.activeLegendSeries()
        }
    }
}
