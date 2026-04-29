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
    property bool lightCurvesLoading: false
    property int maxVisibleCurveCount: 20
    property bool realtimeSessionActive: false

    signal backRequested()

    readonly property int experimentId: experimentData && experimentData.id !== undefined ? Number(experimentData.id) : 0
    readonly property bool isRealtimeChartTab: currentTabIndex >= 0 && currentTabIndex <= 2
    readonly property var realtimeTabs: [
        { title: qsTr("\u5149\u5f3a") },
        { title: qsTr("\u4e0d\u7a33\u5b9a\u6027") },
        { title: qsTr("\u5747\u5300\u5ea6\u6307\u6570") }
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

    function resetRealtimeCurves(reason) {
        lightCurves = []
        lightCurveCount = 0
        lightCurvesVersion += 1
        applyExperimentHeightFallback()
        console.log("[RealtimeLight][reset]",
                    "channelIndex=", channelIndex,
                    "experimentId=", experimentId,
                    "reason=", reason)
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
        if (sortedCurves.length > maxVisibleCurveCount) {
            console.log("[RealtimeLight][curve trim]",
                        "channelIndex=", channelIndex,
                        "experimentId=", experimentId,
                        "inputCurveCount=", sortedCurves.length,
                        "maxVisibleCurveCount=", maxVisibleCurveCount)
            sortedCurves = sortedCurves.slice(sortedCurves.length - maxVisibleCurveCount)
        }

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
        var targetExperimentId = activeRealtimeExperimentId()
        if (targetExperimentId <= 0 || !realtime_ctrl) {
            resetRealtimeCurves("invalidExperiment")
            return
        }

        var curves = realtime_ctrl.getLightCurves(targetExperimentId)
        console.log("[RealtimeLight][load curves]",
                    "channelIndex=", channelIndex,
                    "experimentId=", targetExperimentId,
                    "curveCount=", curves ? curves.length : 0)
        applyLightCurves(curves)
    }

    function updateRealtimeSubscription(reason) {
        var targetExperimentId = activeRealtimeExperimentId()
        var shouldActivate = visible && isRealtimeChartTab && channelIndex >= 0 && targetExperimentId > 0
        if (!realtime_ctrl) {
            realtimeSessionActive = false
            return
        }

        realtime_ctrl.setActiveSession(channelIndex, targetExperimentId, currentTabIndex, shouldActivate)
        realtimeSessionActive = shouldActivate
        if (shouldActivate) {
            console.log("[RealtimeLight][subscribe active]",
                        "channelIndex=", channelIndex,
                        "experimentId=", targetExperimentId,
                        "tabIndex=", currentTabIndex,
                        "reason=", reason)
            if (currentTabIndex === 0) {
                loadLightIntensityData()
            } else {
                lightCurvesVersion += 1
                console.log("[RealtimeLight][restore cached state]",
                            "channelIndex=", channelIndex,
                            "experimentId=", targetExperimentId,
                            "tabIndex=", currentTabIndex,
                            "reason=", reason)
            }
        } else {
            resetRealtimeCurves(reason)
        }
    }

    function currentTabSource() {
        if (currentTabIndex === 1)
            return "curve/RealtimeInstabilityCurvePage.qml"
        if (currentTabIndex === 2)
            return "curve/RealtimeUniformityIndexPage.qml"
        return "curve/RealtimeLightIntensityPage.qml"
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

        resetRealtimeCurves("stopped")
        realtimeSessionActive = false
    }

    onExperimentDataChanged: {
        applyExperimentHeightFallback()
        updateRealtimeSubscription("experimentDataChanged")
    }

    onVisibleChanged: updateRealtimeSubscription("visibleChanged")
    onCurrentTabIndexChanged: updateRealtimeSubscription("tabChanged")
    onChannelIndexChanged: updateRealtimeSubscription("channelChanged")

    Component.onCompleted: {
        applyExperimentHeightFallback()
        updateRealtimeSubscription("completed")
    }

    Component.onDestruction: {
        if (realtime_ctrl)
            realtime_ctrl.setActiveSession(channelIndex, activeRealtimeExperimentId(), currentTabIndex, false)
    }


    Connections {
        target: realtime_ctrl
        ignoreUnknownSignals: true
        onLightCurvesChanged: {
            if (channel !== realtimePage.channelIndex)
                return
            if (!realtimePage.visible || experimentId <= 0 || !realtimePage.realtimeSessionActive)
                return
            if (Number(experimentId) !== Number(realtimePage.activeRealtimeExperimentId()))
                return

            console.log("[RealtimeLight][signal curves changed]",
                        "channelIndex=", realtimePage.channelIndex,
                        "experimentId=", experimentId,
                        "curveCount=", curveCount)
            if (realtimePage.currentTabIndex === 0) {
                realtimePage.loadLightIntensityData()
            } else {
                realtimePage.lightCurvesVersion += 1
            }
        }
        onExperimentSessionCleared: {
            if (channel !== realtimePage.channelIndex)
                return

            realtimePage.releaseFinishedExperimentResources(channel, experimentId)
        }
    }

    Connections {
        target: experiment_ctrl
        ignoreUnknownSignals: true
        onExperimentStopped: {
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
                        text: lightCurveCount > 0 ? qsTr("\u5df2\u52a0\u8f7d ") + lightCurveCount + qsTr(" \u6b21\u626b\u63cf") : qsTr("\u7b49\u5f85\u5b9e\u9a8c\u6570\u636e")
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
                        console.log("[RealtimePage][tab loaded]",
                                    "channelIndex=", realtimePage.channelIndex,
                                    "experimentId=", realtimePage.activeRealtimeExperimentId(),
                                    "tabIndex=", realtimePage.currentTabIndex,
                                    "source=", realtimeContentLoader.source)
                        try {
                            item.detailPage = realtimePage
                            if (item.loadRealtimeData) {
                                item.loadRealtimeData()
                            }
                        } catch (error) {
                            console.log("[RealtimePage][tab load error]",
                                        "tabIndex=", realtimePage.currentTabIndex,
                                        "source=", realtimeContentLoader.source,
                                        "message=", error)
                        }
                    }
                }
            }
        }
    }
}
