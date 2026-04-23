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

    signal backRequested()

    readonly property var detailTabs: [
        { title: qsTr("光强") },
        { title: qsTr("不稳定性") },
        { title: qsTr("均匀度") },
        { title: qsTr("峰厚度") },
        { title: qsTr("光强平均值") },
        { title: qsTr("分层厚度") },
        { title: qsTr("高级计算") }
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

    function loadLightIntensityData() {
        // 光强页仍然是特殊页：它需要一次性准备整组曲线和右侧时间色带，
        // 所以保留在详情页层面统一预加载，其他页签按各自子页懒加载。
        lightCurves = []
        minHeightValue = 0
        maxHeightValue = 10
        minLightValue = 0
        maxLightValue = 100
        lightCurveCount = 0

        if (!experimentData || experimentData.id === undefined || !data_ctrl)
            return

        var pointsPerCurve = Math.max(480, Math.round((detailPage.width > 0 ? detailPage.width : 1000) * 1.1))
        var curves = data_ctrl.getLightIntensityCurves(Number(experimentData.id), pointsPerCurve)
        if (!curves || curves.length === 0)
            return

        var sortedCurves = curves.slice(0)
        sortedCurves.sort(function(a, b) {
            var timeDiff = detailPage.toNumber(a.timestamp, 0) - detailPage.toNumber(b.timestamp, 0)
            if (timeDiff !== 0)
                return timeDiff
            return detailPage.toNumber(a.scan_id, 0) - detailPage.toNumber(b.scan_id, 0)
        })

        var normalizedCurves = []
        var minHeight = Number.POSITIVE_INFINITY
        var maxHeight = Number.NEGATIVE_INFINITY
        var minLight = Number.POSITIVE_INFINITY
        var maxLight = Number.NEGATIVE_INFINITY

        for (var i = 0; i < sortedCurves.length; ++i) {
            var curve = sortedCurves[i]
            var backscatterPoints = curve.backscatter_points || []
            var transmissionPoints = curve.transmission_points || []
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

            normalizedCurves.push({
                scan_id: curve.scan_id,
                timestamp: curve.timestamp,
                scan_elapsed_ms: curve.scan_elapsed_ms,
                point_count: curve.point_count,
                backscatter_points: backscatterPoints,
                transmission_points: transmissionPoints,
                color: curveColor(i, sortedCurves.length),
                legend_text: formatElapsedTime(curve.scan_elapsed_ms)
            })
        }

        lightCurves = normalizedCurves
        minHeightValue = isFinite(minHeight) ? minHeight : 0
        maxHeightValue = isFinite(maxHeight) ? maxHeight : 10
        minLightValue = isFinite(minLight) ? minLight : 0
        maxLightValue = isFinite(maxLight) ? maxLight : 100
        lightCurveCount = normalizedCurves.length
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

    function reloadCurrentTab() {
        // 使用 setSource 注入 detailPage，保证子页在 Component.onCompleted 之前
        // 就能拿到公共状态，避免首次进入页签时需要切换一次才触发计算。
        detailContentLoader.setSource(currentTabSource(), { "detailPage": detailPage })
    }

    onExperimentDataChanged: {
        loadLightIntensityData()
        reloadCurrentTab()
    }
    onCurrentTabIndexChanged: reloadCurrentTab()
    Component.onCompleted: {
        loadLightIntensityData()
        reloadCurrentTab()
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
            }
        }
    }
}
