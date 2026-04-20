import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import CustomComponents 1.0

Item {
    id: detailPage
    objectName: "ExperimentDetailPage"

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

    function paddedMin(value, maxValue, minPadding) {
        var minNumber = toNumber(value, 0)
        var maxNumber = toNumber(maxValue, minNumber)
        var range = maxNumber - minNumber
        var padding = Math.max(Math.abs(range) * 0.08, minPadding === undefined ? 1 : minPadding)
        if (Math.abs(range) < 0.000001)
            padding = Math.max(Math.abs(maxNumber) * 0.1, minPadding === undefined ? 1 : minPadding)
        return minNumber - padding
    }

    function paddedMax(value, minValue, minPadding) {
        var maxNumber = toNumber(value, 0)
        var minNumber = toNumber(minValue, maxNumber)
        var range = maxNumber - minNumber
        var padding = Math.max(Math.abs(range) * 0.08, minPadding === undefined ? 1 : minPadding)
        if (Math.abs(range) < 0.000001)
            padding = Math.max(Math.abs(maxNumber) * 0.1, minPadding === undefined ? 1 : minPadding)
        return maxNumber + padding
    }

    function xAxisTicks() {
        var ticks = []
        var start = floorToStep(minHeightValue, 10)
        var end = ceilToStep(maxHeightValue, 10)
        if (end <= start)
            end = start + 10
        for (var value = start; value <= end + 0.0001; value += 10)
            ticks.push(value)
        return ticks
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

    function currentTabComponent() {
        if (currentTabIndex === 1)
            return instabilityCurveComponent
        if (currentTabIndex === 2)
            return uniformityIndexComponent
        if (currentTabIndex === 4)
            return lightIntensityAvgComponent
        if (currentTabIndex === 0)
            return lightIntensitySwitchComponent
        return placeholderComponent
    }

    onExperimentDataChanged: loadLightIntensityData()
    Component.onCompleted: loadLightIntensityData()

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

                    Button {
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
                sourceComponent: detailPage.currentTabComponent()
            }
        }
    }

    Component {
        id: placeholderComponent

        Rectangle {
            color: "#FFFFFF"

            Rectangle {
                anchors.fill: parent
                anchors.margins: 18
                color: "#F8FBFF"
                border.color: "#D8E4F0"
                border.width: 1
                radius: 6

                Text {
                    anchors.centerIn: parent
                    text: qsTr("该页面暂未实现")
                    font.pixelSize: 16
                    font.family: "Microsoft YaHei"
                    color: "#7A8CA5"
                }
            }
        }
    }

    Component {
        id: lightIntensitySwitchComponent

        Rectangle {
            id: lightIntensityPanel
            color: "#FFFFFF"
            property int currentLightModeIndex: 0
            property var lightModeTitles: [qsTr("背射光"), qsTr("透射光"), qsTr("背射光+透射光")]
            property real chartMinX: detailPage.floorToStep(detailPage.minHeightValue, 10)
            property real chartMaxX: Math.max(detailPage.ceilToStep(detailPage.maxHeightValue, 10), chartMinX + 10)
            property var xAxisTickValues: detailPage.xAxisTicks()
            property var yAxisLabels: ["0", "20", "40", "60", "80", "100"]

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
                            visible: lightCurveCount === 0
                            text: qsTr("数据库中暂无该实验的光强曲线数据")
                            font.pixelSize: 15
                            font.family: "Microsoft YaHei"
                            color: "#7A8CA5"
                        }

                        RowLayout {
                            anchors.fill: parent
                            spacing: 14
                            visible: lightCurveCount > 0

                            Item {
                                Layout.fillWidth: true
                                Layout.fillHeight: true

                                ColumnLayout {
                                    anchors.fill: parent
                                    spacing: 12
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
                                                model: lightIntensityPanel.yAxisLabels.length
                                                Rectangle {
                                                    width: parent.width
                                                    height: 1
                                                    color: "#EEF3F8"
                                                    y: index * (parent.height / (lightIntensityPanel.yAxisLabels.length - 1))
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
                                                model: lightCurves
                                                CurveItem {
                                                    anchors.fill: parent
                                                    lineColor: modelData.color
                                                    lineWidth: 2
                                                    autoScale: false
                                                    minXValue: lightIntensityPanel.chartMinX
                                                    maxXValue: lightIntensityPanel.chartMaxX
                                                    minYValue: 0
                                                    maxYValue: 100
                                                    dataPoints: modelData.transmission_points
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
                                                model: lightIntensityPanel.yAxisLabels.length
                                                Rectangle {
                                                    width: parent.width
                                                    height: 1
                                                    color: "#EEF3F8"
                                                    y: index * (parent.height / (lightIntensityPanel.yAxisLabels.length - 1))
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
                                                model: lightCurves
                                                CurveItem {
                                                    anchors.fill: parent
                                                    lineColor: modelData.color
                                                    lineWidth: 2
                                                    autoScale: false
                                                    minXValue: lightIntensityPanel.chartMinX
                                                    maxXValue: lightIntensityPanel.chartMaxX
                                                    minYValue: 0
                                                    maxYValue: 100
                                                    dataPoints: modelData.backscatter_points
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
                                            model: lightIntensityPanel.yAxisLabels.length
                                            Rectangle {
                                                width: parent.width
                                                height: 1
                                                color: "#EEF3F8"
                                                y: index * (parent.height / (lightIntensityPanel.yAxisLabels.length - 1))
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
                                            model: lightCurves
                                            CurveItem {
                                                anchors.fill: parent
                                                lineColor: modelData.color
                                                lineWidth: 2
                                                autoScale: false
                                                minXValue: lightIntensityPanel.chartMinX
                                                maxXValue: lightIntensityPanel.chartMaxX
                                                minYValue: 0
                                                maxYValue: 100
                                                dataPoints: lightIntensityPanel.currentLightModeIndex === 0 ? modelData.backscatter_points : modelData.transmission_points
                                            }
                                        }

                                        Text {
                                            anchors.left: parent.left
                                            anchors.leftMargin: -50
                                            anchors.verticalCenter: parent.verticalCenter
                                            text: lightIntensityPanel.currentLightModeIndex === 0 ? "BS\n(%)" : "T\n(%)"
                                            font.pixelSize: 12
                                            font.family: "Microsoft YaHei"
                                            color: "#6E8096"
                                            font.bold: true
                                            horizontalAlignment: Text.AlignHCenter
                                            verticalAlignment: Text.AlignVCenter
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

                                        Item {
                                            anchors.left: parent.left
                                            anchors.top: parent.top
                                            anchors.bottom: parent.bottom
                                            width: parent.width - 26

                                            Repeater {
                                                model: Math.min(14, lightCurves.length)

                                                delegate: Item {
                                                    width: parent.width
                                                    height: 20
                                                    property int markerCount: Math.min(14, lightCurves.length)
                                                    property int curveIndex: lightCurves.length <= 1 ? 0 : Math.round(index * (lightCurves.length - 1) / Math.max(markerCount - 1, 1))
                                                    y: markerCount <= 1 ? 0 : index * (parent.height - height) / (markerCount - 1)

                                                    Text {
                                                        anchors.right: parent.right
                                                        anchors.verticalCenter: parent.verticalCenter
                                                        text: lightCurves.length > 0 ? lightCurves[curveIndex].legend_text : "--"
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
                                                target: detailPage
                                                function onLightCurvesChanged() { heatLegendCanvas.requestPaint() }
                                            }

                                            onWidthChanged: requestPaint()
                                            onHeightChanged: requestPaint()

                                            onPaint: {
                                                var ctx = getContext("2d")
                                                ctx.clearRect(0, 0, width, height)
                                                if (!lightCurves || lightCurves.length === 0)
                                                    return
                                                var gradient = ctx.createLinearGradient(0, 0, 0, height)
                                                var stopCount = Math.max(lightCurves.length - 1, 1)
                                                for (var i = 0; i < lightCurves.length; ++i) {
                                                    gradient.addColorStop(stopCount === 0 ? 0 : i / stopCount, lightCurves[i].color)
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
        }
    }

    Component {
        id: instabilityCurveComponent

        Rectangle {
            id: instabilityPanel
            color: "#FFFFFF"
            property var rows: []
            property var points: []
            property real chartMinX: 0
            property real chartMaxX: 1
            property real chartMinY: 0
            property real chartMaxY: 1
            property var xAxisTickValues: [0, 1]
            property var yAxisLabels: detailPage.makeAxisLabels(chartMinY, chartMaxY, 6)

            function loadInstabilityData() {
                rows = []
                points = []
                chartMinX = 0
                chartMaxX = 1
                chartMinY = 0
                chartMaxY = 1
                xAxisTickValues = [0, 1]
                yAxisLabels = detailPage.makeAxisLabels(chartMinY, chartMaxY, 6)

                if (!experimentData || experimentData.id === undefined || !data_ctrl)
                    return

                var source = data_ctrl.getInstabilityCurveData(Number(experimentData.id))
                if (!source || source.length === 0)
                    return

                var sorted = source.slice(0)
                sorted.sort(function(a, b) {
                    var elapsedDiff = detailPage.toNumber(a.scan_elapsed_ms, 0) - detailPage.toNumber(b.scan_elapsed_ms, 0)
                    if (elapsedDiff !== 0)
                        return elapsedDiff
                    return detailPage.toNumber(a.scan_id, 0) - detailPage.toNumber(b.scan_id, 0)
                })

                var plotted = []
                var maxY = 0
                for (var i = 0; i < sorted.length; ++i) {
                    var xValue = detailPage.toNumber(sorted[i].scan_elapsed_ms, 0) / 60000.0
                    var yValue = detailPage.toNumber(sorted[i].instability_value, 0)
                    plotted.push({ x: xValue, y: yValue })
                    maxY = Math.max(maxY, yValue)
                }

                rows = sorted
                points = plotted
                chartMinX = plotted.length > 0 ? plotted[0].x : 0
                chartMaxX = plotted.length > 1 ? plotted[plotted.length - 1].x : chartMinX + 1
                if (chartMaxX <= chartMinX)
                    chartMaxX = chartMinX + 1
                chartMinY = 0
                chartMaxY = Math.max(1, maxY * 1.12)
                xAxisTickValues = detailPage.buildTimeTicks(chartMinX, chartMaxX, 6)
                yAxisLabels = detailPage.makeAxisLabels(chartMinY, chartMaxY, 6, 1)
            }

            Component.onCompleted: loadInstabilityData()

            Rectangle {
                anchors.fill: parent
                anchors.margins: 18
                radius: 6
                color: "#F8FBFF"
                border.color: "#D8E4F0"
                border.width: 1

                Item {
                    anchors.fill: parent
                    anchors.margins: 12

                    Text {
                        anchors.centerIn: parent
                        visible: instabilityPanel.points.length === 0
                        text: qsTr("数据库中暂无该实验的不稳定性曲线数据")
                        font.pixelSize: 15
                        font.family: "Microsoft YaHei"
                        color: "#7A8CA5"
                    }

                    Item {
                        anchors.fill: parent
                        visible: instabilityPanel.points.length > 0

                        Rectangle {
                            anchors.fill: parent
                            radius: 6
                            color: "#FFFFFF"
                            border.color: "#DCE6F2"
                            border.width: 1
                        }

                        Item {
                            anchors.fill: parent
                            anchors.leftMargin: 72
                            anchors.rightMargin: 22
                            anchors.topMargin: 18
                            anchors.bottomMargin: 45

                            Repeater {
                                model: instabilityPanel.yAxisLabels.length
                                Rectangle {
                                    width: parent.width
                                    height: 1
                                    color: "#EEF3F8"
                                    y: index * (parent.height / (instabilityPanel.yAxisLabels.length - 1))
                                }
                            }

                            Repeater {
                                model: instabilityPanel.xAxisTickValues
                                Rectangle {
                                    width: 1
                                    height: parent.height
                                    color: "#EEF3F8"
                                    x: (modelData - instabilityPanel.chartMinX) / Math.max(instabilityPanel.chartMaxX - instabilityPanel.chartMinX, 1) * parent.width
                                }
                            }

                            CurveItem {
                                anchors.fill: parent
                                lineColor: "#2F7CF6"
                                lineWidth: 2
                                autoScale: false
                                minXValue: instabilityPanel.chartMinX
                                maxXValue: instabilityPanel.chartMaxX
                                minYValue: instabilityPanel.chartMinY
                                maxYValue: instabilityPanel.chartMaxY
                                dataPoints: instabilityPanel.points
                            }

                            Text {
                                anchors.left: parent.left
                                anchors.leftMargin: -64
                                anchors.verticalCenter: parent.verticalCenter
                                text: "Ius"
                                font.pixelSize: 12
                                font.family: "Microsoft YaHei"
                                color: "#6E8096"
                                font.bold: true
                            }

                            Repeater {
                                model: instabilityPanel.yAxisLabels
                                delegate: Text {
                                    anchors.right: parent.left
                                    anchors.rightMargin: 10
                                    y: index * (parent.height / Math.max(instabilityPanel.yAxisLabels.length - 1, 1)) - height / 2
                                    text: instabilityPanel.yAxisLabels[instabilityPanel.yAxisLabels.length - 1 - index]
                                    font.pixelSize: 11
                                    font.family: "Microsoft YaHei"
                                    color: "#7A8CA5"
                                }
                            }

                            Repeater {
                                model: instabilityPanel.xAxisTickValues
                                delegate: Text {
                                    y: parent.height + 6
                                    x: (modelData - instabilityPanel.chartMinX) / Math.max(instabilityPanel.chartMaxX - instabilityPanel.chartMinX, 1) * parent.width - width / 2
                                    text: detailPage.formatNumber(modelData, 1)
                                    font.pixelSize: 11
                                    font.family: "Microsoft YaHei"
                                    color: "#7A8CA5"
                                }
                            }

                            Text {
                                anchors.horizontalCenter: parent.horizontalCenter
                                anchors.bottom: parent.bottom
                                anchors.bottomMargin: -38
                                text: qsTr("时间(min)")
                                font.pixelSize: 12
                                font.family: "Microsoft YaHei"
                                color: "#6E8096"
                                font.bold: true
                            }
                        }
                    }
                }
            }
        }
    }

    Component {
        id: uniformityIndexComponent

        Rectangle {
            id: uniformityPanel
            color: "#FFFFFF"
            property int currentModeIndex: 0
            property var modeTitles: [qsTr("背射光"), qsTr("透射光"), qsTr("背射光+透射光")]
            property var rows: []
            property var bsPoints: []
            property var tPoints: []
            property real chartMinX: 0
            property real chartMaxX: 1
            property real chartMinY: 0
            property real chartMaxY: 1
            property var xAxisTickValues: [0, 1]
            property var yAxisLabels: detailPage.makeAxisLabels(0, 1, 6, 2)

            function loadUniformityData() {
                rows = []
                bsPoints = []
                tPoints = []
                if (!experimentData || experimentData.id === undefined || !data_ctrl)
                    return

                var source = data_ctrl.getUniformityIndices(Number(experimentData.id))
                if (!source || source.length === 0)
                    return

                var sorted = source.slice(0)
                sorted.sort(function(a, b) {
                    return detailPage.toNumber(a.scan_elapsed_ms, 0) - detailPage.toNumber(b.scan_elapsed_ms, 0)
                })

                var localBs = []
                var localT = []
                for (var i = 0; i < sorted.length; ++i) {
                    var xValue = detailPage.toNumber(sorted[i].scan_elapsed_ms, 0) / 60000.0
                    localBs.push({ x: xValue, y: detailPage.toNumber(sorted[i].ui_backscatter, 0) })
                    localT.push({ x: xValue, y: detailPage.toNumber(sorted[i].ui_transmission, 0) })
                }

                rows = sorted
                bsPoints = localBs
                tPoints = localT
                chartMinX = localBs.length > 0 ? localBs[0].x : 0
                chartMaxX = localBs.length > 1 ? localBs[localBs.length - 1].x : chartMinX + 1
                if (chartMaxX <= chartMinX)
                    chartMaxX = chartMinX + 1
                chartMinY = 0
                chartMaxY = 1
                xAxisTickValues = detailPage.buildTimeTicks(chartMinX, chartMaxX, 6)
                yAxisLabels = detailPage.makeAxisLabels(0, 1, 6, 2)
            }

            Component.onCompleted: loadUniformityData()

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
                        text: qsTr("数据库中暂无该实验的均匀度数据")
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
                                        model: uniformityPanel.yAxisLabels.length
                                        Rectangle {
                                            width: parent.width
                                            height: 1
                                            color: "#EEF3F8"
                                            y: index * (parent.height / Math.max(uniformityPanel.yAxisLabels.length - 1, 1))
                                        }
                                    }

                                    Repeater {
                                        model: uniformityPanel.xAxisTickValues
                                        Rectangle {
                                            width: 1
                                            height: parent.height
                                            color: "#EEF3F8"
                                            x: (modelData - uniformityPanel.chartMinX) / Math.max(uniformityPanel.chartMaxX - uniformityPanel.chartMinX, 1) * parent.width
                                        }
                                    }

                                    CurveItem {
                                        anchors.fill: parent
                                        lineColor: "#21A366"
                                        lineWidth: 2
                                        autoScale: false
                                        minXValue: uniformityPanel.chartMinX
                                        maxXValue: uniformityPanel.chartMaxX
                                        minYValue: 0
                                        maxYValue: 1
                                        dataPoints: uniformityPanel.tPoints
                                    }

                                    Text {
                                        anchors.left: parent.left
                                        anchors.leftMargin: -50
                                        anchors.verticalCenter: parent.verticalCenter
                                        text: "UI\n(T)"
                                        font.pixelSize: 12
                                        font.family: "Microsoft YaHei"
                                        color: "#6E8096"
                                        font.bold: true
                                        horizontalAlignment: Text.AlignHCenter
                                        verticalAlignment: Text.AlignVCenter
                                    }

                                    Repeater {
                                        model: uniformityPanel.yAxisLabels
                                        delegate: Text {
                                            anchors.right: parent.left
                                            anchors.rightMargin: 10
                                            y: index * (parent.height / Math.max(uniformityPanel.yAxisLabels.length - 1, 1)) - height / 2
                                            text: uniformityPanel.yAxisLabels[uniformityPanel.yAxisLabels.length - 1 - index]
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
                                    anchors.bottomMargin: 45

                                    Repeater {
                                        model: uniformityPanel.yAxisLabels.length
                                        Rectangle {
                                            width: parent.width
                                            height: 1
                                            color: "#EEF3F8"
                                            y: index * (parent.height / Math.max(uniformityPanel.yAxisLabels.length - 1, 1))
                                        }
                                    }

                                    Repeater {
                                        model: uniformityPanel.xAxisTickValues
                                        Rectangle {
                                            width: 1
                                            height: parent.height
                                            color: "#EEF3F8"
                                            x: (modelData - uniformityPanel.chartMinX) / Math.max(uniformityPanel.chartMaxX - uniformityPanel.chartMinX, 1) * parent.width
                                        }
                                    }

                                    CurveItem {
                                        anchors.fill: parent
                                        lineColor: "#2F7CF6"
                                        lineWidth: 2
                                        autoScale: false
                                        minXValue: uniformityPanel.chartMinX
                                        maxXValue: uniformityPanel.chartMaxX
                                        minYValue: 0
                                        maxYValue: 1
                                        dataPoints: uniformityPanel.bsPoints
                                    }

                                    Text {
                                        anchors.left: parent.left
                                        anchors.leftMargin: -50
                                        anchors.verticalCenter: parent.verticalCenter
                                        text: "UI\n(BS)"
                                        font.pixelSize: 12
                                        font.family: "Microsoft YaHei"
                                        color: "#6E8096"
                                        font.bold: true
                                        horizontalAlignment: Text.AlignHCenter
                                        verticalAlignment: Text.AlignVCenter
                                    }

                                    Repeater {
                                        model: uniformityPanel.yAxisLabels
                                        delegate: Text {
                                            anchors.right: parent.left
                                            anchors.rightMargin: 10
                                            y: index * (parent.height / Math.max(uniformityPanel.yAxisLabels.length - 1, 1)) - height / 2
                                            text: uniformityPanel.yAxisLabels[uniformityPanel.yAxisLabels.length - 1 - index]
                                            font.pixelSize: 11
                                            font.family: "Microsoft YaHei"
                                            color: "#7A8CA5"
                                        }
                                    }

                                    Repeater {
                                        model: uniformityPanel.xAxisTickValues
                                        delegate: Text {
                                            y: parent.height + 6
                                            x: (modelData - uniformityPanel.chartMinX) / Math.max(uniformityPanel.chartMaxX - uniformityPanel.chartMinX, 1) * parent.width - width / 2
                                            text: detailPage.formatNumber(modelData, 1)
                                            font.pixelSize: 11
                                            font.family: "Microsoft YaHei"
                                            color: "#7A8CA5"
                                        }
                                    }

                                    Text {
                                        anchors.horizontalCenter: parent.horizontalCenter
                                        anchors.bottom: parent.bottom
                                        anchors.bottomMargin: -38
                                        text: qsTr("时间(min)")
                                        font.pixelSize: 12
                                        font.family: "Microsoft YaHei"
                                        color: "#6E8096"
                                        font.bold: true
                                    }
                                }
                            }
                        }

                        Rectangle {
                            anchors.fill: parent
                            visible: uniformityPanel.currentModeIndex !== 2
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
                            visible: uniformityPanel.currentModeIndex !== 2

                            Repeater {
                                model: uniformityPanel.yAxisLabels.length
                                Rectangle {
                                    width: parent.width
                                    height: 1
                                    color: "#EEF3F8"
                                    y: index * (parent.height / Math.max(uniformityPanel.yAxisLabels.length - 1, 1))
                                }
                            }

                            Repeater {
                                model: uniformityPanel.xAxisTickValues
                                Rectangle {
                                    width: 1
                                    height: parent.height
                                    color: "#EEF3F8"
                                    x: (modelData - uniformityPanel.chartMinX) / Math.max(uniformityPanel.chartMaxX - uniformityPanel.chartMinX, 1) * parent.width
                                }
                            }

                            CurveItem {
                                anchors.fill: parent
                                lineColor: uniformityPanel.currentModeIndex === 0 ? "#2F7CF6" : "#21A366"
                                lineWidth: 2
                                autoScale: false
                                minXValue: uniformityPanel.chartMinX
                                maxXValue: uniformityPanel.chartMaxX
                                minYValue: 0
                                maxYValue: 1
                                dataPoints: uniformityPanel.currentModeIndex === 0 ? uniformityPanel.bsPoints : uniformityPanel.tPoints
                            }

                            Text {
                                anchors.left: parent.left
                                anchors.leftMargin: -50
                                anchors.verticalCenter: parent.verticalCenter
                                text: uniformityPanel.currentModeIndex === 0 ? "UI\n(BS)" : "UI\n(T)"
                                font.pixelSize: 12
                                font.family: "Microsoft YaHei"
                                color: "#6E8096"
                                font.bold: true
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }

                            Repeater {
                                model: uniformityPanel.yAxisLabels
                                delegate: Text {
                                    anchors.right: parent.left
                                    anchors.rightMargin: 10
                                    y: index * (parent.height / Math.max(uniformityPanel.yAxisLabels.length - 1, 1)) - height / 2
                                    text: uniformityPanel.yAxisLabels[uniformityPanel.yAxisLabels.length - 1 - index]
                                    font.pixelSize: 11
                                    font.family: "Microsoft YaHei"
                                    color: "#7A8CA5"
                                }
                            }

                            Repeater {
                                model: uniformityPanel.xAxisTickValues
                                delegate: Text {
                                    y: parent.height + 6
                                    x: (modelData - uniformityPanel.chartMinX) / Math.max(uniformityPanel.chartMaxX - uniformityPanel.chartMinX, 1) * parent.width - width / 2
                                    text: detailPage.formatNumber(modelData, 1)
                                    font.pixelSize: 11
                                    font.family: "Microsoft YaHei"
                                    color: "#7A8CA5"
                                }
                            }

                            Text {
                                anchors.horizontalCenter: parent.horizontalCenter
                                anchors.bottom: parent.bottom
                                anchors.bottomMargin: -38
                                text: qsTr("时间(min)")
                                font.pixelSize: 12
                                font.family: "Microsoft YaHei"
                                color: "#6E8096"
                                font.bold: true
                            }
                        }
                    }
                }
            }
        }
    }

    Component {
        id: lightIntensityAvgComponent

        Rectangle {
            id: avgPanel
            color: "#FFFFFF"
            property int currentModeIndex: 0
            property var modeTitles: [qsTr("背射光"), qsTr("透射光"), qsTr("背射光+透射光")]
            property var rows: []
            property var bsPoints: []
            property var tPoints: []
            property real chartMinX: 0
            property real chartMaxX: 1
            property real chartMinY: 0
            property real chartMaxY: 1
            property var xAxisTickValues: [0, 1]
            property var yAxisLabels: detailPage.makeAxisLabels(chartMinY, chartMaxY, 6, 1)

            function loadAverageData() {
                rows = []
                bsPoints = []
                tPoints = []
                if (!experimentData || experimentData.id === undefined || !data_ctrl)
                    return

                var source = data_ctrl.getLightIntensityAverages(Number(experimentData.id))
                if (!source || source.length === 0)
                    return

                var sorted = source.slice(0)
                sorted.sort(function(a, b) {
                    return detailPage.toNumber(a.scan_elapsed_ms, 0) - detailPage.toNumber(b.scan_elapsed_ms, 0)
                })

                var localBs = []
                var localT = []
                var minY = Number.POSITIVE_INFINITY
                var maxY = Number.NEGATIVE_INFINITY
                for (var i = 0; i < sorted.length; ++i) {
                    var xValue = detailPage.toNumber(sorted[i].scan_elapsed_ms, 0) / 60000.0
                    var bsValue = detailPage.toNumber(sorted[i].avg_backscatter, 0)
                    var tValue = detailPage.toNumber(sorted[i].avg_transmission, 0)
                    localBs.push({ x: xValue, y: bsValue })
                    localT.push({ x: xValue, y: tValue })
                    minY = Math.min(minY, bsValue, tValue)
                    maxY = Math.max(maxY, bsValue, tValue)
                }

                rows = sorted
                bsPoints = localBs
                tPoints = localT
                chartMinX = localBs.length > 0 ? localBs[0].x : 0
                chartMaxX = localBs.length > 1 ? localBs[localBs.length - 1].x : chartMinX + 1
                if (chartMaxX <= chartMinX)
                    chartMaxX = chartMinX + 1
                chartMinY = isFinite(minY) ? detailPage.paddedMin(minY, maxY, 1) : 0
                chartMaxY = isFinite(maxY) ? detailPage.paddedMax(maxY, minY, 1) : 1
                xAxisTickValues = detailPage.buildTimeTicks(chartMinX, chartMaxX, 6)
                yAxisLabels = detailPage.makeAxisLabels(chartMinY, chartMaxY, 6, 1)
            }

            Component.onCompleted: loadAverageData()

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 18
                spacing: 16

                Row {
                    spacing: 8
                    Repeater {
                        model: avgPanel.modeTitles
                        delegate: Button {
                            width: 116
                            height: 28
                            text: modelData
                            onClicked: avgPanel.currentModeIndex = index
                            contentItem: Text {
                                text: parent.text
                                font.pixelSize: 12
                                font.family: "Microsoft YaHei"
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                                color: avgPanel.currentModeIndex === index ? "#FFFFFF" : "#4A89DC"
                            }
                            background: Rectangle {
                                color: avgPanel.currentModeIndex === index ? "#4A89DC" : "#FFFFFF"
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
                        visible: avgPanel.rows.length === 0
                        text: qsTr("数据库中暂无该实验的光强平均值数据")
                        font.pixelSize: 15
                        font.family: "Microsoft YaHei"
                        color: "#7A8CA5"
                    }

                    Item {
                        anchors.fill: parent
                        anchors.margins: 12
                        visible: avgPanel.rows.length > 0

                        ColumnLayout {
                            anchors.fill: parent
                            spacing: 12
                            visible: avgPanel.currentModeIndex === 2

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
                                        model: avgPanel.yAxisLabels.length
                                        Rectangle {
                                            width: parent.width
                                            height: 1
                                            color: "#EEF3F8"
                                            y: index * (parent.height / Math.max(avgPanel.yAxisLabels.length - 1, 1))
                                        }
                                    }

                                    Repeater {
                                        model: avgPanel.xAxisTickValues
                                        Rectangle {
                                            width: 1
                                            height: parent.height
                                            color: "#EEF3F8"
                                            x: (modelData - avgPanel.chartMinX) / Math.max(avgPanel.chartMaxX - avgPanel.chartMinX, 1) * parent.width
                                        }
                                    }

                                    CurveItem {
                                        anchors.fill: parent
                                        lineColor: "#21A366"
                                        lineWidth: 2
                                        autoScale: false
                                        minXValue: avgPanel.chartMinX
                                        maxXValue: avgPanel.chartMaxX
                                        minYValue: avgPanel.chartMinY
                                        maxYValue: avgPanel.chartMaxY
                                        dataPoints: avgPanel.tPoints
                                    }

                                    Text {
                                        anchors.left: parent.left
                                        anchors.leftMargin: -50
                                        anchors.verticalCenter: parent.verticalCenter
                                        text: "Avg\n(T)"
                                        font.pixelSize: 12
                                        font.family: "Microsoft YaHei"
                                        color: "#6E8096"
                                        font.bold: true
                                        horizontalAlignment: Text.AlignHCenter
                                        verticalAlignment: Text.AlignVCenter
                                    }

                                    Repeater {
                                        model: avgPanel.yAxisLabels
                                        delegate: Text {
                                            anchors.right: parent.left
                                            anchors.rightMargin: 10
                                            y: index * (parent.height / Math.max(avgPanel.yAxisLabels.length - 1, 1)) - height / 2
                                            text: avgPanel.yAxisLabels[avgPanel.yAxisLabels.length - 1 - index]
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
                                    anchors.bottomMargin: 45

                                    Repeater {
                                        model: avgPanel.yAxisLabels.length
                                        Rectangle {
                                            width: parent.width
                                            height: 1
                                            color: "#EEF3F8"
                                            y: index * (parent.height / Math.max(avgPanel.yAxisLabels.length - 1, 1))
                                        }
                                    }

                                    Repeater {
                                        model: avgPanel.xAxisTickValues
                                        Rectangle {
                                            width: 1
                                            height: parent.height
                                            color: "#EEF3F8"
                                            x: (modelData - avgPanel.chartMinX) / Math.max(avgPanel.chartMaxX - avgPanel.chartMinX, 1) * parent.width
                                        }
                                    }

                                    CurveItem {
                                        anchors.fill: parent
                                        lineColor: "#2F7CF6"
                                        lineWidth: 2
                                        autoScale: false
                                        minXValue: avgPanel.chartMinX
                                        maxXValue: avgPanel.chartMaxX
                                        minYValue: avgPanel.chartMinY
                                        maxYValue: avgPanel.chartMaxY
                                        dataPoints: avgPanel.bsPoints
                                    }

                                    Text {
                                        anchors.left: parent.left
                                        anchors.leftMargin: -50
                                        anchors.verticalCenter: parent.verticalCenter
                                        text: "Avg\n(BS)"
                                        font.pixelSize: 12
                                        font.family: "Microsoft YaHei"
                                        color: "#6E8096"
                                        font.bold: true
                                        horizontalAlignment: Text.AlignHCenter
                                        verticalAlignment: Text.AlignVCenter
                                    }

                                    Repeater {
                                        model: avgPanel.yAxisLabels
                                        delegate: Text {
                                            anchors.right: parent.left
                                            anchors.rightMargin: 10
                                            y: index * (parent.height / Math.max(avgPanel.yAxisLabels.length - 1, 1)) - height / 2
                                            text: avgPanel.yAxisLabels[avgPanel.yAxisLabels.length - 1 - index]
                                            font.pixelSize: 11
                                            font.family: "Microsoft YaHei"
                                            color: "#7A8CA5"
                                        }
                                    }

                                    Repeater {
                                        model: avgPanel.xAxisTickValues
                                        delegate: Text {
                                            y: parent.height + 6
                                            x: (modelData - avgPanel.chartMinX) / Math.max(avgPanel.chartMaxX - avgPanel.chartMinX, 1) * parent.width - width / 2
                                            text: detailPage.formatNumber(modelData, 1)
                                            font.pixelSize: 11
                                            font.family: "Microsoft YaHei"
                                            color: "#7A8CA5"
                                        }
                                    }

                                    Text {
                                        anchors.horizontalCenter: parent.horizontalCenter
                                        anchors.bottom: parent.bottom
                                        anchors.bottomMargin: -38
                                        text: qsTr("时间(min)")
                                        font.pixelSize: 12
                                        font.family: "Microsoft YaHei"
                                        color: "#6E8096"
                                        font.bold: true
                                    }
                                }
                            }
                        }

                        Rectangle {
                            anchors.fill: parent
                            visible: avgPanel.currentModeIndex !== 2
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
                            visible: avgPanel.currentModeIndex !== 2

                            Repeater {
                                model: avgPanel.yAxisLabels.length
                                Rectangle {
                                    width: parent.width
                                    height: 1
                                    color: "#EEF3F8"
                                    y: index * (parent.height / Math.max(avgPanel.yAxisLabels.length - 1, 1))
                                }
                            }

                            Repeater {
                                model: avgPanel.xAxisTickValues
                                Rectangle {
                                    width: 1
                                    height: parent.height
                                    color: "#EEF3F8"
                                    x: (modelData - avgPanel.chartMinX) / Math.max(avgPanel.chartMaxX - avgPanel.chartMinX, 1) * parent.width
                                }
                            }

                            CurveItem {
                                anchors.fill: parent
                                lineColor: avgPanel.currentModeIndex === 0 ? "#2F7CF6" : "#21A366"
                                lineWidth: 2
                                autoScale: false
                                minXValue: avgPanel.chartMinX
                                maxXValue: avgPanel.chartMaxX
                                minYValue: avgPanel.chartMinY
                                maxYValue: avgPanel.chartMaxY
                                dataPoints: avgPanel.currentModeIndex === 0 ? avgPanel.bsPoints : avgPanel.tPoints
                            }

                            Text {
                                anchors.left: parent.left
                                anchors.leftMargin: -50
                                anchors.verticalCenter: parent.verticalCenter
                                text: avgPanel.currentModeIndex === 0 ? "Avg\n(BS)" : "Avg\n(T)"
                                font.pixelSize: 12
                                font.family: "Microsoft YaHei"
                                color: "#6E8096"
                                font.bold: true
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }

                            Repeater {
                                model: avgPanel.yAxisLabels
                                delegate: Text {
                                    anchors.right: parent.left
                                    anchors.rightMargin: 10
                                    y: index * (parent.height / Math.max(avgPanel.yAxisLabels.length - 1, 1)) - height / 2
                                    text: avgPanel.yAxisLabels[avgPanel.yAxisLabels.length - 1 - index]
                                    font.pixelSize: 11
                                    font.family: "Microsoft YaHei"
                                    color: "#7A8CA5"
                                }
                            }

                            Repeater {
                                model: avgPanel.xAxisTickValues
                                delegate: Text {
                                    y: parent.height + 6
                                    x: (modelData - avgPanel.chartMinX) / Math.max(avgPanel.chartMaxX - avgPanel.chartMinX, 1) * parent.width - width / 2
                                    text: detailPage.formatNumber(modelData, 1)
                                    font.pixelSize: 11
                                    font.family: "Microsoft YaHei"
                                    color: "#7A8CA5"
                                }
                            }

                            Text {
                                anchors.horizontalCenter: parent.horizontalCenter
                                anchors.bottom: parent.bottom
                                anchors.bottomMargin: -38
                                text: qsTr("时间(min)")
                                font.pixelSize: 12
                                font.family: "Microsoft YaHei"
                                color: "#6E8096"
                                font.bold: true
                            }
                        }
                    }
                }
            }
        }
    }
}
