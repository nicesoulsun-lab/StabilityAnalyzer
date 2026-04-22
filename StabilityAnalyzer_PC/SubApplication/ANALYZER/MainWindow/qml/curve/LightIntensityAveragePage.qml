import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import ".."


//光强平均值曲线
Rectangle {
    id: avgPanel

    // 光强平均值页和均匀度页结构类似，
    // 重点差别是 Y 轴范围要跟随当前实验数据自动放大。
    property var detailPage
    readonly property var experimentData: detailPage ? detailPage.experimentData : ({})
    property int currentModeIndex: 0
    property var modeTitles: [qsTr("背射光"), qsTr("透射光"), qsTr("背射光+透射光")]
    property var rows: []
    property var bsPoints: []
    property var tPoints: []
    property real chartMinX: 0
    property real chartMaxX: 1
    property real chartMinY: 0
    property real chartMaxY: 1
    property var xAxisTickValues: [0, 1]
    property var yAxisLabels: detailPage ? detailPage.makeAxisLabels(chartMinY, chartMaxY, 6, 1) : [0, 1]

    color: "#FFFFFF"

    function loadAverageData() {
        // 先收集 BS/T 两条曲线，再统一计算一套可复用的时间轴和 Y 轴范围。
        rows = []
        bsPoints = []
        tPoints = []
        if (!detailPage || !experimentData || experimentData.id === undefined || !data_ctrl)
            return

        var source = data_ctrl.getLightIntensityAverages(Number(experimentData.id))
        if (!source || source.length === 0)
            return

        var sorted = source.slice(0)
        sorted.sort(function(a, b) {
            return detailPage.toNumber(a.scan_elapsed_ms, 0) - detailPage.toNumber(b.scan_elapsed_ms, 0)
        })

        var localBs = []
        var localT = []
        var minY = Number.POSITIVE_INFINITY
        var maxY = Number.NEGATIVE_INFINITY
        for (var i = 0; i < sorted.length; ++i) {
            var xValue = detailPage.toNumber(sorted[i].scan_elapsed_ms, 0) / 60000.0
            var bsValue = detailPage.toNumber(sorted[i].avg_backscatter, 0)
            var tValue = detailPage.toNumber(sorted[i].avg_transmission, 0)
            localBs.push({ x: xValue, y: bsValue })
            localT.push({ x: xValue, y: tValue })
            minY = Math.min(minY, bsValue, tValue)
            maxY = Math.max(maxY, bsValue, tValue)
        }

        rows = sorted
        bsPoints = localBs
        tPoints = localT
        chartMinX = localBs.length > 0 ? localBs[0].x : 0
        chartMaxX = localBs.length > 1 ? localBs[localBs.length - 1].x : chartMinX + 1
        if (chartMaxX <= chartMinX)
            chartMaxX = chartMinX + 1
        chartMinY = isFinite(minY) ? detailPage.paddedMin(minY, maxY, 1) : 0
        chartMaxY = isFinite(maxY) ? detailPage.paddedMax(maxY, minY, 1) : 1
        xAxisTickValues = detailPage.buildTimeTicks(chartMinX, chartMaxX, 6)
        yAxisLabels = detailPage.makeAxisLabels(chartMinY, chartMaxY, 6, 1)
    }

    onDetailPageChanged: loadAverageData()
    Component.onCompleted: {
        if (detailPage)
            loadAverageData()
    }

    Connections {
        target: detailPage
        function onExperimentDataChanged() { avgPanel.loadAverageData() }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 18
        spacing: 16

        Row {
            spacing: 8
            Repeater {
                model: avgPanel.modeTitles
                delegate: Button {
                    width: 116
                    height: 28
                    text: modelData
                    onClicked: avgPanel.currentModeIndex = index
                    contentItem: Text {
                        text: parent.text
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

            Text {
                anchors.centerIn: parent
                visible: avgPanel.rows.length === 0
                text: qsTr("数据库中暂无该实验的光强平均值数据")
                font.pixelSize: 15
                font.family: "Microsoft YaHei"
                color: "#7A8CA5"
            }

            Item {
                anchors.fill: parent
                anchors.margins: 12
                visible: avgPanel.rows.length > 0

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 12
                    visible: avgPanel.currentModeIndex === 2

                    TrendChart {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        Layout.preferredHeight: 150
                        dataPoints: avgPanel.tPoints
                        lineColor: "#21A366"
                        minXValue: avgPanel.chartMinX
                        maxXValue: avgPanel.chartMaxX
                        minYValue: avgPanel.chartMinY
                        maxYValue: avgPanel.chartMaxY
                        xAxisTickValues: avgPanel.xAxisTickValues
                        yAxisLabels: avgPanel.yAxisLabels
                        yAxisTitle: "Avg\n(T)"
                        showXAxisLabels: false
                        showXAxisTitle: false
                        formatXLabel: function(value) { return detailPage.formatNumber(value, 1) }
                    }

                    TrendChart {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        Layout.preferredHeight: 150
                        dataPoints: avgPanel.bsPoints
                        lineColor: "#2F7CF6"
                        minXValue: avgPanel.chartMinX
                        maxXValue: avgPanel.chartMaxX
                        minYValue: avgPanel.chartMinY
                        maxYValue: avgPanel.chartMaxY
                        xAxisTickValues: avgPanel.xAxisTickValues
                        yAxisLabels: avgPanel.yAxisLabels
                        yAxisTitle: "Avg\n(BS)"
                        xAxisTitle: qsTr("时间(min)")
                        formatXLabel: function(value) { return detailPage.formatNumber(value, 1) }
                    }
                }

                TrendChart {
                    anchors.fill: parent
                    visible: avgPanel.currentModeIndex !== 2
                    dataPoints: avgPanel.currentModeIndex === 0 ? avgPanel.bsPoints : avgPanel.tPoints
                    lineColor: avgPanel.currentModeIndex === 0 ? "#2F7CF6" : "#21A366"
                    minXValue: avgPanel.chartMinX
                    maxXValue: avgPanel.chartMaxX
                    minYValue: avgPanel.chartMinY
                    maxYValue: avgPanel.chartMaxY
                    xAxisTickValues: avgPanel.xAxisTickValues
                    yAxisLabels: avgPanel.yAxisLabels
                    yAxisTitle: avgPanel.currentModeIndex === 0 ? "Avg\n(BS)" : "Avg\n(T)"
                    xAxisTitle: qsTr("时间(min)")
                    formatXLabel: function(value) { return detailPage.formatNumber(value, 1) }
                }
            }
        }
    }
}
