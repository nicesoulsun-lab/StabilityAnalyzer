import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import ".."

//分层厚度曲线
Rectangle {
    id: separationPanel

    // 分层厚度页展示后端计算好的三区结果：
    // 澄清层、浓相层、沉淀层。
    property var detailPage
    readonly property var experimentData: detailPage ? detailPage.experimentData : ({})
    property int currentModeIndex: 3
    property var modeTitles: [qsTr("澄清层"), qsTr("浓相层"), qsTr("沉淀层"), qsTr("全部")]
    property var rows: []
    property var clarificationPoints: []
    property var concentratedPoints: []
    property var sedimentPoints: []
    property real chartMinX: 0
    property real chartMaxX: 1
    property real chartMinY: 0
    property real chartMaxY: 1
    property var xAxisTickValues: [0, 1]
    property var yAxisLabels: detailPage ? detailPage.makeAxisLabels(chartMinY, chartMaxY, 6, 1) : [0, 1]
    readonly property var latestRow: rows.length > 0 ? rows[rows.length - 1] : null

    color: "#FFFFFF"

    function formatLayerRange(lowerValue, upperValue) {
        if (!detailPage)
            return "--"

        var lower = detailPage.toNumber(lowerValue, Number.NaN)
        var upper = detailPage.toNumber(upperValue, Number.NaN)
        if (isNaN(lower) || isNaN(upper))
            return "--"

        return detailPage.formatNumber(lower, 1) + " - " + detailPage.formatNumber(upper, 1) + " mm"
    }

    function layerRangeSummary() {
        if (!detailPage || !latestRow)
            return qsTr("高度范围：--")

        var minHeight = detailPage.toNumber(detailPage.minHeightValue, 0)
        var maxHeight = detailPage.toNumber(detailPage.maxHeightValue, 0)
        var sedimentBoundary = detailPage.toNumber(latestRow.sediment_boundary_mm, minHeight)
        var clarificationBoundary = detailPage.toNumber(latestRow.clarification_boundary_mm, maxHeight)

        return qsTr("当前高度范围：澄清层 %1；浓相层 %2；沉淀层 %3")
                .arg(formatLayerRange(clarificationBoundary, maxHeight))
                .arg(formatLayerRange(sedimentBoundary, clarificationBoundary))
                .arg(formatLayerRange(minHeight, sedimentBoundary))
    }

    function loadSeparationData() {
        // 三层厚度的排序、折线点和坐标轴由后端统一生成。
        rows = []
        clarificationPoints = []
        concentratedPoints = []
        sedimentPoints = []
        if (!detailPage || !experimentData || experimentData.id === undefined || !data_ctrl)
            return

        var chartData = data_ctrl.getSeparationLayerChartData(Number(experimentData.id))
        if (!chartData || !chartData.rows || chartData.rows.length === 0)
            return

        rows = chartData.rows
        clarificationPoints = chartData.clarificationPoints
        concentratedPoints = chartData.concentratedPoints
        sedimentPoints = chartData.sedimentPoints
        chartMinX = detailPage.toNumber(chartData.chartMinX, 0)
        chartMaxX = detailPage.toNumber(chartData.chartMaxX, 1)
        chartMinY = detailPage.toNumber(chartData.chartMinY, 0)
        chartMaxY = detailPage.toNumber(chartData.chartMaxY, 1)
        xAxisTickValues = chartData.xAxisTickValues || [0, 1]
        yAxisLabels = chartData.yAxisLabels || detailPage.makeAxisLabels(chartMinY, chartMaxY, 6, 1)
    }

    onDetailPageChanged: loadSeparationData()
    Component.onCompleted: {
        if (detailPage)
            loadSeparationData()
    }

    Connections {
        target: detailPage
        function onExperimentDataChanged() { separationPanel.loadSeparationData() }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 18
        spacing: 16

        Row {
            spacing: 8
            Repeater {
                model: separationPanel.modeTitles
                delegate: Button {
                    width: 104
                    height: 28
                    text: modelData
                    onClicked: separationPanel.currentModeIndex = index
                    contentItem: Text {
                        text: parent.text
                        font.pixelSize: 12
                        font.family: "Microsoft YaHei"
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        color: separationPanel.currentModeIndex === index ? "#FFFFFF" : "#4A89DC"
                    }
                    background: Rectangle {
                        color: separationPanel.currentModeIndex === index ? "#4A89DC" : "#FFFFFF"
                        border.color: "#4A89DC"
                        border.width: 1
                    }
                }
            }
        }

        Text {
            Layout.fillWidth: true
            visible: separationPanel.rows.length > 0
            text: separationPanel.layerRangeSummary()
            wrapMode: Text.Wrap
            font.pixelSize: 12
            font.family: "Microsoft YaHei"
            color: "#5B6B7F"
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
                visible: separationPanel.rows.length === 0
                text: qsTr("数据库中暂无该实验的分层厚度数据")
                font.pixelSize: 15
                font.family: "Microsoft YaHei"
                color: "#7A8CA5"
            }

            Item {
                anchors.fill: parent
                anchors.margins: 12
                visible: separationPanel.rows.length > 0

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 12
                    visible: separationPanel.currentModeIndex === 3

                    TrendChart {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        Layout.preferredHeight: 138
                        dataPoints: separationPanel.clarificationPoints
                        lineColor: "#F39C12"
                        minXValue: separationPanel.chartMinX
                        maxXValue: separationPanel.chartMaxX
                        minYValue: separationPanel.chartMinY
                        maxYValue: separationPanel.chartMaxY
                        xAxisTickValues: separationPanel.xAxisTickValues
                        yAxisLabels: separationPanel.yAxisLabels
                        yAxisTitle: qsTr("澄清层\n(mm)")
                        showXAxisLabels: false
                        showXAxisTitle: false
                        formatXLabel: function(value) { return detailPage.formatNumber(value, 1) }
                    }

                    TrendChart {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        Layout.preferredHeight: 138
                        dataPoints: separationPanel.concentratedPoints
                        lineColor: "#27AE60"
                        minXValue: separationPanel.chartMinX
                        maxXValue: separationPanel.chartMaxX
                        minYValue: separationPanel.chartMinY
                        maxYValue: separationPanel.chartMaxY
                        xAxisTickValues: separationPanel.xAxisTickValues
                        yAxisLabels: separationPanel.yAxisLabels
                        yAxisTitle: qsTr("浓相层\n(mm)")
                        showXAxisLabels: false
                        showXAxisTitle: false
                        formatXLabel: function(value) { return detailPage.formatNumber(value, 1) }
                    }

                    TrendChart {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        Layout.preferredHeight: 138
                        dataPoints: separationPanel.sedimentPoints
                        lineColor: "#8E44AD"
                        minXValue: separationPanel.chartMinX
                        maxXValue: separationPanel.chartMaxX
                        minYValue: separationPanel.chartMinY
                        maxYValue: separationPanel.chartMaxY
                        xAxisTickValues: separationPanel.xAxisTickValues
                        yAxisLabels: separationPanel.yAxisLabels
                        yAxisTitle: qsTr("沉淀层\n(mm)")
                        xAxisTitle: qsTr("时间(min)")
                        formatXLabel: function(value) { return detailPage.formatNumber(value, 1) }
                    }
                }

                TrendChart {
                    anchors.fill: parent
                    visible: separationPanel.currentModeIndex !== 3
                    dataPoints: separationPanel.currentModeIndex === 0
                                ? separationPanel.clarificationPoints
                                : separationPanel.currentModeIndex === 1
                                  ? separationPanel.concentratedPoints
                                  : separationPanel.sedimentPoints
                    lineColor: separationPanel.currentModeIndex === 0
                               ? "#F39C12"
                               : separationPanel.currentModeIndex === 1
                                 ? "#27AE60"
                                 : "#8E44AD"
                    minXValue: separationPanel.chartMinX
                    maxXValue: separationPanel.chartMaxX
                    minYValue: separationPanel.chartMinY
                    maxYValue: separationPanel.chartMaxY
                    xAxisTickValues: separationPanel.xAxisTickValues
                    yAxisLabels: separationPanel.yAxisLabels
                    yAxisTitle: separationPanel.currentModeIndex === 0
                                ? qsTr("澄清层\n(mm)")
                                : separationPanel.currentModeIndex === 1
                                  ? qsTr("浓相层\n(mm)")
                                  : qsTr("沉淀层\n(mm)")
                    xAxisTitle: qsTr("时间(min)")
                    formatXLabel: function(value) { return detailPage.formatNumber(value, 1) }
                }
            }
        }
    }
}
