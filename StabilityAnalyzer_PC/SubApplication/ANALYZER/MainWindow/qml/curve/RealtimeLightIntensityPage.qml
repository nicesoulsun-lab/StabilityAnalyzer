import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import CustomComponents 1.0

Rectangle {
    id: realtimeLightPanel
    color: "#FFFFFF"

    property var detailPage
    readonly property var lightCurves: detailPage ? detailPage.lightCurves : []
    property int currentLightModeIndex: 0
    property var lightModeTitles: [qsTr("\u900f\u5c04\u5149"), qsTr("\u80cc\u5c04\u5149"), qsTr("\u900f\u5c04\u5149/\u80cc\u5c04\u5149")]
    readonly property int displayedCurveCount: lightCurves ? lightCurves.length : 0
    readonly property var availableHeightRange: curveHeightRange(lightCurves)
    readonly property real chartMinX: availableHeightRange.minValue
    readonly property real chartMaxX: Math.max(availableHeightRange.maxValue, availableHeightRange.minValue + 1)
    readonly property var xAxisTickValues: buildHeightTicks(chartMinX, chartMaxX)
    readonly property var transmissionRange: curveRange(lightCurves, "transmission_points")
    readonly property var backscatterRange: curveRange(lightCurves, "backscatter_points")
    readonly property var transmissionLabels: buildYLabels(transmissionRange.minValue, transmissionRange.maxValue)
    readonly property var backscatterLabels: buildYLabels(backscatterRange.minValue, backscatterRange.maxValue)
    readonly property bool transmissionOnlyMode: currentLightModeIndex === 0
    readonly property var singleModeRange: transmissionOnlyMode ? transmissionRange : backscatterRange
    readonly property var singleModeLabels: transmissionOnlyMode ? transmissionLabels : backscatterLabels

    function buildHeightTicks(minValue, maxValue) {
        var ticks = []
        var safeMin = Number(minValue)
        var safeMax = Number(maxValue)
        if (isNaN(safeMin) || isNaN(safeMax))
            return [0, 10]

        var span = Math.max(1, safeMax - safeMin)
        var step = span <= 10 ? 2 : span <= 20 ? 5 : 10
        var start = Math.floor(safeMin / step) * step
        var end = Math.ceil(safeMax / step) * step
        if (end <= start)
            end = start + step

        for (var value = start; value <= end + 0.0001; value += step) {
            if (value >= safeMin - 0.0001 && value <= safeMax + 0.0001)
                ticks.push(value)
        }

        if (ticks.length < 2)
            ticks = [safeMin, safeMax]
        return ticks
    }

    function pointX(point) {
        if (point === undefined || point === null)
            return Number.NaN
        if (point.x !== undefined)
            return Number(point.x)
        if (point.length !== undefined && point.length > 0)
            return Number(point[0])
        return Number.NaN
    }

    function curveHeightRange(curves) {
        var minValue = Number.POSITIVE_INFINITY
        var maxValue = Number.NEGATIVE_INFINITY

        for (var i = 0; i < curves.length; ++i) {
            var curve = curves[i]
            var rawMin = Number(curve.min_height_mm)
            var rawMax = Number(curve.max_height_mm)
            if (isFinite(rawMin))
                minValue = Math.min(minValue, rawMin)
            if (isFinite(rawMax))
                maxValue = Math.max(maxValue, rawMax)

            if (!isFinite(rawMin) || !isFinite(rawMax)) {
                var keys = ["transmission_points", "backscatter_points"]
                for (var keyIndex = 0; keyIndex < keys.length; ++keyIndex) {
                    var points = curve[keys[keyIndex]] || []
                    for (var pointIndex = 0; pointIndex < points.length; ++pointIndex) {
                        var x = pointX(points[pointIndex])
                        if (!isFinite(x))
                            continue
                        minValue = Math.min(minValue, x)
                        maxValue = Math.max(maxValue, x)
                    }
                }
            }
        }

        if (!isFinite(minValue) || !isFinite(maxValue)) {
            if (detailPage)
                return { minValue: detailPage.minHeightValue, maxValue: detailPage.maxHeightValue }
            return { minValue: 0, maxValue: 10 }
        }

        return { minValue: minValue, maxValue: maxValue }
    }

    function paddedRange(minValue, maxValue) {
        if (!isFinite(minValue) || !isFinite(maxValue))
            return { minValue: 0, maxValue: 100 }

        var safeMin = minValue
        var safeMax = maxValue
        if (Math.abs(safeMax - safeMin) < 0.000001) {
            safeMin -= 5
            safeMax += 5
        } else {
            var padding = Math.max((safeMax - safeMin) * 0.08, 1)
            safeMin -= padding
            safeMax += padding
        }
        return { minValue: safeMin, maxValue: safeMax }
    }

    function curveRange(curves, key) {
        var minValue = Number.POSITIVE_INFINITY
        var maxValue = Number.NEGATIVE_INFINITY
        for (var i = 0; i < curves.length; ++i) {
            minValue = Math.min(minValue, Number(curves[i][key === "transmission_points" ? "min_transmission" : "min_backscatter"]))
            maxValue = Math.max(maxValue, Number(curves[i][key === "transmission_points" ? "max_transmission" : "max_backscatter"]))
        }

        if (!isFinite(minValue) || !isFinite(maxValue))
            return { minValue: 0, maxValue: 100 }

        return { minValue: 0, maxValue: paddedRange(Math.max(0, minValue), Math.min(100, maxValue)).maxValue }
    }

    function buildYLabels(minValue, maxValue) {
        return detailPage ? detailPage.makeAxisLabels(minValue, maxValue, 6, 0) : [0, 20, 40, 60, 80, 100]
    }

    function displayedCurveAt(index) {
        if (!lightCurves || index < 0 || index >= lightCurves.length)
            return null
        return lightCurves[index]
    }

    function requestHeatLegendPaint(reason) {
        if (!heatLegendCanvas)
            return

        if (!heatLegendCanvas.visible || heatLegendCanvas.width <= 1 || heatLegendCanvas.height <= 1) {
            console.log("[RealtimeLightPage][legend skip]",
                        "reason=", reason,
                        "visible=", heatLegendCanvas.visible,
                        "width=", heatLegendCanvas.width,
                        "height=", heatLegendCanvas.height,
                        "curveCount=", displayedCurveCount)
            return
        }

        heatLegendCanvas.requestPaint()
    }

    function logCurveState(reason) {
        console.log("[RealtimeLightPage][state]",
                    "reason=", reason,
                    "experimentId=", detailPage && detailPage.experimentData ? Number(detailPage.experimentData.id || 0) : 0,
                    "curveCount=", displayedCurveCount,
                    "modeIndex=", currentLightModeIndex)
    }

    onDetailPageChanged: {
        logCurveState("detailPageChanged")
        requestHeatLegendPaint("detailPageChanged")
    }
    onCurrentLightModeIndexChanged: requestHeatLegendPaint("modeChanged")

    Component.onCompleted: {
        logCurveState("completed")
        requestHeatLegendPaint("completed")
    }

    Connections {
        target: detailPage
        ignoreUnknownSignals: true
        onLightCurvesVersionChanged: {
            realtimeLightPanel.logCurveState("lightCurvesVersionChanged")
            realtimeLightPanel.requestHeatLegendPaint("lightCurvesVersionChanged")
        }
        onExperimentDataChanged: {
            realtimeLightPanel.logCurveState("experimentDataChanged")
            realtimeLightPanel.requestHeatLegendPaint("experimentDataChanged")
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.leftMargin: 18
        anchors.topMargin: 10
        anchors.rightMargin: 10
        anchors.bottomMargin: 5
        spacing: 16

        Row {
            spacing: 8

            Repeater {
                model: realtimeLightPanel.lightModeTitles

                delegate: Button {
                    id: lightModeButton
                    width: 116
                    height: 28
                    text: modelData
                    onClicked: realtimeLightPanel.currentLightModeIndex = index

                    contentItem: Text {
                        text: lightModeButton.text
                        font.pixelSize: 12
                        font.family: "Microsoft YaHei"
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        color: realtimeLightPanel.currentLightModeIndex === index ? "#FFFFFF" : "#4A89DC"
                    }

                    background: Rectangle {
                        color: realtimeLightPanel.currentLightModeIndex === index ? "#4A89DC" : "#FFFFFF"
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
                visible: realtimeLightPanel.displayedCurveCount === 0
                text: qsTr("\u7b49\u5f85\u5b9e\u65f6\u66f2\u7ebf\u6570\u636e")
                font.pixelSize: 15
                font.family: "Microsoft YaHei"
                color: "#7A8CA5"
            }

            RowLayout {
                anchors.fill: parent
                anchors.margins: 12
                spacing: 12
                visible: realtimeLightPanel.displayedCurveCount > 0

                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    spacing: 12

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        visible: realtimeLightPanel.currentLightModeIndex === 2
                        radius: 6
                        color: "#FFFFFF"
                        border.color: "#DCE6F2"
                        border.width: 1

                        Text {
                            anchors.left: parent.left
                            anchors.top: parent.top
                            anchors.leftMargin: 16
                            anchors.topMargin: 10
                            text: qsTr("\u900f\u5c04\u5149")
                            font.pixelSize: 12
                            font.family: "Microsoft YaHei"
                            color: "#4A5D75"
                            font.bold: true
                        }

                        Item {
                            anchors.fill: parent
                            anchors.leftMargin: 58
                            anchors.rightMargin: 16
                            anchors.topMargin: 36
                            anchors.bottomMargin: 45

                            Repeater {
                                model: realtimeLightPanel.transmissionLabels.length
                                Rectangle {
                                    width: parent.width
                                    height: 1
                                    color: "#EEF3F8"
                                    y: index * (parent.height / Math.max(realtimeLightPanel.transmissionLabels.length - 1, 1))
                                }
                            }

                            Repeater {
                                model: realtimeLightPanel.xAxisTickValues
                                Rectangle {
                                    width: 1
                                    height: parent.height
                                    color: "#EEF3F8"
                                    x: (modelData - realtimeLightPanel.chartMinX) / Math.max(realtimeLightPanel.chartMaxX - realtimeLightPanel.chartMinX, 10) * parent.width
                                }
                            }

                            Repeater {
                                model: realtimeLightPanel.displayedCurveCount
                                CurveItem {
                                    property var curveData: realtimeLightPanel.displayedCurveAt(index)
                                    anchors.fill: parent
                                    lineColor: curveData ? curveData.color : "#21A366"
                                    lineWidth: 2
                                    maxPoints: 480
                                    autoScale: false
                                    minXValue: realtimeLightPanel.chartMinX
                                    maxXValue: realtimeLightPanel.chartMaxX
                                    minYValue: realtimeLightPanel.transmissionRange.minValue
                                    maxYValue: realtimeLightPanel.transmissionRange.maxValue
                                    dataPoints: curveData ? (curveData.transmission_points || []) : []
                                }
                            }

                            Text {
                                anchors.left: parent.left
                                anchors.leftMargin: -50
                                anchors.verticalCenter: parent.verticalCenter
                                text: "T\n(%)"
                                font.pixelSize: 12
                                font.family: "Microsoft YaHei"
                                color: "#6E8096"
                                font.bold: true
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }

                            Repeater {
                                model: realtimeLightPanel.transmissionLabels
                                delegate: Text {
                                    anchors.right: parent.left
                                    anchors.rightMargin: 10
                                    y: index * (parent.height / Math.max(realtimeLightPanel.transmissionLabels.length - 1, 1)) - height / 2
                                    text: realtimeLightPanel.transmissionLabels[realtimeLightPanel.transmissionLabels.length - 1 - index]
                                    font.pixelSize: 11
                                    font.family: "Microsoft YaHei"
                                    color: "#7A8CA5"
                                }
                            }

                            Repeater {
                                model: realtimeLightPanel.xAxisTickValues
                                delegate: Text {
                                    y: parent.height + 6
                                    x: (modelData - realtimeLightPanel.chartMinX) / Math.max(realtimeLightPanel.chartMaxX - realtimeLightPanel.chartMinX, 10) * parent.width - width / 2
                                    text: String(Math.round(modelData))
                                    font.pixelSize: 11
                                    font.family: "Microsoft YaHei"
                                    color: "#7A8CA5"
                                }
                            }

                            Text {
                                anchors.horizontalCenter: parent.horizontalCenter
                                anchors.bottom: parent.bottom
                                anchors.bottomMargin: -38
                                text: qsTr("\u9ad8\u5ea6(mm)")
                                font.pixelSize: 12
                                font.family: "Microsoft YaHei"
                                color: "#6E8096"
                                font.bold: true
                            }
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        radius: 6
                        color: "#FFFFFF"
                        border.color: "#DCE6F2"
                        border.width: 1

                        Text {
                            anchors.left: parent.left
                            anchors.top: parent.top
                            anchors.leftMargin: 16
                            anchors.topMargin: 10
                            text: realtimeLightPanel.currentLightModeIndex === 2
                                  ? qsTr("\u80cc\u5c04\u5149")
                                  : (realtimeLightPanel.transmissionOnlyMode ? qsTr("\u900f\u5c04\u5149") : qsTr("\u80cc\u5c04\u5149"))
                            font.pixelSize: 12
                            font.family: "Microsoft YaHei"
                            color: "#4A5D75"
                            font.bold: true
                        }

                        Item {
                            anchors.fill: parent
                            anchors.leftMargin: 58
                            anchors.rightMargin: 16
                            anchors.topMargin: 36
                            anchors.bottomMargin: 45

                            Repeater {
                                model: realtimeLightPanel.singleModeLabels.length
                                Rectangle {
                                    width: parent.width
                                    height: 1
                                    color: "#EEF3F8"
                                    y: index * (parent.height / Math.max(realtimeLightPanel.singleModeLabels.length - 1, 1))
                                }
                            }

                            Repeater {
                                model: realtimeLightPanel.xAxisTickValues
                                Rectangle {
                                    width: 1
                                    height: parent.height
                                    color: "#EEF3F8"
                                    x: (modelData - realtimeLightPanel.chartMinX) / Math.max(realtimeLightPanel.chartMaxX - realtimeLightPanel.chartMinX, 10) * parent.width
                                }
                            }

                            Repeater {
                                model: realtimeLightPanel.displayedCurveCount
                                CurveItem {
                                    property var curveData: realtimeLightPanel.displayedCurveAt(index)
                                    anchors.fill: parent
                                    lineColor: curveData ? curveData.color : "#4A89DC"
                                    lineWidth: 2
                                    maxPoints: 480
                                    autoScale: false
                                    minXValue: realtimeLightPanel.chartMinX
                                    maxXValue: realtimeLightPanel.chartMaxX
                                    minYValue: realtimeLightPanel.currentLightModeIndex === 2
                                               ? realtimeLightPanel.backscatterRange.minValue
                                               : realtimeLightPanel.singleModeRange.minValue
                                    maxYValue: realtimeLightPanel.currentLightModeIndex === 2
                                               ? realtimeLightPanel.backscatterRange.maxValue
                                               : realtimeLightPanel.singleModeRange.maxValue
                                    dataPoints: curveData
                                                ? (realtimeLightPanel.currentLightModeIndex === 2
                                                   ? (curveData.backscatter_points || [])
                                                   : (realtimeLightPanel.transmissionOnlyMode
                                                      ? (curveData.transmission_points || [])
                                                      : (curveData.backscatter_points || [])))
                                                : []
                                }
                            }

                            Text {
                                anchors.left: parent.left
                                anchors.leftMargin: -50
                                anchors.verticalCenter: parent.verticalCenter
                                text: realtimeLightPanel.currentLightModeIndex === 2
                                      ? "BS\n(%)"
                                      : (realtimeLightPanel.transmissionOnlyMode ? "T\n(%)" : "BS\n(%)")
                                font.pixelSize: 12
                                font.family: "Microsoft YaHei"
                                color: "#6E8096"
                                font.bold: true
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }

                            Repeater {
                                model: realtimeLightPanel.currentLightModeIndex === 2
                                       ? realtimeLightPanel.backscatterLabels
                                       : realtimeLightPanel.singleModeLabels
                                delegate: Text {
                                    property var labelSource: realtimeLightPanel.currentLightModeIndex === 2
                                                              ? realtimeLightPanel.backscatterLabels
                                                              : realtimeLightPanel.singleModeLabels
                                    anchors.right: parent.left
                                    anchors.rightMargin: 10
                                    y: index * (parent.height / Math.max(labelSource.length - 1, 1)) - height / 2
                                    text: labelSource[labelSource.length - 1 - index]
                                    font.pixelSize: 11
                                    font.family: "Microsoft YaHei"
                                    color: "#7A8CA5"
                                }
                            }

                            Repeater {
                                model: realtimeLightPanel.xAxisTickValues
                                delegate: Text {
                                    y: parent.height + 6
                                    x: (modelData - realtimeLightPanel.chartMinX) / Math.max(realtimeLightPanel.chartMaxX - realtimeLightPanel.chartMinX, 10) * parent.width - width / 2
                                    text: String(Math.round(modelData))
                                    font.pixelSize: 11
                                    font.family: "Microsoft YaHei"
                                    color: "#7A8CA5"
                                }
                            }

                            Text {
                                anchors.horizontalCenter: parent.horizontalCenter
                                anchors.bottom: parent.bottom
                                anchors.bottomMargin: -38
                                text: qsTr("\u9ad8\u5ea6(mm)")
                                font.pixelSize: 12
                                font.family: "Microsoft YaHei"
                                color: "#6E8096"
                                font.bold: true
                            }
                        }
                    }
                }

                Rectangle {
                    Layout.preferredWidth: 96
                    Layout.fillHeight: true
                    radius: 6
                    color: "#FFFFFF"
                    border.color: "#DCE6F2"
                    border.width: 1

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 8
                        anchors.rightMargin: 8
                        anchors.topMargin: 12
                        anchors.bottomMargin: 12
                        spacing: 8

                        Text {
                            text: qsTr("\u65f6\u95f4\u989c\u8272\u5bf9\u7167")
                            font.pixelSize: 13
                            font.family: "Microsoft YaHei"
                            color: "#4A5D75"
                            font.bold: true
                        }

                        Item {
                            Layout.fillWidth: true
                            Layout.fillHeight: true

                            Item {
                                anchors.left: parent.left
                                anchors.top: parent.top
                                anchors.bottom: parent.bottom
                                width: parent.width - 26

                                Repeater {
                                    model: Math.min(14, realtimeLightPanel.displayedCurveCount)

                                    delegate: Item {
                                        width: parent.width
                                        height: 20
                                        property int markerCount: Math.min(14, realtimeLightPanel.displayedCurveCount)
                                        property int curveIndex: realtimeLightPanel.displayedCurveCount <= 1
                                                                 ? 0
                                                                 : Math.round(index * (realtimeLightPanel.displayedCurveCount - 1) / Math.max(markerCount - 1, 1))
                                        y: markerCount <= 1 ? 0 : index * (parent.height - height) / (markerCount - 1)

                                        Text {
                                            anchors.right: parent.right
                                            anchors.verticalCenter: parent.verticalCenter
                                            text: realtimeLightPanel.displayedCurveCount > 0
                                                  ? realtimeLightPanel.lightCurves[curveIndex].legend_text
                                                  : "--"
                                            font.pixelSize: 11
                                            font.family: "Microsoft YaHei"
                                            color: index === 0 || index === model - 1 || index % 3 === 0 ? "#2F3A4A" : "#BFC7D3"
                                            font.bold: index === 0 || index === model - 1 || index % 3 === 0
                                        }
                                    }
                                }
                            }

                            Canvas {
                                id: heatLegendCanvas
                                anchors.top: parent.top
                                anchors.right: parent.right
                                anchors.bottom: parent.bottom
                                width: 22

                                onVisibleChanged: realtimeLightPanel.requestHeatLegendPaint("visibleChanged")
                                onWidthChanged: realtimeLightPanel.requestHeatLegendPaint("widthChanged")
                                onHeightChanged: realtimeLightPanel.requestHeatLegendPaint("heightChanged")

                                onPaint: {
                                    if (width <= 1 || height <= 1 || !visible) {
                                        console.log("[RealtimeLightPage][legend paint skipped]",
                                                    "visible=", visible,
                                                    "width=", width,
                                                    "height=", height)
                                        return
                                    }

                                    var ctx = getContext("2d")
                                    if (!ctx)
                                        return
                                    ctx.clearRect(0, 0, width, height)
                                    if (!realtimeLightPanel.lightCurves || realtimeLightPanel.lightCurves.length === 0)
                                        return

                                    var gradient = ctx.createLinearGradient(0, 0, 0, height)
                                    var stopCount = Math.max(realtimeLightPanel.lightCurves.length - 1, 1)
                                    for (var i = 0; i < realtimeLightPanel.lightCurves.length; ++i)
                                        gradient.addColorStop(stopCount === 0 ? 0 : i / stopCount, realtimeLightPanel.lightCurves[i].color)

                                    ctx.fillStyle = gradient
                                    ctx.fillRect(0, 0, width, height)
                                    ctx.strokeStyle = "#DCE6F2"
                                    ctx.lineWidth = 1
                                    ctx.strokeRect(0.5, 0.5, width - 1, height - 1)
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
