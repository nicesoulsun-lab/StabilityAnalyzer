import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import "component"

Item {
    id: detailPage
    objectName: "ExperimentDetailPage"

    // 详情页现在只负责公共状态、通用换算和子页面装配，
    // 具体业务图表都拆到 qml/curve/ 下，避免主文件继续膨胀。
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
    property int lightCurveRequestId: 0
    property int lightCurveTotalCount: 0
    property bool lightCurvesTruncated: false

    signal backRequested()

    readonly property var detailTabs: [
        { title: qsTr("\u5149\u5f3a") },
        { title: qsTr("\u4e0d\u7a33\u5b9a\u6027") },
        { title: qsTr("\u5747\u5300\u5ea6") },
        { title: qsTr("\u5cf0\u539a\u5ea6") },
        { title: qsTr("\u5149\u5f3a\u5e73\u5747\u503c") },
        { title: qsTr("\u5206\u5c42\u539a\u5ea6") },
        { title: qsTr("\u9ad8\u7ea7\u8ba1\u7b97") }
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

    function pointsPerCurve() {
        return Math.max(280, Math.min(600, Math.round((detailPage.width > 0 ? detailPage.width : 1000) * 0.5)))
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

    function resetLightIntensityState(clearCurves) {
        if (clearCurves === undefined || clearCurves)
            lightCurves = []
        applyExperimentHeightFallback()
        minLightValue = 0
        maxLightValue = 100
        lightCurveCount = 0
        lightCurveTotalCount = 0
        lightCurvesTruncated = false
        lightCurvesLoading = false
        lightCurvesVersion += 1
    }

    function releaseLightIntensityData(reason) {
        if (detail_ctrl && lightCurveRequestId > 0)
            detail_ctrl.cancelLightCurveRequest(lightCurveRequestId)
        lightCurveRequestId = 0
        if (detail_ctrl)
            detail_ctrl.clearExperimentCache(experimentData && experimentData.id !== undefined ? Number(experimentData.id) : 0)
        resetLightIntensityState(true)
        console.log("[DetailLight][release]",
                    "experimentId=", experimentData && experimentData.id !== undefined ? Number(experimentData.id) : 0,
                    "reason=", reason)
    }

    function applyLightIntensityPayload(payload) {
        var curves = payload && payload.curves ? payload.curves : []
        var sortedCurves = curves.slice(0)
        sortedCurves.sort(function(a, b) {
            var timeDiff = detailPage.toNumber(a.timestamp, 0) - detailPage.toNumber(b.timestamp, 0)
            if (timeDiff !== 0)
                return timeDiff
            return detailPage.toNumber(a.scan_id, 0) - detailPage.toNumber(b.scan_id, 0)
        })

        var normalizedCurves = []
        for (var i = 0; i < sortedCurves.length; ++i) {
            var curve = sortedCurves[i]
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
        minHeightValue = toNumber(payload ? payload.minHeightValue : 0, minHeightValue)
        maxHeightValue = toNumber(payload ? payload.maxHeightValue : 10, maxHeightValue)
        minLightValue = toNumber(payload ? payload.minLightValue : 0, 0)
        maxLightValue = toNumber(payload ? payload.maxLightValue : 100, 100)
        lightCurveCount = normalizedCurves.length
        lightCurveTotalCount = toNumber(payload ? payload.totalCurveCount : normalizedCurves.length, normalizedCurves.length)
        lightCurvesTruncated = !!(payload && payload.truncated)
        lightCurvesLoading = false
        lightCurvesVersion += 1

        console.log("[DetailLight][ready]",
                    "experimentId=", experimentData && experimentData.id !== undefined ? Number(experimentData.id) : 0,
                    "loadedCurveCount=", lightCurveCount,
                    "totalCurveCount=", lightCurveTotalCount,
                    "truncated=", lightCurvesTruncated)
    }

    function loadLightIntensityData() {
        if (currentTabIndex !== 0) {
            releaseLightIntensityData("tabInactive")
            return
        }

        resetLightIntensityState(true)
        if (!experimentData || experimentData.id === undefined || !detail_ctrl)
            return

        lightCurvesLoading = true
        lightCurveRequestId = detail_ctrl.requestLightCurves(Number(experimentData.id), pointsPerCurve())
        console.log("[DetailLight][request]",
                    "experimentId=", Number(experimentData.id),
                    "requestId=", lightCurveRequestId)
        if (lightCurveRequestId <= 0)
            lightCurvesLoading = false
    }

    function currentTabSource() {
        // 统一在这里维护页签和子页面的映射，子页面创建时通过 Loader 注入 detailPage。
        if (currentTabIndex === 1)
            return "curve/InstabilityCurvePage.qml"
        if (currentTabIndex === 2)
            return "curve/UniformityIndexPage.qml"
        if (currentTabIndex === 3)
            return "curve/PeakThicknessPage.qml"
        if (currentTabIndex === 4)
            return "curve/LightIntensityAveragePage.qml"
        if (currentTabIndex === 5)
            return "curve/SeparationLayerPage.qml"
        if (currentTabIndex === 6)
            return "curve/AdvancedCalculationPage.qml"
        if (currentTabIndex === 0)
            return "curve/LightIntensityCurvePage.qml"
        return "curve/PlaceholderPage.qml"
    }

    function initializeDetailTabItem(tabItem) {
        if (!tabItem)
            return

        try {
            tabItem.detailPage = detailPage
        } catch (error) {
            console.log("[DetailPage][tab init] assign detailPage failed",
                        "tabIndex=", currentTabIndex,
                        "source=", currentTabSource(),
                        "error=", error)
            return
        }

        console.log("[DetailPage][tab init]",
                    "tabIndex=", currentTabIndex,
                    "source=", currentTabSource())

        try {
            if (tabItem.resetAnalysisDefaults) {
                tabItem.resetAnalysisDefaults()
            } else if (tabItem.loadDisplayedCurves) {
                tabItem.loadDisplayedCurves()
            }

            if (tabItem.loadInstabilityData) {
                tabItem.loadInstabilityData()
                if (tabItem.ensureModeData)
                    tabItem.ensureModeData()
            }

            if (tabItem.loadUniformityData)
                tabItem.loadUniformityData()

            if (tabItem.applyPeakThicknessParameters) {
                if (tabItem.heightLowerBound !== undefined)
                    tabItem.heightLowerBound = detailPage.floorToStep(detailPage.minHeightValue, 1)
                if (tabItem.heightUpperBound !== undefined)
                    tabItem.heightUpperBound = detailPage.ceilToStep(detailPage.maxHeightValue, 1)
                tabItem.applyPeakThicknessParameters()
            }

            if (tabItem.loadAverageData)
                tabItem.loadAverageData()

            if (tabItem.loadSeparationData)
                tabItem.loadSeparationData()

            if (tabItem.refreshFluidInputs)
                tabItem.refreshFluidInputs()
            if (tabItem.refreshOpticalInputs)
                tabItem.refreshOpticalInputs()
        } catch (error) {
            console.log("[DetailPage][tab init] run failed",
                        "tabIndex=", currentTabIndex,
                        "source=", currentTabSource(),
                        "error=", error)
        }
    }

    onExperimentDataChanged: loadLightIntensityData()
    onCurrentTabIndexChanged: loadLightIntensityData()
    Component.onCompleted: loadLightIntensityData()
    Component.onDestruction: releaseLightIntensityData("destroyed")

    Connections {
        target: detail_ctrl
        ignoreUnknownSignals: true
        onLightCurveRequestFinished: {
            console.log("[DetailLight][finished signal]",
                        "requestId=", requestId,
                        "activeRequestId=", detailPage.lightCurveRequestId,
                        "experimentId=", experimentId,
                        "activeExperimentId=", detailPage.experimentData && detailPage.experimentData.id !== undefined ? Number(detailPage.experimentData.id) : 0)
            if (requestId !== detailPage.lightCurveRequestId)
                return
            if (!detailPage.experimentData || Number(detailPage.experimentData.id) !== Number(experimentId))
                return

            detailPage.lightCurveRequestId = 0
            detailPage.applyLightIntensityPayload(payload)
        }
        onLightCurveRequestFailed: {
            if (requestId !== detailPage.lightCurveRequestId)
                return
            if (!detailPage.experimentData || Number(detailPage.experimentData.id) !== Number(experimentId))
                return

            detailPage.lightCurveRequestId = 0
            detailPage.lightCurvesLoading = false
            console.log("[DetailLight][failed]",
                        "experimentId=", experimentId,
                        "message=", message)
        }
        onLightCurveRequestCancelled: {
            if (requestId !== detailPage.lightCurveRequestId)
                return

            detailPage.lightCurveRequestId = 0
            detailPage.lightCurvesLoading = false
            console.log("[DetailLight][cancelled]",
                        "experimentId=", experimentId,
                        "reason=", reason)
        }
    }

    Rectangle {
        anchors.fill: parent
        color: "transparent"

        ColumnLayout {
            anchors.fill: parent
            spacing: 0

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 42
                color: "transparent"
                border.color: "#E6EEF7"
                border.width: 0

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

                    Repeater {
                        model: detailPage.detailTabs

                        delegate: Item {
                            Layout.preferredWidth: 100
                            Layout.fillHeight: true

                            Text {
                                anchors.centerIn: parent
                                text: modelData.title
                                font.pixelSize: 14
                                font.family: "Microsoft YaHei"
                                color: detailPage.currentTabIndex === index ? "#2F7CF6" : "#2F3A4A"
                            }

                            Rectangle {
                                anchors.left: parent.left
                                anchors.right: parent.right
                                anchors.bottom: parent.bottom
                                anchors.leftMargin: 24
                                anchors.rightMargin: 24
                                height: 2
                                color: "#2F7CF6"
                                visible: detailPage.currentTabIndex === index
                            }

                            MouseArea {
                                anchors.fill: parent
                                onClicked: detailPage.openTab(index)
                            }
                        }
                    }

                    Item { Layout.fillWidth: true }

                    IconButton {
                        id: backButton
                        Layout.preferredWidth: 100
                        Layout.preferredHeight: 28
                        Layout.rightMargin: 20
                        Layout.alignment: Qt.AlignVCenter
                        text: qsTr("返回记录列表")
                        onClicked: detailPage.backRequested()

                        contentItem: Text {
                            text: backButton.text
                            color: "#2F7CF6"
                            font.pixelSize: 13
                            font.family: "Microsoft YaHei"
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }

                        background: Rectangle {
                            radius: 4
                            color: "#EEF5FF"
                            border.color: "#C9DBF8"
                            border.width: 1
                        }
                    }
                }
            }

            Loader {
                id: detailContentLoader
                Layout.fillWidth: true
                Layout.fillHeight: true
                source: detailPage.currentTabSource()

                onLoaded: {
                    detailPage.initializeDetailTabItem(item)
                }
            }
        }
    }
}
