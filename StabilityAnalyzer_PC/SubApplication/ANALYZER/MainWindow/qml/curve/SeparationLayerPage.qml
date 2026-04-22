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

    color: "#FFFFFF"

    function loadSeparationData() {
        // 数据库层已经输出每次扫描的三层厚度，这里只做排序和折线点转换。
        rows = []
        clarificationPoints = []
        concentratedPoints = []
        sedimentPoints = []
        if (!detailPage || !experimentData || experimentData.id === undefined || !data_ctrl)
            return

        var source = data_ctrl.getSeparationLayerData(Number(experimentData.id))
        if (!source || source.length === 0)
            return

        var sorted = source.slice(0)
        sorted.sort(function(a, b) {
            var elapsedDiff = detailPage.toNumber(a.scan_elapsed_ms, 0) - detailPage.toNumber(b.scan_elapsed_ms, 0)
            if (elapsedDiff !== 0)
                return elapsedDiff
            return detailPage.toNumber(a.scan_id, 0) - detailPage.toNumber(b.scan_id, 0)
        })

        var localClarification = []
        var localConcentrated = []
        var localSediment = []
        var minY = Number.POSITIVE_INFINITY
        var maxY = Number.NEGATIVE_INFINITY

        for (var i = 0; i < sorted.length; ++i) {
            var xValue = detailPage.toNumber(sorted[i].scan_elapsed_ms, 0) / 60000.0
            var clarificationValue = detailPage.toNumber(sorted[i].clarification_thickness_mm, 0)
            var concentratedValue = detailPage.toNumber(sorted[i].concentrated_phase_thickness_mm, 0)
            var sedimentValue = detailPage.toNumber(sorted[i].sediment_thickness_mm, 0)

            localClarification.push({ x: xValue, y: clarificationValue })
            localConcentrated.push({ x: xValue, y: concentratedValue })
            localSediment.push({ x: xValue, y: sedimentValue })

            minY = Math.min(minY, clarificationValue, concentratedValue, sedimentValue)
            maxY = Math.max(maxY, clarificationValue, concentratedValue, sedimentValue)
        }

        rows = sorted
        clarificationPoints = localClarification
        concentratedPoints = localConcentrated
        sedimentPoints = localSediment
        chartMinX = localClarification.length > 0 ? localClarification[0].x : 0
        chartMaxX = localClarification.length > 1 ? localClarification[localClarification.length - 1].x : chartMinX + 1
        if (chartMaxX <= chartMinX)
            chartMaxX = chartMinX + 1
        chartMinY = 0
        chartMaxY = isFinite(maxY) ? detailPage.paddedMax(maxY, minY, 1) : 1
        xAxisTickValues = detailPage.buildTimeTicks(chartMinX, chartMaxX, 6)
        yAxisLabels = detailPage.makeAxisLabels(chartMinY, chartMaxY, 6, 1)
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
