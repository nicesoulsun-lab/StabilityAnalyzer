import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12

Rectangle {
    id: uniformityPanel

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
            yAxisLabels: ["1.00", "0.80", "0.60", "0.40", "0.20", "0.00"]
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
            compare_ctrl.cancelUniformityCompareRequest(requestId)
            console.log("[CompareUniformity][release]",
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

        console.log("[CompareUniformity][ready]",
                    "experimentCount=", experimentIds.length,
                    "bsSeries=", bsChart && bsChart.seriesList ? bsChart.seriesList.length : 0,
                    "tSeries=", tChart && tChart.seriesList ? tChart.seriesList.length : 0,
                    "bsPointCounts=", chartPointCounts(bsChart),
                    "tPointCounts=", chartPointCounts(tChart),
                    "bsHasData=", hasChartData(bsChart),
                    "tHasData=", hasChartData(tChart))
    }

    function loadUniformityData() {
        releaseCompareRequest("reload")
        bsChart = createEmptyChart(qsTr("背射光"))
        tChart = createEmptyChart(qsTr("透射光"))

        if (!comparePage || !compare_ctrl || experimentIds.length < 2) {
            console.log("[CompareUniformity][request skipped]",
                        "hasComparePage=", !!comparePage,
                        "hasCtrl=", !!compare_ctrl,
                        "experimentIds=", experimentIds)
            return
        }

        loading = true
        requestId = compare_ctrl.requestUniformityCompare(experimentIds)
        console.log("[CompareUniformity][request]",
                    "requestId=", requestId,
                    "experimentIds=", experimentIds)
        if (requestId <= 0)
            loading = false
    }

    onComparePageChanged: {
        console.log("[CompareUniformity][comparePage changed]",
                    "hasComparePage=", !!comparePage,
                    "experimentIds=", experimentIds)
        if (comparePage)
            loadUniformityData()
    }
    onExperimentIdsChanged: {
        console.log("[CompareUniformity][experimentIds changed]",
                    "experimentIds=", experimentIds,
                    "loading=", loading)
        if (comparePage && experimentIds.length >= 2)
            loadUniformityData()
    }
    onCurrentModeIndexChanged: {
        console.log("[CompareUniformity][mode changed]",
                    "modeIndex=", currentModeIndex,
                    "loading=", loading,
                    "bsHasData=", hasChartData(bsChart),
                    "tHasData=", hasChartData(tChart))
    }
    Component.onCompleted: {
        console.log("[CompareUniformity][completed]",
                    "experimentIds=", experimentIds)
        if (comparePage)
            loadUniformityData()
    }
    Component.onDestruction: releaseCompareRequest("destroyed")

    Connections {
        target: comparePage
        ignoreUnknownSignals: true
        onExperimentDataListChanged: {
            console.log("[CompareUniformity][experimentDataList changed]",
                        "experimentIds=", uniformityPanel.experimentIds)
            uniformityPanel.loadUniformityData()
        }
    }

    Connections {
        target: compare_ctrl
        ignoreUnknownSignals: true

        onUniformityCompareRequestFinished: {
            if (requestId !== uniformityPanel.requestId) {
                console.log("[CompareUniformity][finished ignored]",
                            "signalRequestId=", requestId,
                            "activeRequestId=", uniformityPanel.requestId)
                return
            }

            uniformityPanel.requestId = 0
            uniformityPanel.applyPayload(payload)
        }
        onUniformityCompareRequestFailed: {
            if (requestId !== uniformityPanel.requestId) {
                console.log("[CompareUniformity][failed ignored]",
                            "signalRequestId=", requestId,
                            "activeRequestId=", uniformityPanel.requestId)
                return
            }

            uniformityPanel.requestId = 0
            uniformityPanel.loading = false
            console.log("[CompareUniformity][failed]",
                        "message=", message)
        }
        onUniformityCompareRequestCancelled: {
            if (requestId !== uniformityPanel.requestId) {
                console.log("[CompareUniformity][cancelled ignored]",
                            "signalRequestId=", requestId,
                            "activeRequestId=", uniformityPanel.requestId)
                return
            }

            uniformityPanel.requestId = 0
            uniformityPanel.loading = false
            console.log("[CompareUniformity][cancelled]",
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
                        model: uniformityPanel.modeTitles

                        delegate: Button {
                            id: uniformityModeButton
                            width: 116
                            height: 28
                            text: modelData
                            onClicked: uniformityPanel.currentModeIndex = index

                            contentItem: Text {
                                text: uniformityModeButton.text
                                font.pixelSize: 12
                                font.family: "Microsoft YaHei"
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                                color: uniformityPanel.currentModeIndex === index ? "#FFFFFF" : "#4A89DC"
                            }

                            background: Rectangle {
                                color: uniformityPanel.currentModeIndex === index ? "#4A89DC" : "#FFFFFF"
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
                        visible: uniformityPanel.loading
                        z: 10

                        BusyIndicator {
                            anchors.horizontalCenter: parent.horizontalCenter
                            running: uniformityPanel.loading
                        }

                        Text {
                            anchors.horizontalCenter: parent.horizontalCenter
                            text: qsTr("正在加载均匀度对比数据，请稍候")
                            font.pixelSize: 15
                            font.family: "Microsoft YaHei"
                            color: "#7A8CA5"
                        }
                    }

                    Text {
                        anchors.centerIn: parent
                        visible: !uniformityPanel.loading
                                 && !uniformityPanel.hasChartData(uniformityPanel.bsChart)
                                 && !uniformityPanel.hasChartData(uniformityPanel.tChart)
                        text: qsTr("所选实验暂无可对比的均匀度数据")
                        font.pixelSize: 15
                        font.family: "Microsoft YaHei"
                        color: "#7A8CA5"
                    }

                    Item {
                        anchors.fill: parent
                        anchors.margins: 10
                        visible: uniformityPanel.hasChartData(uniformityPanel.bsChart)
                                 || uniformityPanel.hasChartData(uniformityPanel.tChart)

                        ColumnLayout {
                            anchors.fill: parent
                            spacing: 12
                            visible: uniformityPanel.currentModeIndex === 2

                            CompareTrendChart {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                Layout.preferredHeight: 180
                                chartTitle: uniformityPanel.tChart && uniformityPanel.tChart.title ? uniformityPanel.tChart.title : qsTr("透射光")
                                seriesList: uniformityPanel.tChart && uniformityPanel.tChart.seriesList ? uniformityPanel.tChart.seriesList : []
                                minXValue: comparePage.toNumber(uniformityPanel.tChart.chartMinX, 0)
                                maxXValue: comparePage.toNumber(uniformityPanel.tChart.chartMaxX, 1)
                                minYValue: comparePage.toNumber(uniformityPanel.tChart.chartMinY, 0)
                                maxYValue: comparePage.toNumber(uniformityPanel.tChart.chartMaxY, 1)
                                xAxisTickValues: uniformityPanel.tChart && uniformityPanel.tChart.xAxisTickValues ? uniformityPanel.tChart.xAxisTickValues : [0, 1]
                                yAxisLabels: uniformityPanel.tChart && uniformityPanel.tChart.yAxisLabels ? uniformityPanel.tChart.yAxisLabels : ["1.00", "0.00"]
                                yAxisTitle: "UI\n(T)"
                                showXAxisLabels: false
                                showXAxisTitle: false
                                formatXLabel: function(value) { return comparePage.formatNumber(value, 1) }
                            }

                            CompareTrendChart {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                Layout.preferredHeight: 180
                                chartTitle: uniformityPanel.bsChart && uniformityPanel.bsChart.title ? uniformityPanel.bsChart.title : qsTr("背射光")
                                seriesList: uniformityPanel.bsChart && uniformityPanel.bsChart.seriesList ? uniformityPanel.bsChart.seriesList : []
                                minXValue: comparePage.toNumber(uniformityPanel.bsChart.chartMinX, 0)
                                maxXValue: comparePage.toNumber(uniformityPanel.bsChart.chartMaxX, 1)
                                minYValue: comparePage.toNumber(uniformityPanel.bsChart.chartMinY, 0)
                                maxYValue: comparePage.toNumber(uniformityPanel.bsChart.chartMaxY, 1)
                                xAxisTickValues: uniformityPanel.bsChart && uniformityPanel.bsChart.xAxisTickValues ? uniformityPanel.bsChart.xAxisTickValues : [0, 1]
                                yAxisLabels: uniformityPanel.bsChart && uniformityPanel.bsChart.yAxisLabels ? uniformityPanel.bsChart.yAxisLabels : ["1.00", "0.00"]
                                yAxisTitle: "UI\n(BS)"
                                xAxisTitle: qsTr("时间(min)")
                                formatXLabel: function(value) { return comparePage.formatNumber(value, 1) }
                            }
                        }

                        CompareTrendChart {
                            anchors.fill: parent
                            visible: uniformityPanel.currentModeIndex !== 2
                            chartTitle: uniformityPanel.currentModeIndex === 0
                                        ? (uniformityPanel.bsChart && uniformityPanel.bsChart.title ? uniformityPanel.bsChart.title : qsTr("背射光"))
                                        : (uniformityPanel.tChart && uniformityPanel.tChart.title ? uniformityPanel.tChart.title : qsTr("透射光"))
                            seriesList: uniformityPanel.currentModeIndex === 0
                                        ? (uniformityPanel.bsChart && uniformityPanel.bsChart.seriesList ? uniformityPanel.bsChart.seriesList : [])
                                        : (uniformityPanel.tChart && uniformityPanel.tChart.seriesList ? uniformityPanel.tChart.seriesList : [])
                            minXValue: uniformityPanel.currentModeIndex === 0
                                       ? comparePage.toNumber(uniformityPanel.bsChart.chartMinX, 0)
                                       : comparePage.toNumber(uniformityPanel.tChart.chartMinX, 0)
                            maxXValue: uniformityPanel.currentModeIndex === 0
                                       ? comparePage.toNumber(uniformityPanel.bsChart.chartMaxX, 1)
                                       : comparePage.toNumber(uniformityPanel.tChart.chartMaxX, 1)
                            minYValue: uniformityPanel.currentModeIndex === 0
                                       ? comparePage.toNumber(uniformityPanel.bsChart.chartMinY, 0)
                                       : comparePage.toNumber(uniformityPanel.tChart.chartMinY, 0)
                            maxYValue: uniformityPanel.currentModeIndex === 0
                                       ? comparePage.toNumber(uniformityPanel.bsChart.chartMaxY, 1)
                                       : comparePage.toNumber(uniformityPanel.tChart.chartMaxY, 1)
                            xAxisTickValues: uniformityPanel.currentModeIndex === 0
                                             ? (uniformityPanel.bsChart && uniformityPanel.bsChart.xAxisTickValues ? uniformityPanel.bsChart.xAxisTickValues : [0, 1])
                                             : (uniformityPanel.tChart && uniformityPanel.tChart.xAxisTickValues ? uniformityPanel.tChart.xAxisTickValues : [0, 1])
                            yAxisLabels: uniformityPanel.currentModeIndex === 0
                                         ? (uniformityPanel.bsChart && uniformityPanel.bsChart.yAxisLabels ? uniformityPanel.bsChart.yAxisLabels : ["1.00", "0.00"])
                                         : (uniformityPanel.tChart && uniformityPanel.tChart.yAxisLabels ? uniformityPanel.tChart.yAxisLabels : ["1.00", "0.00"])
                            yAxisTitle: uniformityPanel.currentModeIndex === 0 ? "UI\n(BS)" : "UI\n(T)"
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
            seriesList: uniformityPanel.activeLegendSeries()
        }
    }
}
