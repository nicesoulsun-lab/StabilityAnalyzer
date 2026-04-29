import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12

Rectangle {
    id: uniformityPanel
    color: "#FFFFFF"

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

    function loadRealtimeData() {
        rows = []
        bsPoints = []
        tPoints = []
        chartMinX = 0
        chartMaxX = 1
        chartMinY = 0
        chartMaxY = 1
        xAxisTickValues = [0, 1]
        yAxisLabels = detailPage ? detailPage.makeAxisLabels(0, 1, 6, 2) : [0, 1]

        if (!detailPage || !realtime_ctrl || !experimentData || experimentData.id === undefined)
            return

        var chartData = realtime_ctrl.getUniformityChartData(Number(experimentData.id))
        if (!chartData || !chartData.rows || chartData.rows.length === 0) {
            console.log("[RealtimeUniformity][empty]",
                        "experimentId=", Number(experimentData.id))
            return
        }

        rows = chartData.rows
        bsPoints = chartData.bsPoints || []
        tPoints = chartData.tPoints || []
        chartMinX = detailPage.toNumber(chartData.chartMinX, 0)
        chartMaxX = detailPage.toNumber(chartData.chartMaxX, 1)
        chartMinY = detailPage.toNumber(chartData.chartMinY, 0)
        chartMaxY = detailPage.toNumber(chartData.chartMaxY, 1)
        xAxisTickValues = chartData.xAxisTickValues || [0, 1]
        yAxisLabels = chartData.yAxisLabels || detailPage.makeAxisLabels(0, 1, 6, 2)

        console.log("[RealtimeUniformity][ready]",
                    "experimentId=", Number(experimentData.id),
                    "rowCount=", rows.length,
                    "bsPoints=", bsPoints.length,
                    "tPoints=", tPoints.length)
    }

    onDetailPageChanged: loadRealtimeData()
    Component.onCompleted: loadRealtimeData()

    Connections {
        target: detailPage
        ignoreUnknownSignals: true
        onExperimentDataChanged: uniformityPanel.loadRealtimeData()
        onLightCurvesVersionChanged: uniformityPanel.loadRealtimeData()
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
                text: qsTr("\u6682\u65e0\u5b9e\u65f6\u5747\u5300\u5ea6\u6570\u636e")
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
                        formatXLabel: function(value) { return detailPage ? detailPage.formatNumber(value, 1) : Number(value).toFixed(1) }
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
                        xAxisTitle: qsTr("\u65f6\u95f4(min)")
                        formatXLabel: function(value) { return detailPage ? detailPage.formatNumber(value, 1) : Number(value).toFixed(1) }
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
                    xAxisTitle: qsTr("\u65f6\u95f4(min)")
                    formatXLabel: function(value) { return detailPage ? detailPage.formatNumber(value, 1) : Number(value).toFixed(1) }
                }
            }
        }
    }
}
