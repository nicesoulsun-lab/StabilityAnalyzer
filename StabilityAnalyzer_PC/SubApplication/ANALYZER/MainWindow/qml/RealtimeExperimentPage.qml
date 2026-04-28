import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import "component"

Item {
    id: realtimePage
    objectName: "RealtimeExperimentPage"

    property int channelIndex: -1
    property var experimentData: ({})
    property int currentTabIndex: 0
    property var lightCurves: []
    property real minHeightValue: 0
    property real maxHeightValue: 10
    property real minLightValue: 0
    property real maxLightValue: 100
    property int lightCurveCount: 0
    property int lightCurvesVersion: 0
    property string lastRealtimeCurveSignature: ""
    property int maxVisibleCurveCount: 48

    signal backRequested()

    readonly property int experimentId: experimentData && experimentData.id !== undefined ? Number(experimentData.id) : 0
    readonly property var realtimeTabs: [
        { title: qsTr("光强") },
        { title: qsTr("不稳定性") },
        { title: qsTr("均匀度指数") }
    ]

    function openTab(tabIndex) {
        currentTabIndex = tabIndex
    }

    function toNumber(value, fallback) {
        var parsed = Number(value)
        return isNaN(parsed) ? fallback : parsed
    }

    function formatNumber(value, digits) {
        var parsed = Number(value)
        if (isNaN(parsed))
            return "--"
        return parsed.toFixed(digits === undefined ? 2 : digits)
    }

    function formatElapsedTime(elapsedMs) {
        var totalMinutes = Math.max(0, Math.round(toNumber(elapsedMs, 0) / 60000))
        var days = Math.floor(totalMinutes / (24 * 60))
        var hours = Math.floor((totalMinutes % (24 * 60)) / 60)
        var minutes = totalMinutes % 60

        function pad(value) {
            return value < 10 ? "0" + value : String(value)
        }

        return pad(days) + "d:" + pad(hours) + "h:" + pad(minutes) + "m"
    }

    function curveColor(index, total) {
        if (total <= 1)
            return Qt.hsla(0.65, 0.85, 0.48, 1.0)

        var ratio = index / (total - 1)
        var hue = 0.65 * (1.0 - ratio)
        return Qt.hsla(hue, 0.9, 0.5, 1.0)
    }

    function floorToStep(value, step) {
        var actualStep = step <= 0 ? 1 : step
        return Math.floor(toNumber(value, 0) / actualStep) * actualStep
    }

    function ceilToStep(value, step) {
        var actualStep = step <= 0 ? 1 : step
        return Math.ceil(toNumber(value, 0) / actualStep) * actualStep
    }

    function makeAxisLabels(minValue, maxValue, count, digits) {
        var labels = []
        var labelCount = count || 5
        if (labelCount <= 1)
            labelCount = 2

        var minNumber = toNumber(minValue, 0)
        var maxNumber = toNumber(maxValue, minNumber)
        var range = maxNumber - minNumber
        if (Math.abs(range) < 0.000001)
            range = Math.max(1, Math.abs(maxNumber) * 0.1)

        for (var i = 0; i < labelCount; ++i) {
            var ratio = labelCount === 1 ? 0 : i / (labelCount - 1)
            labels.push(formatNumber(minNumber + range * ratio, digits === undefined ? 1 : digits))
        }
        return labels
    }

    function buildTimeTicks(minValue, maxValue, count) {
        var ticks = []
        var tickCount = Math.max(2, count || 6)
        var minNumber = toNumber(minValue, 0)
        var maxNumber = toNumber(maxValue, minNumber + 1)
        if (Math.abs(maxNumber - minNumber) < 0.000001)
            maxNumber = minNumber + 1
        for (var i = 0; i < tickCount; ++i) {
            var ratio = tickCount === 1 ? 0 : i / (tickCount - 1)
            ticks.push(minNumber + (maxNumber - minNumber) * ratio)
        }
        return ticks
    }

    function applyExperimentHeightFallback() {
        var startMm = toNumber(experimentData.scan_range_start, 0)
        var endMm = toNumber(experimentData.scan_range_end, 55)
        var stepUm = toNumber(experimentData.scan_step, 20)
        var effectiveEndMm = endMm
        if (stepUm > 0 && endMm > startMm)
            effectiveEndMm = endMm - stepUm / 1000.0
        minHeightValue = Math.min(startMm, endMm)
        maxHeightValue = Math.max(startMm, effectiveEndMm)
        if (maxHeightValue <= minHeightValue)
            maxHeightValue = minHeightValue + 10
    }

    function pointsPerCurve() {
        return Math.max(280, Math.min(600, Math.round((realtimePage.width > 0 ? realtimePage.width : 1000) * 0.5)))
    }

    function pointX(point) {
        if (point === undefined || point === null)
            return Number.NaN
        if (point.x !== undefined)
            return toNumber(point.x, Number.NaN)
        if (point.length !== undefined && point.length > 0)
            return toNumber(point[0], Number.NaN)
        return Number.NaN
    }

    function updateCurveBoundsFromPoints(points, bounds) {
        if (!points || points.length === 0)
            return

        for (var i = 0; i < points.length; ++i) {
            var x = pointX(points[i])
            if (isNaN(x))
                continue
            bounds.minHeight = Math.min(bounds.minHeight, x)
            bounds.maxHeight = Math.max(bounds.maxHeight, x)
        }
    }

    function buildRealtimeCurveSignature(curves) {
        if (!curves || curves.length === 0)
            return ""

        var parts = [String(curves.length)]
        for (var i = 0; i < curves.length; ++i) {
            var curve = curves[i]
            parts.push(String(curve.scan_id) + ":" + String(curve.point_count) + ":" + String(curve.timestamp))
        }
        return parts.join("|")
    }

    function applyLightCurves(curves) {
        minLightValue = 0
        maxLightValue = 100
        applyExperimentHeightFallback()

        if (!curves || curves.length === 0) {
            lightCurves = []
            lightCurveCount = 0
            lightCurvesVersion += 1
            return
        }

        var sortedCurves = curves.slice(0)
        sortedCurves.sort(function(a, b) {
            var timeDiff = realtimePage.toNumber(a.timestamp, 0) - realtimePage.toNumber(b.timestamp, 0)
            if (timeDiff !== 0)
                return timeDiff
            return realtimePage.toNumber(a.scan_id, 0) - realtimePage.toNumber(b.scan_id, 0)
        })
        if (sortedCurves.length > maxVisibleCurveCount)
            sortedCurves = sortedCurves.slice(sortedCurves.length - maxVisibleCurveCount)

        var normalizedCurves = []
        var minHeight = Number.POSITIVE_INFINITY
        var maxHeight = Number.NEGATIVE_INFINITY
        var minLight = Number.POSITIVE_INFINITY
        var maxLight = Number.NEGATIVE_INFINITY

        for (var i = 0; i < sortedCurves.length; ++i) {
            var curve = sortedCurves[i]
            var rawMinHeight = toNumber(curve.min_height_mm, Number.NaN)
            var rawMaxHeight = toNumber(curve.max_height_mm, Number.NaN)
            var rawMinBackscatter = toNumber(curve.min_backscatter, Number.NaN)
            var rawMaxBackscatter = toNumber(curve.max_backscatter, Number.NaN)
            var rawMinTransmission = toNumber(curve.min_transmission, Number.NaN)
            var rawMaxTransmission = toNumber(curve.max_transmission, Number.NaN)

            if (!isNaN(rawMinHeight))
                minHeight = Math.min(minHeight, rawMinHeight)
            if (!isNaN(rawMaxHeight))
                maxHeight = Math.max(maxHeight, rawMaxHeight)
            if (!isNaN(rawMinBackscatter))
                minLight = Math.min(minLight, rawMinBackscatter)
            if (!isNaN(rawMaxBackscatter))
                maxLight = Math.max(maxLight, rawMaxBackscatter)
            if (!isNaN(rawMinTransmission))
                minLight = Math.min(minLight, rawMinTransmission)
            if (!isNaN(rawMaxTransmission))
                maxLight = Math.max(maxLight, rawMaxTransmission)

            if (isNaN(rawMinHeight) || isNaN(rawMaxHeight)) {
                var heightBounds = {
                    minHeight: minHeight,
                    maxHeight: maxHeight
                }
                updateCurveBoundsFromPoints(curve.backscatter_points || [], heightBounds)
                updateCurveBoundsFromPoints(curve.transmission_points || [], heightBounds)
                minHeight = heightBounds.minHeight
                maxHeight = heightBounds.maxHeight
            }

            normalizedCurves.push({
                                      scan_id: curve.scan_id,
                                      timestamp: curve.timestamp,
                                      scan_elapsed_ms: curve.scan_elapsed_ms,
                                      point_count: curve.point_count,
                                      min_height_mm: curve.min_height_mm,
                                      max_height_mm: curve.max_height_mm,
                                      min_backscatter: curve.min_backscatter,
                                      max_backscatter: curve.max_backscatter,
                                      min_transmission: curve.min_transmission,
                                      max_transmission: curve.max_transmission,
                                      backscatter_points: curve.backscatter_points || [],
                                      transmission_points: curve.transmission_points || [],
                                      color: curveColor(i, sortedCurves.length),
                                      legend_text: formatElapsedTime(curve.scan_elapsed_ms)
                                  })
        }

        lightCurves = normalizedCurves
        minHeightValue = isFinite(minHeight) ? minHeight : minHeightValue
        maxHeightValue = isFinite(maxHeight) ? maxHeight : maxHeightValue
        minLightValue = isFinite(minLight) ? minLight : 0
        maxLightValue = isFinite(maxLight) ? maxLight : 100
        lightCurveCount = normalizedCurves.length
        lightCurvesVersion += 1
    }

    function loadLightIntensityData() {
        lightCurves = []
        lightCurveCount = 0
        lightCurvesVersion += 1
        lastRealtimeCurveSignature = ""
        applyExperimentHeightFallback()

        if (experimentId <= 0 || !data_ctrl)
            return

        var curves = data_ctrl.getLightIntensityCurves(experimentId, pointsPerCurve())
        applyLightCurves(curves)
    }

    function loadLightIntensityScanData(scanId) {
        if (experimentId <= 0 || !data_ctrl || scanId < 0)
            return

        var curves = data_ctrl.getLightIntensityCurve(experimentId, scanId, pointsPerCurve())
        if (!curves || curves.length === 0)
            return

        var mergedCurves = []
        var replaced = false
        for (var i = 0; i < lightCurves.length; ++i) {
            if (Number(lightCurves[i].scan_id) === Number(scanId)) {
                mergedCurves.push(curves[0])
                replaced = true
            } else {
                mergedCurves.push(lightCurves[i])
            }
        }
        if (!replaced)
            mergedCurves.push(curves[0])

        applyLightCurves(mergedCurves)
    }

    function mergeRealtimeLightCurve(curve) {
        if (!curve || curve.scan_id === undefined)
            return

        var mergedCurves = []
        var replaced = false
        for (var i = 0; i < lightCurves.length; ++i) {
            if (Number(lightCurves[i].scan_id) === Number(curve.scan_id)) {
                mergedCurves.push(curve)
                replaced = true
            } else {
                mergedCurves.push(lightCurves[i])
            }
        }
        if (!replaced)
            mergedCurves.push(curve)

        var signature = buildRealtimeCurveSignature(mergedCurves)
        console.log("[RealtimeLight][qml merge]",
                    "experimentId=", experimentId,
                    "channelIndex=", channelIndex,
                    "scanId=", curve.scan_id,
                    "replaced=", replaced,
                    "beforeCount=", lightCurves.length,
                    "afterCount=", mergedCurves.length,
                    "pointCount=", curve.point_count)
        applyLightCurves(mergedCurves)
        lastRealtimeCurveSignature = signature
    }

    function loadRealtimeLightIntensityScanData(scanId) {
        if (experimentId <= 0 || !data_ctrl || scanId < 0)
            return false

        var curve = data_ctrl.getRealtimeLightIntensityCurve(experimentId, scanId)
        if (!curve || curve.scan_id === undefined) {
            console.log("[RealtimeLight][qml cache miss]",
                        "experimentId=", experimentId,
                        "channelIndex=", channelIndex,
                        "scanId=", scanId)
            return false
        }

        console.log("[RealtimeLight][qml cache hit]",
                    "experimentId=", experimentId,
                    "channelIndex=", channelIndex,
                    "scanId=", scanId,
                    "pointCount=", curve.point_count)
        mergeRealtimeLightCurve(curve)
        return true
    }

    function syncRealtimeExperimentContext(targetExperimentId) {
        if (targetExperimentId <= 0 || Number(targetExperimentId) === Number(realtimePage.experimentId))
            return

        console.log("[RealtimeLight][qml sync experiment]",
                    "oldExperimentId=", realtimePage.experimentId,
                    "newExperimentId=", targetExperimentId,
                    "channelIndex=", realtimePage.channelIndex)

        var latestExperiment = data_ctrl ? data_ctrl.getExperimentById(targetExperimentId) : null
        realtimePage.experimentData = latestExperiment && latestExperiment.id !== undefined
                ? latestExperiment
                : ({ "id": targetExperimentId })
        realtimePage.lightCurves = []
        realtimePage.lightCurveCount = 0
        realtimePage.lightCurvesVersion += 1
        realtimePage.lastRealtimeCurveSignature = ""
        realtimePage.applyExperimentHeightFallback()
    }

    function currentTabSource() {
        if (currentTabIndex === 1)
            return "curve/InstabilityCurvePage.qml"
        if (currentTabIndex === 2)
            return "curve/UniformityIndexPage.qml"
        return "curve/LightIntensityCurvePage.qml"
    }

    function activeRealtimeExperimentId() {
        var activeExperimentId = experimentId
        if (experiment_ctrl && experiment_ctrl.getCurrentExperimentId && channelIndex >= 0) {
            var currentExperimentId = Number(experiment_ctrl.getCurrentExperimentId(channelIndex))
            if (currentExperimentId > 0)
                activeExperimentId = currentExperimentId
        }
        return activeExperimentId
    }

    function pollRealtimeCurveUpdates() {
        var activeExperimentId = activeRealtimeExperimentId()
        if (activeExperimentId <= 0 || !data_ctrl)
            return

        var signature = data_ctrl.getRealtimeLightIntensityCurveSignature(activeExperimentId)
        if (!signature || signature === lastRealtimeCurveSignature)
            return

        realtimePage.syncRealtimeExperimentContext(activeExperimentId)
        var curves = data_ctrl.getRealtimeLightIntensityCurves(activeExperimentId)
        if (!curves || curves.length === 0)
            return

        console.log("[RealtimeLight][qml poll sync]",
                    "experimentId=", activeExperimentId,
                    "channelIndex=", channelIndex,
                    "curveCount=", curves.length,
                    "signature=", signature)
        applyLightCurves(curves)
        lastRealtimeCurveSignature = signature

        if (realtimeContentLoader.item && realtimeContentLoader.item.resetAnalysisDefaults)
            realtimeContentLoader.item.resetAnalysisDefaults()
        else if (realtimeContentLoader.item && realtimeContentLoader.item.loadDisplayedCurves)
            realtimeContentLoader.item.loadDisplayedCurves()
    }

    function refreshRealtimeData() {
        if (experimentId > 0 && data_ctrl && experimentData.scan_range_start === undefined) {
            var latestExperiment = data_ctrl.getExperimentById(experimentId)
            if (latestExperiment && latestExperiment.id !== undefined)
                experimentData = latestExperiment
        }
        loadLightIntensityData()
    }

    function refreshRealtimeScan(scanId, reloadAnalysis) {
        console.log("[RealtimeLight][qml refresh]",
                    "experimentId=", experimentId,
                    "channelIndex=", channelIndex,
                    "scanId=", scanId,
                    "reloadAnalysis=", reloadAnalysis,
                    "currentCurveCount=", lightCurves.length)
        if (experimentId > 0 && data_ctrl && experimentData.scan_range_start === undefined) {
            var latestExperiment = data_ctrl.getExperimentById(experimentId)
            if (latestExperiment && latestExperiment.id !== undefined)
                experimentData = latestExperiment
        }

        if (!loadRealtimeLightIntensityScanData(scanId))
            loadLightIntensityScanData(scanId)

        if (reloadAnalysis && realtimeContentLoader.item
                && realtimeContentLoader.item.loadDisplayedCurves)
            realtimeContentLoader.item.loadDisplayedCurves()
    }

    function releaseFinishedExperimentResources(stoppedChannel, stoppedExperimentId) {
        if (Number(stoppedChannel) !== Number(channelIndex))
            return

        var pageExperimentId = Number(experimentId)
        var activeExperimentId = Number(activeRealtimeExperimentId())
        var finishedExperimentId = Number(stoppedExperimentId)
        if (finishedExperimentId > 0
                && finishedExperimentId !== pageExperimentId
                && finishedExperimentId !== activeExperimentId) {
            return
        }

        console.log("[RealtimeLight][stop cleanup]",
                    "channelIndex=", channelIndex,
                    "pageExperimentId=", pageExperimentId,
                    "activeExperimentId=", activeExperimentId,
                    "stoppedExperimentId=", finishedExperimentId,
                    "curveCount=", lightCurves.length)

        lightCurves = []
        lightCurveCount = 0
        lightCurvesVersion += 1
        lastRealtimeCurveSignature = ""
        experimentData = ({})
        applyExperimentHeightFallback()
    }

    onExperimentDataChanged: refreshRealtimeData()

    Component.onCompleted: {
        refreshRealtimeData()
    }

    Timer {
        id: realtimeCurvePollTimer
        interval: 400
        repeat: true
        running: realtimePage.visible
        triggeredOnStart: true
        onTriggered: realtimePage.pollRealtimeCurveUpdates()
    }

    /*
    Connections {
        target: data_ctrl
        ignoreUnknownSignals: true
        function onRealtimeLightCurveReady(channel, experimentId, curve, scanCompleted) {
            console.log("[RealtimeLight][signal realtimeLightCurveReady]",
                        "pageExperimentId=", realtimePage.experimentId,
                        "signalExperimentId=", experimentId,
                        "pageChannelIndex=", realtimePage.channelIndex,
                        "signalChannel=", channel,
                        "scanId=", curve && curve.scan_id !== undefined ? curve.scan_id : -1,
                        "scanCompleted=", scanCompleted)
            if (channel !== realtimePage.channelIndex)
                return

            realtimePage.syncRealtimeExperimentContext(experimentId)

            realtimePage.mergeRealtimeLightCurve(curve)

            if (scanCompleted && currentTabIndex !== 0
                    && realtimeContentLoader.item
                    && realtimeContentLoader.item.loadDisplayedCurves)
                realtimeContentLoader.item.loadDisplayedCurves()
        }
        function onScanDataChanged(channel, experimentId, scanId, scanCompleted) {
            console.log("[RealtimeLight][signal scanDataChanged]",
                        "pageExperimentId=", realtimePage.experimentId,
                        "signalExperimentId=", experimentId,
                        "pageChannelIndex=", realtimePage.channelIndex,
                        "signalChannel=", channel,
                        "scanId=", scanId,
                        "scanCompleted=", scanCompleted)
            if (channel !== realtimePage.channelIndex)
                return

            realtimePage.syncRealtimeExperimentContext(experimentId)
            if (currentTabIndex === 0) {
                if (!realtimePage.loadRealtimeLightIntensityScanData(scanId))
                    realtimePage.loadLightIntensityScanData(scanId)
                return
            }

            if (scanCompleted && realtimeContentLoader.item
                    && realtimeContentLoader.item.loadDisplayedCurves)
                realtimeContentLoader.item.loadDisplayedCurves()
        }
        function onDataBatchAdded(count, experimentId) {
            // 实时页改为以 scan_id 为单位增量刷新，避免每批数据到达时整场实验全量重查。
        }
        }
    }

    */

    Connections {
        target: data_ctrl
        ignoreUnknownSignals: true
        function onRealtimeLightCurveReady(channel, experimentId, curve, scanCompleted) {
            console.log("[RealtimeLight][signal realtimeLightCurveReady]",
                        "pageExperimentId=", realtimePage.experimentId,
                        "signalExperimentId=", experimentId,
                        "pageChannelIndex=", realtimePage.channelIndex,
                        "signalChannel=", channel,
                        "scanId=", curve && curve.scan_id !== undefined ? curve.scan_id : -1,
                        "scanCompleted=", scanCompleted)
            if (channel !== realtimePage.channelIndex)
                return

            realtimePage.syncRealtimeExperimentContext(experimentId)
            realtimePage.mergeRealtimeLightCurve(curve)

            if (currentTabIndex === 0) {
                if (realtimeContentLoader.item && realtimeContentLoader.item.resetAnalysisDefaults)
                    realtimeContentLoader.item.resetAnalysisDefaults()
                else if (realtimeContentLoader.item && realtimeContentLoader.item.loadDisplayedCurves)
                    realtimeContentLoader.item.loadDisplayedCurves()
                return
            }

            if (scanCompleted && realtimeContentLoader.item
                    && realtimeContentLoader.item.loadDisplayedCurves)
                realtimeContentLoader.item.loadDisplayedCurves()
        }
        function onScanDataChanged(channel, experimentId, scanId, scanCompleted) {
            console.log("[RealtimeLight][signal scanDataChanged]",
                        "pageExperimentId=", realtimePage.experimentId,
                        "signalExperimentId=", experimentId,
                        "pageChannelIndex=", realtimePage.channelIndex,
                        "signalChannel=", channel,
                        "scanId=", scanId,
                        "scanCompleted=", scanCompleted)
            if (channel !== realtimePage.channelIndex)
                return

            realtimePage.syncRealtimeExperimentContext(experimentId)
            if (currentTabIndex === 0) {
                realtimePage.refreshRealtimeScan(scanId, true)
                return
            }

            if (scanCompleted && realtimeContentLoader.item
                    && realtimeContentLoader.item.loadDisplayedCurves)
                realtimeContentLoader.item.loadDisplayedCurves()
        }
        function onDataBatchAdded(count, experimentId) {
            // Realtime light refresh now happens per completed scan.
        }
    }

    Connections {
        target: experiment_ctrl
        ignoreUnknownSignals: true
        function onExperimentStopped(channel, experimentId) {
            realtimePage.releaseFinishedExperimentResources(channel, experimentId)
        }
    }

    Rectangle {
        anchors.fill: parent
        color: "#FFFFFF"

        ColumnLayout {
            anchors.fill: parent
            spacing: 0

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 42
                color: "transparent"

                Image {
                    anchors.fill: parent
                    source: "qrc:/icon/qml/icon/options_bar_bg_1.png"
                    fillMode: Image.TileVertically
                }

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 18
                    anchors.rightMargin: 18
                    spacing: 0

                    Text {
                        Layout.preferredWidth: 150
                        text: qsTr("实时实验 ") + (channelIndex >= 0 ? String.fromCharCode(65 + channelIndex) + qsTr("通道") : "")
                        font.pixelSize: 14
                        font.family: "Microsoft YaHei"
                        font.bold: true
                        color: "#2F3A4A"
                        verticalAlignment: Text.AlignVCenter
                    }

                    Repeater {
                        model: realtimePage.realtimeTabs

                        delegate: Item {
                            Layout.preferredWidth: index === 2 ? 112 : 96
                            Layout.fillHeight: true

                            Text {
                                anchors.centerIn: parent
                                text: modelData.title
                                font.pixelSize: 14
                                font.family: "Microsoft YaHei"
                                color: realtimePage.currentTabIndex === index ? "#2F7CF6" : "#2F3A4A"
                            }

                            Rectangle {
                                anchors.left: parent.left
                                anchors.right: parent.right
                                anchors.bottom: parent.bottom
                                anchors.leftMargin: 22
                                anchors.rightMargin: 22
                                height: 2
                                color: "#2F7CF6"
                                visible: realtimePage.currentTabIndex === index
                            }

                            MouseArea {
                                anchors.fill: parent
                                onClicked: realtimePage.openTab(index)
                            }
                        }
                    }

                    Item { Layout.fillWidth: true }

                    Text {
                        Layout.preferredWidth: 160
                        text: lightCurveCount > 0 ? qsTr("已加载 ") + lightCurveCount + qsTr(" 次扫描") : qsTr("等待实验数据")
                        font.pixelSize: 12
                        font.family: "Microsoft YaHei"
                        color: "#6E8096"
                        horizontalAlignment: Text.AlignRight
                        verticalAlignment: Text.AlignVCenter
                    }
                }
            }

            Loader {
                id: realtimeContentLoader
                Layout.fillWidth: true
                Layout.fillHeight: true
                source: realtimePage.currentTabSource()

                onLoaded: {
                    if (item) {
                        try {
                            item.detailPage = realtimePage
                            if (item.resetAnalysisDefaults) {
                                item.resetAnalysisDefaults()
                            } else if (item.loadDisplayedCurves) {
                                item.loadDisplayedCurves()
                            }
                        } catch (error) {
                        }
                    }
                }
            }
        }
    }
}
