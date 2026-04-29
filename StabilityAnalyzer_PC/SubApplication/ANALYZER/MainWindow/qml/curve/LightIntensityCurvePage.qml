import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import CustomComponents 1.0
import "../component"


//光强曲线
Rectangle {
    id: lightIntensityPanel

    // 光强页保留原来的双图/单图布局，不走 TrendChart，
    // 这样可以尽量少动已有显示方式和被射/散射切换逻辑。
    property var detailPage
    readonly property var lightCurves: detailPage ? detailPage.lightCurves : []
    readonly property bool lightCurvesLoading: !!(detailPage && detailPage.lightCurvesLoading)
    property int currentLightModeIndex: 0
    property var lightModeTitles: [qsTr("\u900f\u5c04\u5149"), qsTr("\u80cc\u5c04\u5149"), qsTr("\u900f\u5c04\u5149/\u80cc\u5c04\u5149")]
    property int currentDataModeIndex: 0
    property var dataModeTitles: [qsTr("原始数据"), qsTr("参比数据")]
    property int referenceCurveIndex: 0
    property real heightLowerBound: detailPage ? detailPage.floorToStep(detailPage.minHeightValue, 1) : 0
    property real heightUpperBound: detailPage ? detailPage.ceilToStep(detailPage.maxHeightValue, 1) : 55
    property var processedCurves: []
    property string lastDisplaySignature: ""
    readonly property bool useDetailCurves: currentDataModeIndex === 0
                                                && isFullHeightRange()
                                                && lightCurves
                                                && lightCurves.length > 0
    readonly property var displayedCurves: useDetailCurves ? lightCurves : processedCurves
    readonly property int displayedCurveCount: displayedCurves ? displayedCurves.length : 0
    readonly property var availableHeightRange: curveHeightRange(lightCurves)

    readonly property real safeHeightLowerBound: {
        if (!detailPage)
            return 0
        return Math.max(availableHeightRange.minValue, Math.min(heightLowerBound, availableHeightRange.maxValue))
    }
    readonly property real safeHeightUpperBound: {
        if (!detailPage)
            return 10
        return Math.max(safeHeightLowerBound, Math.min(heightUpperBound, availableHeightRange.maxValue))
    }
    readonly property real chartMinX: safeHeightLowerBound
    readonly property real chartMaxX: Math.max(safeHeightUpperBound, safeHeightLowerBound + 1)
    readonly property var xAxisTickValues: buildHeightTicks(chartMinX, chartMaxX)
    readonly property var transmissionRange: curveRange(displayedCurves, "transmission_points")
    readonly property var backscatterRange: curveRange(displayedCurves, "backscatter_points")
    readonly property var transmissionLabels: buildYLabels(transmissionRange.minValue, transmissionRange.maxValue)
    readonly property var backscatterLabels: buildYLabels(backscatterRange.minValue, backscatterRange.maxValue)
    readonly property bool transmissionOnlyMode: currentLightModeIndex === 0
    readonly property var singleModeRange: transmissionOnlyMode ? transmissionRange : backscatterRange
    readonly property var singleModeLabels: transmissionOnlyMode ? transmissionLabels : backscatterLabels

    color: "#FFFFFF"

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

    function clampReferenceIndex(index) {
        if (!displayedCurves || displayedCurves.length <= 0)
            return 0
        return Math.max(0, Math.min(index, displayedCurves.length - 1))
    }

    function displayedCurveAt(index) {
        if (!displayedCurves || index < 0 || index >= displayedCurves.length)
            return null
        return displayedCurves[index]
    }

    function normalizeCurves(curves) {
        var normalizedCurves = []
        for (var i = 0; i < curves.length; ++i) {
            normalizedCurves.push({
                                      scan_id: curves[i].scan_id,
                                      timestamp: curves[i].timestamp,
                                      scan_elapsed_ms: curves[i].scan_elapsed_ms,
                                      point_count: curves[i].point_count,
                                      backscatter_points: curves[i].backscatter_points || [],
                                      transmission_points: curves[i].transmission_points || [],
                                      min_backscatter: curves[i].min_backscatter,
                                      max_backscatter: curves[i].max_backscatter,
                                      min_transmission: curves[i].min_transmission,
                                      max_transmission: curves[i].max_transmission,
                                      reference_scan_id: curves[i].reference_scan_id !== undefined ? curves[i].reference_scan_id : curves[i].scan_id,
                                      color: detailPage.curveColor(i, curves.length),
                                      legend_text: detailPage.formatElapsedTime(curves[i].scan_elapsed_ms)
                                  })
        }
        return normalizedCurves
    }

    function isFullHeightRange() {
        if (!detailPage)
            return false
        return Math.abs(safeHeightLowerBound - availableHeightRange.minValue) < 0.000001
                && Math.abs(safeHeightUpperBound - availableHeightRange.maxValue) < 0.000001
    }

    function paddedRange(minValue, maxValue) {
        if (!isFinite(minValue) || !isFinite(maxValue))
            return { minValue: 0, maxValue: 100 }

        var safeMin = minValue
        var safeMax = maxValue
        if (Math.abs(safeMax - safeMin) < 0.000001) {
            var fallbackPadding = currentDataModeIndex === 0 ? 5 : 1
            safeMin -= fallbackPadding
            safeMax += fallbackPadding
        } else {
            var padding = Math.max((safeMax - safeMin) * 0.08, currentDataModeIndex === 0 ? 1 : 0.5)
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
            return currentDataModeIndex === 0
                    ? { minValue: 0, maxValue: 100 }
        : { minValue: -5, maxValue: 5 }

        if (currentDataModeIndex === 0)
            return { minValue: 0, maxValue: paddedRange(Math.max(0, minValue), Math.min(100, maxValue)).maxValue }
        return paddedRange(minValue, maxValue)
    }

    function buildYLabels(minValue, maxValue) {
        return detailPage ? detailPage.makeAxisLabels(minValue, maxValue, 6, currentDataModeIndex === 0 ? 0 : 1) : [0, 1]
    }

    function requestHeatLegendPaint(reason) {
        if (!heatLegendCanvas)
            return

        if (!heatLegendCanvas.visible || heatLegendCanvas.width <= 1 || heatLegendCanvas.height <= 1) {
            console.log("[LightIntensity][heat legend skip]",
                        "reason=", reason,
                        "visible=", heatLegendCanvas.visible,
                        "width=", heatLegendCanvas.width,
                        "height=", heatLegendCanvas.height,
                        "curveCount=", displayedCurveCount)
            return
        }

        heatLegendCanvas.requestPaint()
    }

    function displaySignature() {
        if (!detailPage || !detailPage.experimentData || detailPage.experimentData.id === undefined)
            return ""

        return [
            Number(detailPage.experimentData.id),
            detailPage.lightCurvesVersion,
            currentLightModeIndex,
            currentDataModeIndex,
            referenceCurveIndex,
            Number(safeHeightLowerBound).toFixed(3),
            Number(safeHeightUpperBound).toFixed(3),
            lightCurves ? lightCurves.length : 0
        ].join("|")
    }

    function scheduleDisplayedCurvesReload() {
        displayedCurveReloadTimer.restart()
    }

    function applyAnalysisSettings(referenceIndex, lowerBound, upperBound) {
        referenceCurveIndex = Math.max(0, referenceIndex)
        heightLowerBound = Math.max(availableHeightRange.minValue, Math.min(lowerBound, availableHeightRange.maxValue))
        heightUpperBound = Math.max(heightLowerBound, Math.min(upperBound, availableHeightRange.maxValue))
        scheduleDisplayedCurvesReload()
    }

    function loadDisplayedCurves() {
        if (!detailPage || !detailPage.experimentData || detailPage.experimentData.id === undefined) {
            processedCurves = []
            lastDisplaySignature = ""
            return
        }
        if (!detail_ctrl) {
            processedCurves = []
            lastDisplaySignature = ""
            return
        }
        if (detailPage.lightCurvesLoading) {
            return
        }

        var signature = displaySignature()
        if (signature === lastDisplaySignature && displayedCurves && displayedCurves.length > 0)
            return
        lastDisplaySignature = signature

        // 默认原始视图直接复用详情页已加载的原始曲线，避免首次进入光强页重复查询。
        if (useDetailCurves) {
            processedCurves = []
            referenceCurveIndex = 0
            console.log("[LightIntensity][curve page display]",
                        "curveCount=", lightCurves.length,
                        "firstTransmissionPoints=", lightCurves.length > 0 ? lightCurves[0].transmission_points.length : 0,
                        "firstBackscatterPoints=", lightCurves.length > 0 ? lightCurves[0].backscatter_points.length : 0)
            return
        }

        // 其余情况统一走 data_ctrl，由后端分析层完成裁剪/参比处理。
        var referenceCurve = displayedCurveAt(clampReferenceIndex(referenceCurveIndex))
        var referenceScanId = referenceCurve ? Number(referenceCurve.scan_id) : 0
        var curves = detail_ctrl.getProcessedLightIntensityCurves(
                    Number(detailPage.experimentData.id),
                    referenceScanId,
                    safeHeightLowerBound,
                    safeHeightUpperBound,
                    currentDataModeIndex === 1)
        if (!curves || curves.length === 0) {
            if (lightCurves && lightCurves.length > 0) {
                processedCurves = []
                referenceCurveIndex = 0
                console.log("[LightIntensity][curve page fallback]",
                            "curveCount=", lightCurves.length)
            } else {
                processedCurves = []
            }
            return
        }

        var normalizedCurves = normalizeCurves(curves)
        processedCurves = normalizedCurves
        if (normalizedCurves.length > 0) {
            var actualReferenceScanId = Number(normalizedCurves[0].reference_scan_id)
            for (var index = 0; index < normalizedCurves.length; ++index) {
                if (Number(normalizedCurves[index].scan_id) === actualReferenceScanId) {
                    referenceCurveIndex = index
                    break
                }
            }
        }
    }

    function resetAnalysisDefaults() {
        if (!detailPage)
            return
        applyAnalysisSettings(0,
                              detailPage.floorToStep(availableHeightRange.minValue, 1),
                              detailPage.ceilToStep(availableHeightRange.maxValue, 1))
    }

    onDetailPageChanged: {
        lastDisplaySignature = ""
        resetAnalysisDefaults()
    }
    onCurrentDataModeIndexChanged: {
        lastDisplaySignature = ""
        scheduleDisplayedCurvesReload()
    }
    Component.onCompleted: scheduleDisplayedCurvesReload()

    Timer {
        id: displayedCurveReloadTimer
        interval: 0
        repeat: false
        onTriggered: lightIntensityPanel.loadDisplayedCurves()
    }

    Connections {
        target: detailPage
        ignoreUnknownSignals: true
        onExperimentDataChanged: {
            lightIntensityPanel.lastDisplaySignature = ""
            lightIntensityPanel.resetAnalysisDefaults()
        }
        onLightCurvesVersionChanged: {
            if (detailPage && !detailPage.lightCurvesLoading) {
                console.log("[LightIntensity][version changed]",
                            "curveCount=", lightCurves ? lightCurves.length : 0,
                            "loading=", detailPage.lightCurvesLoading,
                            "currentDataModeIndex=", currentDataModeIndex)
                lightIntensityPanel.lastDisplaySignature = ""
                lightIntensityPanel.resetAnalysisDefaults()
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.leftMargin: 18
        anchors.topMargin: 10
        anchors.rightMargin: 10
        anchors.bottomMargin: 5
        spacing: 16

        RowLayout {
            Layout.fillWidth: true
            spacing: 14

            Row {
                spacing: 8

                Repeater {
                    model: lightIntensityPanel.lightModeTitles

                    delegate: Button {
                        id: lightModeButton
                        width: 116
                        height: 28
                        text: modelData
                        onClicked: lightIntensityPanel.currentLightModeIndex = index

                        contentItem: Text {
                            text: lightModeButton.text
                            font.pixelSize: 12
                            font.family: "Microsoft YaHei"
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                            color: lightIntensityPanel.currentLightModeIndex === index ? "#FFFFFF" : "#4A89DC"
                        }

                        background: Rectangle {
                            color: lightIntensityPanel.currentLightModeIndex === index ? "#4A89DC" : "#FFFFFF"
                            border.color: "#4A89DC"
                            border.width: 1
                        }
                    }
                }
            }

            Row {
                spacing: 8

                Repeater {
                    model: lightIntensityPanel.dataModeTitles

                    delegate: Button {
                        id: dataModeButton
                        width: 88
                        height: 28
                        text: modelData
                        onClicked: lightIntensityPanel.currentDataModeIndex = index

                        contentItem: Text {
                            text: dataModeButton.text
                            font.pixelSize: 12
                            font.family: "Microsoft YaHei"
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                            color: lightIntensityPanel.currentDataModeIndex === index ? "#FFFFFF" : "#4A89DC"
                        }

                        background: Rectangle {
                            color: lightIntensityPanel.currentDataModeIndex === index ? "#4A89DC" : "#FFFFFF"
                            border.color: "#4A89DC"
                            border.width: 1
                        }
                    }
                }
            }

            Item { Layout.fillWidth: true }

            IconButton {
                Layout.preferredWidth: 110
                Layout.preferredHeight: 28
                text: qsTr("设置分析参数")
                onClicked: {
                    referenceCombo.currentIndex = lightIntensityPanel.clampReferenceIndex(lightIntensityPanel.referenceCurveIndex)
                    lowerBoundField.text = detailPage.formatNumber(lightIntensityPanel.heightLowerBound, 0)
                    upperBoundField.text = detailPage.formatNumber(lightIntensityPanel.heightUpperBound, 0)
                    analysisPopup.open()
                }

                contentItem: Text {
                    text: parent.text
                    font.pixelSize: 12
                    font.family: "Microsoft YaHei"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    color: "#FFFFFF"
                }

                background: Rectangle {
                    color: "#4A89DC"
                    radius: 2
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: "#F8FBFF"
            border.color: "#D8E4F0"
            border.width: 1
            radius: 6

            Item {
                anchors.fill: parent
                anchors.margins: 12

                Text {
                    anchors.centerIn: parent
                    visible: !lightIntensityPanel.lightCurvesLoading && displayedCurveCount === 0
                    text: qsTr("数据库中暂无该实验的光强曲线数据")
                    font.pixelSize: 15
                    font.family: "Microsoft YaHei"
                    color: "#7A8CA5"
                }

                Column {
                    anchors.centerIn: parent
                    spacing: 12
                    visible: lightIntensityPanel.lightCurvesLoading

                    BusyIndicator {
                        anchors.horizontalCenter: parent.horizontalCenter
                        running: lightIntensityPanel.lightCurvesLoading
                    }

                    Text {
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: qsTr("\u6b63\u5728\u52a0\u8f7d\u5149\u5f3a\u66f2\u7ebf\uff0c\u8bf7\u7a0d\u5019")
                        font.pixelSize: 15
                        font.family: "Microsoft YaHei"
                        color: "#7A8CA5"
                    }
                }

                RowLayout {
                    anchors.fill: parent
                    spacing: 14
                    visible: displayedCurveCount > 0 && !lightIntensityPanel.lightCurvesLoading

                    Item {
                        Layout.fillWidth: true
                        Layout.fillHeight: true

                        ColumnLayout {
                            anchors.fill: parent
                            spacing: 12
                            // 双通道模式仍沿用上下两张图，方便和现有界面保持一致。
                            visible: lightIntensityPanel.currentLightModeIndex === 2

                            Item {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                Layout.preferredHeight: 150

                                Rectangle {
                                    anchors.fill: parent
                                    radius: 6
                                    color: "#FFFFFF"
                                    border.color: "#DCE6F2"
                                    border.width: 1
                                }

                                Item {
                                    anchors.fill: parent
                                    anchors.leftMargin: 58
                                    anchors.rightMargin: 16
                                    anchors.topMargin: 16
                                    anchors.bottomMargin: 12

                                    Repeater {
                                        model: lightIntensityPanel.transmissionLabels.length
                                        Rectangle {
                                            width: parent.width
                                            height: 1
                                            color: "#EEF3F8"
                                            y: index * (parent.height / Math.max(lightIntensityPanel.transmissionLabels.length - 1, 1))
                                        }
                                    }

                                    Repeater {
                                        model: lightIntensityPanel.xAxisTickValues
                                        Rectangle {
                                            width: 1
                                            height: parent.height
                                            color: "#EEF3F8"
                                            x: (modelData - lightIntensityPanel.chartMinX) / Math.max(lightIntensityPanel.chartMaxX - lightIntensityPanel.chartMinX, 10) * parent.width
                                        }
                                    }

                                    Repeater {
                                        model: lightIntensityPanel.currentLightModeIndex === 2 ? displayedCurveCount : 0
                                        CurveItem {
                                            property var curveData: lightIntensityPanel.displayedCurveAt(index)
                                            anchors.fill: parent
                                            lineColor: curveData ? curveData.color : "#4A89DC"
                                            lineWidth: 2
                                            maxPoints: 480
                                            autoScale: false
                                            minXValue: lightIntensityPanel.chartMinX
                                            maxXValue: lightIntensityPanel.chartMaxX
                                            minYValue: lightIntensityPanel.transmissionRange.minValue
                                            maxYValue: lightIntensityPanel.transmissionRange.maxValue
                                            dataPoints: curveData ? curveData.transmission_points : []
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
                                        model: lightIntensityPanel.transmissionLabels
                                        delegate: Text {
                                            anchors.right: parent.left
                                            anchors.rightMargin: 10
                                            y: index * (parent.height / Math.max(lightIntensityPanel.transmissionLabels.length - 1, 1)) - height / 2
                                            text: lightIntensityPanel.transmissionLabels[lightIntensityPanel.transmissionLabels.length - 1 - index]
                                            font.pixelSize: 11
                                            font.family: "Microsoft YaHei"
                                            color: "#7A8CA5"
                                        }
                                    }
                                }
                            }

                            Item {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                Layout.preferredHeight: 170

                                Rectangle {
                                    anchors.fill: parent
                                    radius: 6
                                    color: "#FFFFFF"
                                    border.color: "#DCE6F2"
                                    border.width: 1
                                }

                                Item {
                                    anchors.fill: parent
                                    anchors.leftMargin: 58
                                    anchors.rightMargin: 16
                                    anchors.topMargin: 16
                                    anchors.bottomMargin: 45

                                    Repeater {
                                        model: lightIntensityPanel.backscatterLabels.length
                                        Rectangle {
                                            width: parent.width
                                            height: 1
                                            color: "#EEF3F8"
                                            y: index * (parent.height / Math.max(lightIntensityPanel.backscatterLabels.length - 1, 1))
                                        }
                                    }

                                    Repeater {
                                        model: lightIntensityPanel.xAxisTickValues
                                        Rectangle {
                                            width: 1
                                            height: parent.height
                                            color: "#EEF3F8"
                                            x: (modelData - lightIntensityPanel.chartMinX) / Math.max(lightIntensityPanel.chartMaxX - lightIntensityPanel.chartMinX, 10) * parent.width
                                        }
                                    }

                                    Repeater {
                                        model: lightIntensityPanel.currentLightModeIndex === 2 ? displayedCurveCount : 0
                                        CurveItem {
                                            property var curveData: lightIntensityPanel.displayedCurveAt(index)
                                            anchors.fill: parent
                                            lineColor: curveData ? curveData.color : "#4A89DC"
                                            lineWidth: 2
                                            maxPoints: 480
                                            autoScale: false
                                            minXValue: lightIntensityPanel.chartMinX
                                            maxXValue: lightIntensityPanel.chartMaxX
                                            minYValue: lightIntensityPanel.backscatterRange.minValue
                                            maxYValue: lightIntensityPanel.backscatterRange.maxValue
                                            dataPoints: curveData ? curveData.backscatter_points : []
                                        }
                                    }

                                    Repeater {
                                        model: lightIntensityPanel.xAxisTickValues
                                        delegate: Text {
                                            y: parent.height + 6
                                            x: (modelData - lightIntensityPanel.chartMinX) / Math.max(lightIntensityPanel.chartMaxX - lightIntensityPanel.chartMinX, 10) * parent.width - width / 2
                                            text: String(Math.round(modelData))
                                            font.pixelSize: 11
                                            font.family: "Microsoft YaHei"
                                            color: "#7A8CA5"
                                        }
                                    }

                                    Text {
                                        anchors.left: parent.left
                                        anchors.leftMargin: -50
                                        anchors.verticalCenter: parent.verticalCenter
                                        text: "BS\n(%)"
                                        font.pixelSize: 12
                                        font.family: "Microsoft YaHei"
                                        color: "#6E8096"
                                        font.bold: true
                                        horizontalAlignment: Text.AlignHCenter
                                        verticalAlignment: Text.AlignVCenter
                                    }

                                    Repeater {
                                        model: lightIntensityPanel.backscatterLabels
                                        delegate: Text {
                                            anchors.right: parent.left
                                            anchors.rightMargin: 10
                                            y: index * (parent.height / Math.max(lightIntensityPanel.backscatterLabels.length - 1, 1)) - height / 2
                                            text: lightIntensityPanel.backscatterLabels[lightIntensityPanel.backscatterLabels.length - 1 - index]
                                            font.pixelSize: 11
                                            font.family: "Microsoft YaHei"
                                            color: "#7A8CA5"
                                        }
                                    }

                                    Text {
                                        anchors.horizontalCenter: parent.horizontalCenter
                                        anchors.bottom: parent.bottom
                                        anchors.bottomMargin: -38
                                        text: qsTr("高度(mm)")
                                        font.pixelSize: 12
                                        font.family: "Microsoft YaHei"
                                        color: "#6E8096"
                                        font.bold: true
                                    }
                                }
                            }
                        }

                        Item {
                            anchors.fill: parent
                            visible: lightIntensityPanel.currentLightModeIndex !== 2

                            Rectangle {
                                anchors.fill: parent
                                radius: 6
                                color: "#FFFFFF"
                                border.color: "#DCE6F2"
                                border.width: 1
                            }

                            Item {
                                anchors.fill: parent
                                anchors.leftMargin: 58
                                anchors.rightMargin: 16
                                anchors.topMargin: 16
                                anchors.bottomMargin: 45

                                Repeater {
                                    model: lightIntensityPanel.singleModeLabels.length
                                    Rectangle {
                                        width: parent.width
                                        height: 1
                                        color: "#EEF3F8"
                                        y: index * (parent.height / Math.max(lightIntensityPanel.singleModeLabels.length - 1, 1))
                                    }
                                }

                                Repeater {
                                    model: lightIntensityPanel.xAxisTickValues
                                    Rectangle {
                                        width: 1
                                        height: parent.height
                                        color: "#EEF3F8"
                                        x: (modelData - lightIntensityPanel.chartMinX) / Math.max(lightIntensityPanel.chartMaxX - lightIntensityPanel.chartMinX, 10) * parent.width
                                    }
                                }

            Repeater {
                model: lightIntensityPanel.currentLightModeIndex !== 2 ? displayedCurveCount : 0
                CurveItem {
                    property var curveData: lightIntensityPanel.displayedCurveAt(index)
                    anchors.fill: parent
                    lineColor: curveData ? curveData.color : "#4A89DC"
                    lineWidth: 2
                    maxPoints: 480
                    autoScale: false
                    minXValue: lightIntensityPanel.chartMinX
                    maxXValue: lightIntensityPanel.chartMaxX
                    minYValue: lightIntensityPanel.singleModeRange.minValue
                    maxYValue: lightIntensityPanel.singleModeRange.maxValue
                    dataPoints: curveData
                                ? (lightIntensityPanel.transmissionOnlyMode ? curveData.transmission_points : curveData.backscatter_points)
                                : []
                }
            }

                                Text {
                                    anchors.left: parent.left
                                    anchors.leftMargin: -50
                                    anchors.verticalCenter: parent.verticalCenter
                                    text: lightIntensityPanel.transmissionOnlyMode ? "T\n(%)" : "BS\n(%)"
                                    font.pixelSize: 12
                                    font.family: "Microsoft YaHei"
                                    color: "#6E8096"
                                    font.bold: true
                                    horizontalAlignment: Text.AlignHCenter
                                    verticalAlignment: Text.AlignVCenter
                                }

                                Repeater {
                                    model: lightIntensityPanel.singleModeLabels
                                    delegate: Text {
                                        anchors.right: parent.left
                                        anchors.rightMargin: 10
                                        y: index * (parent.height / Math.max(lightIntensityPanel.singleModeLabels.length - 1, 1)) - height / 2
                                        text: lightIntensityPanel.singleModeLabels[lightIntensityPanel.singleModeLabels.length - 1 - index]
                                        font.pixelSize: 11
                                        font.family: "Microsoft YaHei"
                                        color: "#7A8CA5"
                                    }
                                }

                                Repeater {
                                    model: lightIntensityPanel.xAxisTickValues
                                    delegate: Text {
                                        y: parent.height + 6
                                        x: (modelData - lightIntensityPanel.chartMinX) / Math.max(lightIntensityPanel.chartMaxX - lightIntensityPanel.chartMinX, 10) * parent.width - width / 2
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
                                    text: qsTr("高度(mm)")
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
                                text: qsTr("时间颜色对照")
                                font.pixelSize: 13
                                font.family: "Microsoft YaHei"
                                color: "#4A5D75"
                                font.bold: true
                            }

                            Item {
                                Layout.fillWidth: true
                                Layout.fillHeight: true

                                // 右侧色带只做时间和颜色的对照，不参与图表缩放。
                                Item {
                                    anchors.left: parent.left
                                    anchors.top: parent.top
                                    anchors.bottom: parent.bottom
                                    width: parent.width - 26

                                    Repeater {
                                        model: Math.min(14, displayedCurves.length)

                                        delegate: Item {
                                            width: parent.width
                                            height: 20
                                            property int markerCount: Math.min(14, displayedCurves.length)
                                            property int curveIndex: displayedCurves.length <= 1 ? 0 : Math.round(index * (displayedCurves.length - 1) / Math.max(markerCount - 1, 1))
                                            y: markerCount <= 1 ? 0 : index * (parent.height - height) / (markerCount - 1)

                                            Text {
                                                anchors.right: parent.right
                                                anchors.verticalCenter: parent.verticalCenter
                                                text: displayedCurves.length > 0 ? displayedCurves[curveIndex].legend_text : "--"
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

                                    Connections {
                                        target: lightIntensityPanel
                                        onDisplayedCurvesChanged: lightIntensityPanel.requestHeatLegendPaint("displayedCurvesChanged")
                                    }

                                    onVisibleChanged: lightIntensityPanel.requestHeatLegendPaint("visibleChanged")
                                    onWidthChanged: lightIntensityPanel.requestHeatLegendPaint("widthChanged")
                                    onHeightChanged: lightIntensityPanel.requestHeatLegendPaint("heightChanged")

                                    onPaint: {
                                        if (width <= 1 || height <= 1 || !visible) {
                                            console.log("[LightIntensity][heat legend paint skipped]",
                                                        "visible=", visible,
                                                        "width=", width,
                                                        "height=", height)
                                            return
                                        }

                                        var ctx = getContext("2d")
                                        if (!ctx)
                                            return
                                        ctx.clearRect(0, 0, width, height)
                                        if (!displayedCurves || displayedCurves.length === 0)
                                            return
                                        var gradient = ctx.createLinearGradient(0, 0, 0, height)
                                        var stopCount = Math.max(displayedCurves.length - 1, 1)
                                        for (var i = 0; i < displayedCurves.length; ++i) {
                                            gradient.addColorStop(stopCount === 0 ? 0 : i / stopCount, displayedCurves[i].color)
                                        }
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

    Popup {
        id: analysisPopup
        anchors.centerIn: Overlay.overlay
        width: 380
        height: 430
        modal: true
        focus: true
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
        padding: 0

        background: Rectangle {
            radius: 2
            color: "#FFFFFF"
            border.color: "#D9E4F2"
            border.width: 1
        }

        ColumnLayout {
            anchors.fill: parent
            spacing: 0

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 44
                color: "#F7F9FC"

                Text {
                    anchors.left: parent.left
                    anchors.leftMargin: 16
                    anchors.verticalCenter: parent.verticalCenter
                    text: qsTr("分析参数")
                    font.pixelSize: 14
                    font.family: "Microsoft YaHei"
                    color: "#2F3A4A"
                }

                IconButton {
                    id: closeButton
                    anchors.right: parent.right
                    anchors.rightMargin: 12
                    anchors.verticalCenter: parent.verticalCenter
                    width: 24
                    height: 24
                    text: "×"
                    onClicked: analysisPopup.close()

                    contentItem: Text {
                        text: parent.text
                        font.pixelSize: 20
                        color: "#2F3A4A"
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }

                    background: Rectangle {
                        radius: 14
                        color: closeButton.hovered ? "#E8EDF4" : "transparent"
                    }
                }
            }

            Column {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 18
                padding: 24

                Row {
                    spacing: 18

                    Text {
                        width: 78
                        anchors.verticalCenter: parent.verticalCenter
                        text: qsTr("\u53c2\u6bd4\u7ebf")
                        font.pixelSize: 13
                        font.family: "Microsoft YaHei"
                        //font.bold: true
                        color: "#2F3A4A"
                    }

                    ComboBox {
                        id: referenceCombo
                        width: 210
                        height: 38
                        model: lightIntensityPanel.displayedCurves.length > 0 ? lightIntensityPanel.displayedCurves.map(function(curve) { return curve.legend_text }) : []
                        currentIndex: lightIntensityPanel.clampReferenceIndex(lightIntensityPanel.referenceCurveIndex)

                        background: Rectangle {
                            radius: 4
                            color: "#FFFFFF"
                            border.color: "#82C1F2"
                            border.width: 1
                        }

                        contentItem: Text {
                            text: referenceCombo.displayText
                            leftPadding: 12
                            verticalAlignment: Text.AlignVCenter
                            font.family: "Microsoft YaHei"
                            color: "#333333"
                        }
                    }
                }

                Rectangle {
                    width: analysisPopup.width - 48
                    height: 1
                    color: "#E6EDF5"
                }

                Row {
                    spacing: 18

                    Text {
                        width: 78
                        anchors.verticalCenter: parent.verticalCenter
                        text: qsTr("高度区间")
                        font.pixelSize: 13
                        font.family: "Microsoft YaHei"
                        //font.bold: true
                        color: "#2F3A4A"
                    }

                    LineEdit {
                        id: lowerBoundField
                        width: 60
                        height: 38
                        font.pixelSize: 14
                        horizontalAlignment: Text.AlignHCenter
                        validator: IntValidator {
                            bottom: detailPage ? Math.floor(detailPage.minHeightValue) : 0
                            top: detailPage ? Math.ceil(detailPage.maxHeightValue) : 100
                        }
                    }

                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        text: "mm"
                        font.pixelSize: 13
                        font.family: "Microsoft YaHei"
                        color: "#2F3A4A"
                    }

                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        text: "~"
                        font.pixelSize: 14
                        font.family: "Microsoft YaHei"
                        color: "#2F3A4A"
                    }

                    LineEdit {
                        id: upperBoundField
                        width: 60
                        height: 38
                        font.pixelSize: 14
                        horizontalAlignment: Text.AlignHCenter
                        validator: IntValidator {
                            bottom: detailPage ? Math.floor(detailPage.minHeightValue) : 0
                            top: detailPage ? Math.ceil(detailPage.maxHeightValue) : 100
                        }
                    }

                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        text: "mm"
                        font.pixelSize: 13
                        font.family: "Microsoft YaHei"
                        color: "#2F3A4A"
                    }
                }

                Rectangle {
                    width: analysisPopup.width - 48
                    height: 1
                    color: "#E6EDF5"
                }

                Row {
                    spacing: 2

                    CheckBox {
                        id: heightCheckBox
                        scale: 0.65
                    }

                    Text {
                        width: 78
                        anchors.verticalCenter: parent.verticalCenter
                        text: qsTr("\u81ea\u5b9a\u4e49\u9ad8\u5ea6\u5206\u6bb5")
                        font.pixelSize: 13
                        font.family: "Microsoft YaHei"
                        color: "#2F3A4A"
                    }
                }

                IconButton {
                    anchors.horizontalCenter: parent.horizontalCenter
                    width: 108
                    height: 40
                    button_text: qsTr("确认")
                    onClicked: {
                        var lowerValue = lightIntensityPanel.detailPage.toNumber(lowerBoundField.text, lightIntensityPanel.heightLowerBound)
                        var upperValue = lightIntensityPanel.detailPage.toNumber(upperBoundField.text, lightIntensityPanel.heightUpperBound)
                        if (upperValue < lowerValue) {
                            var tempValue = lowerValue
                            lowerValue = upperValue
                            upperValue = tempValue
                        }

                        lightIntensityPanel.applyAnalysisSettings(
                                    referenceCombo.currentIndex,
                                    lowerValue,
                                    upperValue)
                        analysisPopup.close()
                    }

                    button_color: "#4A89DC"
                    text_color: "#FFFFFF"
                }
            }
        }
    }
}
