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
    property var modeTitles: [qsTr("\u80cc\u5c04\u5149"), qsTr("\u900f\u5c04\u5149"), qsTr("\u80cc\u5c04\u5149/\u900f\u5c04\u5149")]
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
        // 曲线点和坐标轴都交由后端整理，页面只做模式切换。
        rows = []
        bsPoints = []
        tPoints = []
        if (!detailPage || !experimentData || experimentData.id === undefined || !data_ctrl)
            return

        var chartData = data_ctrl.getUniformityChartData(Number(experimentData.id))
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
        yAxisLabels = chartData.yAxisLabels || detailPage.makeAxisLabels(0, 1, 6, 2)
        console.log("[DetailCurve][uniformity]",
                    "experimentId=", Number(experimentData.id),
                    "rowCount=", rows.length,
                    "bsPoints=", bsPoints ? bsPoints.length : 0,
                    "tPoints=", tPoints ? tPoints.length : 0)
    }

    onDetailPageChanged: loadUniformityData()
    Component.onCompleted: {
        if (detailPage)
            loadUniformityData()
    }

    Connections {
        target: detailPage
        onExperimentDataChanged: uniformityPanel.loadUniformityData()
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
                text: qsTr("\u6570\u636e\u5e93\u4e2d\u6682\u65e0\u8be5\u5b9e\u9a8c\u7684\u5747\u5300\u5ea6\u6570\u636e")
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
