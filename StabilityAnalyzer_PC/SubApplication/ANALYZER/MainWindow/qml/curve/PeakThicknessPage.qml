import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import ".."

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
    property string channelLabel: intensityMode === 0 ? "BS" : "T"

    color: "#FFFFFF"

    function normalizedTextToNumber(textValue, fallback) {
        var parsed = Number(textValue)
        return isNaN(parsed) ? fallback : parsed
    }

    function buildScanGroups(sourceRows) {
        // 原始实验数据是一行一个高度点，这里先按 scan_id 还原成逐帧曲线。
        var groups = []
        var current = null
        for (var i = 0; i < sourceRows.length; ++i) {
            var row = sourceRows[i]
            var scanId = detailPage.toNumber(row.scan_id, 0)
            if (!current || current.scan_id !== scanId) {
                current = {
                    scan_id: scanId,
                    scan_elapsed_ms: detailPage.toNumber(row.scan_elapsed_ms, 0),
                    points: []
                }
                groups.push(current)
            }

            current.scan_elapsed_ms = Math.max(current.scan_elapsed_ms, detailPage.toNumber(row.scan_elapsed_ms, 0))
            current.points.push({
                height: detailPage.toNumber(row.height, 0) / 1000.0,
                backscatter: detailPage.toNumber(row.backscatter_intensity, 0),
                transmission: detailPage.toNumber(row.transmission_intensity, 0)
            })
        }

        for (var groupIndex = 0; groupIndex < groups.length; ++groupIndex) {
            groups[groupIndex].points.sort(function(a, b) {
                return a.height - b.height
            })
        }

        return groups
    }

    function filteredScanPoints(scanPoints, lowerBound, upperBound) {
        var filtered = []
        for (var i = 0; i < scanPoints.length; ++i) {
            if (scanPoints[i].height >= lowerBound && scanPoints[i].height <= upperBound)
                filtered.push(scanPoints[i])
        }
        return filtered
    }

    function signalValue(scanPoint) {
        return intensityMode === 0 ? scanPoint.backscatter : scanPoint.transmission
    }

    function computeThickness(referencePoints, currentPoints, thresholdValue) {
        // 当前实现取“最长连续超阈值区段”的高度宽度作为层厚度。
        var pairCount = Math.min(referencePoints.length, currentPoints.length)
        if (pairCount < 2)
            return 0

        var bestStart = -1
        var bestEnd = -1
        var segmentStart = -1

        for (var i = 0; i < pairCount; ++i) {
            var diff = Math.abs(signalValue(currentPoints[i]) - signalValue(referencePoints[i]))
            if (diff >= thresholdValue) {
                if (segmentStart < 0)
                    segmentStart = i
            } else if (segmentStart >= 0) {
                if (bestStart < 0 || currentPoints[i - 1].height - currentPoints[segmentStart].height > currentPoints[bestEnd].height - currentPoints[bestStart].height) {
                    bestStart = segmentStart
                    bestEnd = i - 1
                }
                segmentStart = -1
            }
        }

        if (segmentStart >= 0) {
            if (bestStart < 0 || currentPoints[pairCount - 1].height - currentPoints[segmentStart].height > currentPoints[bestEnd].height - currentPoints[bestStart].height) {
                bestStart = segmentStart
                bestEnd = pairCount - 1
            }
        }

        if (bestStart < 0 || bestEnd < bestStart)
            return 0

        return Math.max(0, currentPoints[bestEnd].height - currentPoints[bestStart].height)
    }

    function applyPeakThicknessParameters() {
        // 参数面板修改后整页重算一次，结果暂时不落库，便于后续继续迭代算法。
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
        var source = data_ctrl.getDataByExperiment(Number(experimentData.id))
        if (!source || source.length === 0)
            return

        var sorted = source.slice(0)
        sorted.sort(function(a, b) {
            var scanDiff = detailPage.toNumber(a.scan_id, 0) - detailPage.toNumber(b.scan_id, 0)
            if (scanDiff !== 0)
                return scanDiff
            var heightDiff = detailPage.toNumber(a.height, 0) - detailPage.toNumber(b.height, 0)
            if (heightDiff !== 0)
                return heightDiff
            return detailPage.toNumber(a.id, 0) - detailPage.toNumber(b.id, 0)
        })

        var groups = buildScanGroups(sorted)
        if (groups.length === 0)
            return

        var referencePoints = filteredScanPoints(groups[0].points, lowerBound, upperBound)
        if (referencePoints.length < 2)
            return

        var localRows = []
        var localPoints = []
        var maxThickness = 0

        for (var i = 0; i < groups.length; ++i) {
            var currentPoints = filteredScanPoints(groups[i].points, lowerBound, upperBound)
            var thickness = computeThickness(referencePoints, currentPoints, thresholdValue)
            var xValue = detailPage.toNumber(groups[i].scan_elapsed_ms, 0) / 60000.0

            localRows.push({
                scan_id: groups[i].scan_id,
                scan_elapsed_ms: groups[i].scan_elapsed_ms,
                layer_thickness_mm: thickness,
                channel_used: channelLabel,
                threshold: thresholdValue,
                height_lower_bound_mm: lowerBound,
                height_upper_bound_mm: upperBound
            })
            localPoints.push({ x: xValue, y: thickness })
            maxThickness = Math.max(maxThickness, thickness)
        }

        rows = localRows
        points = localPoints
        chartMinX = localPoints.length > 0 ? localPoints[0].x : 0
        chartMaxX = localPoints.length > 1 ? localPoints[localPoints.length - 1].x : chartMinX + 1
        if (chartMaxX <= chartMinX)
            chartMaxX = chartMinX + 1
        chartMinY = 0
        chartMaxY = Math.max(1, maxThickness * 1.12)
        xAxisTickValues = detailPage.buildTimeTicks(chartMinX, chartMaxX, 6)
        yAxisLabels = detailPage.makeAxisLabels(chartMinY, chartMaxY, 6, 1)
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
                            TextField {
                                id: lowerBoundField
                                width: 42
                                height: 28
                                text: detailPage.formatNumber(peakThicknessPanel.heightLowerBound, 0)
                                horizontalAlignment: Text.AlignHCenter
                                font.pixelSize: 12
                                font.family: "Microsoft YaHei"
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
                            TextField {
                                id: upperBoundField
                                width: 42
                                height: 28
                                text: detailPage.formatNumber(peakThicknessPanel.heightUpperBound, 0)
                                horizontalAlignment: Text.AlignHCenter
                                font.pixelSize: 12
                                font.family: "Microsoft YaHei"
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
                            TextField {
                                id: thresholdField
                                width: 42
                                height: 28
                                text: detailPage.formatNumber(peakThicknessPanel.peakThreshold, 0)
                                horizontalAlignment: Text.AlignHCenter
                                font.pixelSize: 12
                                font.family: "Microsoft YaHei"
                            }
                            Label {
                                text: "%"
                                font.pixelSize: 12
                                font.family: "Microsoft YaHei"
                                color: "#2F3A4A"
                            }
                        }

                        Button {
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
