import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import ".."
import "../component"

//峰厚度曲线
Rectangle {
    id: peakThicknessPanel

    // 峰厚度目前还是 QML 侧 MVP 算法：
    // 以首帧为参考，统计当前高度窗口内连续超阈值区段的厚度。
    property var detailPage
    readonly property var experimentData: detailPage ? detailPage.experimentData : ({})
    property var rows: []
    property var points: []
    property real chartMinX: 0
    property real chartMaxX: 1
    property real chartMinY: 0
    property real chartMaxY: 1
    property var xAxisTickValues: [0, 1]
    property var yAxisLabels: detailPage ? detailPage.makeAxisLabels(chartMinY, chartMaxY, 6, 1) : [0, 1]
    property int intensityMode: 0
    property real heightLowerBound: 0
    property real heightUpperBound: 55
    property real peakThreshold: 2

    color: "#FFFFFF"

    function normalizedTextToNumber(textValue, fallback) {
        var parsed = Number(textValue)
        return isNaN(parsed) ? fallback : parsed
    }

    function applyPeakThicknessParameters() {
        // 峰厚度的参考帧对比和超阈值区段识别都已迁回后端分析层。
        rows = []
        points = []
        chartMinX = 0
        chartMaxX = 1
        chartMinY = 0
        chartMaxY = 1
        xAxisTickValues = [0, 1]
        yAxisLabels = detailPage.makeAxisLabels(0, 1, 6, 1)

        if (!detailPage || !experimentData || experimentData.id === undefined || !data_ctrl)
            return

        var lowerBound = Math.max(0, heightLowerBound)
        var upperBound = Math.max(lowerBound, heightUpperBound)
        var thresholdValue = Math.max(0, peakThreshold)
        var chartData = data_ctrl.getPeakThicknessChartData(Number(experimentData.id), intensityMode, lowerBound, upperBound, thresholdValue)
        if (!chartData || !chartData.rows || chartData.rows.length === 0)
            return

        rows = chartData.rows
        points = chartData.points
        chartMinX = detailPage.toNumber(chartData.chartMinX, 0)
        chartMaxX = detailPage.toNumber(chartData.chartMaxX, 1)
        chartMinY = detailPage.toNumber(chartData.chartMinY, 0)
        chartMaxY = detailPage.toNumber(chartData.chartMaxY, 1)
        xAxisTickValues = chartData.xAxisTickValues || [0, 1]
        yAxisLabels = chartData.yAxisLabels || detailPage.makeAxisLabels(chartMinY, chartMaxY, 6, 1)
        console.log("[DetailCurve][peak thickness]",
                    "experimentId=", Number(experimentData.id),
                    "rowCount=", rows.length,
                    "pointCount=", points ? points.length : 0)
    }

    onDetailPageChanged: {
        if (detailPage) {
            heightLowerBound = detailPage.floorToStep(detailPage.minHeightValue, 1)
            heightUpperBound = detailPage.ceilToStep(detailPage.maxHeightValue, 1)
            applyPeakThicknessParameters()
        }
    }
    Component.onCompleted: {
        if (detailPage) {
            heightLowerBound = detailPage.floorToStep(detailPage.minHeightValue, 1)
            heightUpperBound = detailPage.ceilToStep(detailPage.maxHeightValue, 1)
            applyPeakThicknessParameters()
        }
    }

    Connections {
        target: detailPage
        function onExperimentDataChanged() {
            if (detailPage) {
                peakThicknessPanel.heightLowerBound = detailPage.floorToStep(detailPage.minHeightValue, 1)
                peakThicknessPanel.heightUpperBound = detailPage.ceilToStep(detailPage.maxHeightValue, 1)
                peakThicknessPanel.applyPeakThicknessParameters()
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 18
        spacing: 16

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            radius: 6
            color: "#F8FBFF"
            border.color: "#D8E4F0"
            border.width: 1

            Text {
                anchors.centerIn: parent
                visible: peakThicknessPanel.points.length === 0
                text: qsTr("数据库中暂无该实验的峰厚度数据")
                font.pixelSize: 15
                font.family: "Microsoft YaHei"
                color: "#7A8CA5"
            }

            Item {
                anchors.fill: parent
                anchors.margins: 12
                visible: peakThicknessPanel.points.length > 0

                TrendChart {
                    anchors.fill: parent
                    anchors.rightMargin: 190
                    dataPoints: peakThicknessPanel.points
                    lineColor: "#C8DD8E"
                    lineWidth: 3
                    minXValue: peakThicknessPanel.chartMinX
                    maxXValue: peakThicknessPanel.chartMaxX
                    minYValue: peakThicknessPanel.chartMinY
                    maxYValue: peakThicknessPanel.chartMaxY
                    xAxisTickValues: peakThicknessPanel.xAxisTickValues
                    yAxisLabels: peakThicknessPanel.yAxisLabels
                    yAxisTitle: qsTr("层厚度\n(mm)")
                    xAxisTitle: qsTr("时间")
                    formatXLabel: function(value) {
                        return "0h:" + (detailPage.toNumber(value, 0) < 10 ? "0" : "") + Math.round(detailPage.toNumber(value, 0)) + "m"
                    }
                }

                Rectangle {
                    width: 172
                    anchors.top: parent.top
                    anchors.right: parent.right
                    radius: 4
                    color: "#FFFFFF"
                    border.color: "#AFCDF0"
                    border.width: 1

                    Column {
                        id: operationArea
                        anchors.fill: parent
                        anchors.margins: 12
                        spacing: 10

                        Row {
                            spacing: 8
                            Label {
                                text: qsTr("光强类型:")
                                font.pixelSize: 12
                                font.family: "Microsoft YaHei"
                                color: "#2F3A4A"
                                width: 72
                                horizontalAlignment: Text.AlignLeft
                            }
                            ComboBox {
                                id: intensityTypeBox
                                width: 74
                                height: 28
                                model: [qsTr("BS"), qsTr("T")]
                                currentIndex: peakThicknessPanel.intensityMode

                                background: Rectangle {
                                    radius: 4
                                    color: "#FFFFFF"
                                    border.color: "#82C1F2"
                                    border.width: 1
                                }
                            }
                        }

                        Row {
                            spacing: 8
                            Label {
                                text: qsTr("高度下限:")
                                font.pixelSize: 12
                                font.family: "Microsoft YaHei"
                                color: "#2F3A4A"
                                width: 72
                                horizontalAlignment: Text.AlignLeft
                            }
                            LineEdit {
                                id: lowerBoundField
                                width: 42
                                height: 28
                                font.pixelSize: 12
                                text: detailPage.formatNumber(peakThicknessPanel.heightLowerBound, 0)
                                horizontalAlignment: Text.AlignHCenter
                                validator: IntValidator {
                                    bottom: detailPage ? Math.floor(detailPage.minHeightValue) : 0
                                    top: detailPage ? Math.ceil(detailPage.maxHeightValue) : 100
                                }
                            }

                            Label {
                                text: "mm"
                                font.pixelSize: 12
                                font.family: "Microsoft YaHei"
                                color: "#2F3A4A"
                            }
                        }

                        Row {
                            spacing: 8
                            Label {
                                text: qsTr("高度上限:")
                                font.pixelSize: 12
                                font.family: "Microsoft YaHei"
                                color: "#2F3A4A"
                                width: 72
                                horizontalAlignment: Text.AlignLeft
                            }
                            LineEdit {
                                id: upperBoundField
                                width: 42
                                height: 28
                                font.pixelSize: 12
                                text: detailPage.formatNumber(peakThicknessPanel.heightUpperBound, 0)
                                horizontalAlignment: Text.AlignHCenter
                                validator: IntValidator {
                                    bottom: detailPage ? Math.floor(detailPage.minHeightValue) : 0
                                    top: detailPage ? Math.ceil(detailPage.maxHeightValue) : 100
                                }
                            }
                            Label {
                                text: "mm"
                                font.pixelSize: 12
                                font.family: "Microsoft YaHei"
                                color: "#2F3A4A"
                            }
                        }

                        Row {
                            spacing: 8
                            Label {
                                text: qsTr("峰厚度阈值:")
                                font.pixelSize: 12
                                font.family: "Microsoft YaHei"
                                color: "#2F3A4A"
                                width: 72
                                horizontalAlignment: Text.AlignLeft
                            }
                            LineEdit {
                                id: thresholdField
                                width: 42
                                height: 28
                                font.pixelSize: 12
                                text: detailPage.formatNumber(peakThicknessPanel.peakThreshold, 0)
                                horizontalAlignment: Text.AlignHCenter
                                validator: IntValidator {
                                    bottom: detailPage ? Math.floor(detailPage.minHeightValue) : 0
                                    top: detailPage ? Math.ceil(detailPage.maxHeightValue) : 100
                                }
                            }
                            Label {
                                text: "%"
                                font.pixelSize: 12
                                font.family: "Microsoft YaHei"
                                color: "#2F3A4A"
                            }
                        }

                        IconButton {
                            width: 80
                            height: 30
                            text: qsTr("应用")
                            onClicked: {
                                peakThicknessPanel.intensityMode = intensityTypeBox.currentIndex
                                peakThicknessPanel.heightLowerBound = peakThicknessPanel.normalizedTextToNumber(lowerBoundField.text, peakThicknessPanel.heightLowerBound)
                                peakThicknessPanel.heightUpperBound = peakThicknessPanel.normalizedTextToNumber(upperBoundField.text, peakThicknessPanel.heightUpperBound)
                                peakThicknessPanel.peakThreshold = peakThicknessPanel.normalizedTextToNumber(thresholdField.text, peakThicknessPanel.peakThreshold)
                                if (peakThicknessPanel.heightUpperBound < peakThicknessPanel.heightLowerBound) {
                                    var tempValue = peakThicknessPanel.heightLowerBound
                                    peakThicknessPanel.heightLowerBound = peakThicknessPanel.heightUpperBound
                                    peakThicknessPanel.heightUpperBound = tempValue
                                }
                                lowerBoundField.text = detailPage.formatNumber(peakThicknessPanel.heightLowerBound, 0)
                                upperBoundField.text = detailPage.formatNumber(peakThicknessPanel.heightUpperBound, 0)
                                thresholdField.text = detailPage.formatNumber(peakThicknessPanel.peakThreshold, 0)
                                peakThicknessPanel.applyPeakThicknessParameters()
                            }

                            contentItem: Text {
                                text: parent.text
                                color: "#FFFFFF"
                                font.pixelSize: 12
                                font.family: "Microsoft YaHei"
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }

                            background: Rectangle {
                                radius: 4
                                color: "#4A89DC"
                            }
                        }
                    }
                }
            }
        }
    }
}
