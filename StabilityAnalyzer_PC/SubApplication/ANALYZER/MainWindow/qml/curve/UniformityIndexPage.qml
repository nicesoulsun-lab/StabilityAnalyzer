import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import ".."

//均匀度曲线
Rectangle {
    id: uniformityPanel

    // 均匀度是后端直接给出的时间序列，页面只负责通道切换和统一图表展示。
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
    property var yAxisLabels: detailPage ? detailPage.makeAxisLabels(0, 1, 6, 2) : [0, 1]

    color: "#FFFFFF"

    function loadUniformityData() {
        // 这里不再复算业务指标，只做时间排序和折线点整理。
        rows = []
        bsPoints = []
        tPoints = []
        if (!detailPage || !experimentData || experimentData.id === undefined || !data_ctrl)
            return

        var source = data_ctrl.getUniformityIndices(Number(experimentData.id))
        if (!source || source.length === 0)
            return

        var sorted = source.slice(0)
        sorted.sort(function(a, b) {
            return detailPage.toNumber(a.scan_elapsed_ms, 0) - detailPage.toNumber(b.scan_elapsed_ms, 0)
        })

        var localBs = []
        var localT = []
        for (var i = 0; i < sorted.length; ++i) {
            var xValue = detailPage.toNumber(sorted[i].scan_elapsed_ms, 0) / 60000.0
            localBs.push({ x: xValue, y: detailPage.toNumber(sorted[i].ui_backscatter, 0) })
            localT.push({ x: xValue, y: detailPage.toNumber(sorted[i].ui_transmission, 0) })
        }

        rows = sorted
        bsPoints = localBs
        tPoints = localT
        chartMinX = localBs.length > 0 ? localBs[0].x : 0
        chartMaxX = localBs.length > 1 ? localBs[localBs.length - 1].x : chartMinX + 1
        if (chartMaxX <= chartMinX)
            chartMaxX = chartMinX + 1
        chartMinY = 0
        chartMaxY = 1
        xAxisTickValues = detailPage.buildTimeTicks(chartMinX, chartMaxX, 6)
        yAxisLabels = detailPage.makeAxisLabels(0, 1, 6, 2)
    }

    onDetailPageChanged: loadUniformityData()
    Component.onCompleted: {
        if (detailPage)
            loadUniformityData()
    }

    Connections {
        target: detailPage
        function onExperimentDataChanged() { uniformityPanel.loadUniformityData() }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 18
        spacing: 16

        Row {
            spacing: 8
            Repeater {
                model: uniformityPanel.modeTitles
                delegate: Button {
                    width: 116
                    height: 28
                    text: modelData
                    onClicked: uniformityPanel.currentModeIndex = index
                    contentItem: Text {
                        text: parent.text
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

            Text {
                anchors.centerIn: parent
                visible: uniformityPanel.rows.length === 0
                text: qsTr("数据库中暂无该实验的均匀度数据")
                font.pixelSize: 15
                font.family: "Microsoft YaHei"
                color: "#7A8CA5"
            }

            Item {
                anchors.fill: parent
                anchors.margins: 12
                visible: uniformityPanel.rows.length > 0

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 12
                    visible: uniformityPanel.currentModeIndex === 2

                    TrendChart {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        Layout.preferredHeight: 150
                        dataPoints: uniformityPanel.tPoints
                        lineColor: "#21A366"
                        minXValue: uniformityPanel.chartMinX
                        maxXValue: uniformityPanel.chartMaxX
                        minYValue: 0
                        maxYValue: 1
                        xAxisTickValues: uniformityPanel.xAxisTickValues
                        yAxisLabels: uniformityPanel.yAxisLabels
                        yAxisTitle: "UI\n(T)"
                        showXAxisLabels: false
                        showXAxisTitle: false
                        formatXLabel: function(value) { return detailPage.formatNumber(value, 1) }
                    }

                    TrendChart {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        Layout.preferredHeight: 150
                        dataPoints: uniformityPanel.bsPoints
                        lineColor: "#2F7CF6"
                        minXValue: uniformityPanel.chartMinX
                        maxXValue: uniformityPanel.chartMaxX
                        minYValue: 0
                        maxYValue: 1
                        xAxisTickValues: uniformityPanel.xAxisTickValues
                        yAxisLabels: uniformityPanel.yAxisLabels
                        yAxisTitle: "UI\n(BS)"
                        xAxisTitle: qsTr("时间(min)")
                        formatXLabel: function(value) { return detailPage.formatNumber(value, 1) }
                    }
                }

                TrendChart {
                    anchors.fill: parent
                    visible: uniformityPanel.currentModeIndex !== 2
                    dataPoints: uniformityPanel.currentModeIndex === 0 ? uniformityPanel.bsPoints : uniformityPanel.tPoints
                    lineColor: uniformityPanel.currentModeIndex === 0 ? "#2F7CF6" : "#21A366"
                    minXValue: uniformityPanel.chartMinX
                    maxXValue: uniformityPanel.chartMaxX
                    minYValue: 0
                    maxYValue: 1
                    xAxisTickValues: uniformityPanel.xAxisTickValues
                    yAxisLabels: uniformityPanel.yAxisLabels
                    yAxisTitle: uniformityPanel.currentModeIndex === 0 ? "UI\n(BS)" : "UI\n(T)"
                    xAxisTitle: qsTr("时间(min)")
                    formatXLabel: function(value) { return detailPage.formatNumber(value, 1) }
                }
            }
        }
    }
}
