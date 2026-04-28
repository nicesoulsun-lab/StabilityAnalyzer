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
        // 平均值曲线的排序、取点和坐标轴范围都由后端统一整理。
        rows = []
        bsPoints = []
        tPoints = []
        if (!detailPage || !experimentData || experimentData.id === undefined || !data_ctrl)
            return

        var chartData = data_ctrl.getLightIntensityAverageChartData(Number(experimentData.id))
        if (!chartData || !chartData.rows || chartData.rows.length === 0)
            return

        rows = chartData.rows
        bsPoints = chartData.bsPoints
        tPoints = chartData.tPoints
        chartMinX = detailPage.toNumber(chartData.chartMinX, 0)
        chartMaxX = detailPage.toNumber(chartData.chartMaxX, 1)
        chartMinY = detailPage.toNumber(chartData.chartMinY, 0)
        chartMaxY = detailPage.toNumber(chartData.chartMaxY, 1)
        xAxisTickValues = chartData.xAxisTickValues || [0, 1]
        yAxisLabels = chartData.yAxisLabels || detailPage.makeAxisLabels(chartMinY, chartMaxY, 6, 1)
        console.log("[DetailCurve][average]",
                    "experimentId=", Number(experimentData.id),
                    "rowCount=", rows.length,
                    "bsPoints=", bsPoints ? bsPoints.length : 0,
                    "tPoints=", tPoints ? tPoints.length : 0)
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
