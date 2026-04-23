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
    property int currentLightModeIndex: 0
    property var lightModeTitles: [qsTr("背射光"), qsTr("透射光"), qsTr("背射光+透射光")]
    property int currentDataModeIndex: 0
    property var dataModeTitles: [qsTr("原始数据"), qsTr("参比数据")]
    property int referenceCurveIndex: 0
    property real heightLowerBound: detailPage ? detailPage.floorToStep(detailPage.minHeightValue, 1) : 0
    property real heightUpperBound: detailPage ? detailPage.ceilToStep(detailPage.maxHeightValue, 1) : 55
    property var displayedCurves: []
    readonly property int displayedCurveCount: displayedCurves ? displayedCurves.length : 0

    readonly property real safeHeightLowerBound: {
        if (!detailPage)
            return 0
        return Math.max(detailPage.minHeightValue, Math.min(heightLowerBound, detailPage.maxHeightValue))
    }
    readonly property real safeHeightUpperBound: {
        if (!detailPage)
            return 10
        return Math.max(safeHeightLowerBound, Math.min(heightUpperBound, detailPage.maxHeightValue))
    }
    readonly property real chartMinX: safeHeightLowerBound
    readonly property real chartMaxX: Math.max(safeHeightUpperBound, safeHeightLowerBound + 1)
    readonly property var xAxisTickValues: buildHeightTicks(chartMinX, chartMaxX)
    readonly property var transmissionRange: curveRange(displayedCurves, "transmission_points")
    readonly property var backscatterRange: curveRange(displayedCurves, "backscatter_points")
    readonly property var transmissionLabels: buildYLabels(transmissionRange.minValue, transmissionRange.maxValue)
    readonly property var backscatterLabels: buildYLabels(backscatterRange.minValue, backscatterRange.maxValue)
    readonly property var singleModeRange: currentLightModeIndex === 0 ? backscatterRange : transmissionRange
    readonly property var singleModeLabels: currentLightModeIndex === 0 ? backscatterLabels : transmissionLabels

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

    function clampReferenceIndex(index) {
        if (!displayedCurves || displayedCurves.length <= 0)
            return 0
        return Math.max(0, Math.min(index, displayedCurves.length - 1))
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
        return Math.abs(safeHeightLowerBound - detailPage.minHeightValue) < 0.000001
                && Math.abs(safeHeightUpperBound - detailPage.maxHeightValue) < 0.000001
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

    function applyAnalysisSettings(referenceIndex, lowerBound, upperBound) {
        referenceCurveIndex = Math.max(0, referenceIndex)
        heightLowerBound = Math.max(detailPage.minHeightValue, Math.min(lowerBound, detailPage.maxHeightValue))
        heightUpperBound = Math.max(heightLowerBound, Math.min(upperBound, detailPage.maxHeightValue))
        loadDisplayedCurves()
    }

    function loadDisplayedCurves() {
        displayedCurves = []
        if (!detailPage || !detailPage.experimentData || detailPage.experimentData.id === undefined || !data_ctrl)
            return

        // 默认原始视图直接复用详情页已加载的原始曲线，避免首次进入光强页重复查询。
        if (currentDataModeIndex === 0 && isFullHeightRange() && lightCurves && lightCurves.length > 0) {
            displayedCurves = normalizeCurves(lightCurves)
            referenceCurveIndex = 0
            return
        }

        // 其余情况统一走 data_ctrl，由后端分析层完成裁剪/参比处理。
        var pointsPerCurve = Math.max(480, Math.round((lightIntensityPanel.width > 0 ? lightIntensityPanel.width : 1000) * 1.1))
        var curves = data_ctrl.getProcessedLightIntensityCurves(
                    Number(detailPage.experimentData.id),
                    pointsPerCurve,
                    referenceCurveIndex,
                    safeHeightLowerBound,
                    safeHeightUpperBound,
                    currentDataModeIndex === 1)
        if (!curves || curves.length === 0)
            return

        var normalizedCurves = normalizeCurves(curves)
        displayedCurves = normalizedCurves
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
                              detailPage.floorToStep(detailPage.minHeightValue, 1),
                              detailPage.ceilToStep(detailPage.maxHeightValue, 1))
    }

    onDetailPageChanged: resetAnalysisDefaults()
    onCurrentDataModeIndexChanged: loadDisplayedCurves()

    Connections {
        target: detailPage
        function onExperimentDataChanged() { lightIntensityPanel.resetAnalysisDefaults() }
        function onLightCurvesChanged() { lightIntensityPanel.loadDisplayedCurves() }
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
                    visible: displayedCurveCount === 0
                    text: qsTr("数据库中暂无该实验的光强曲线数据")
                    font.pixelSize: 15
                    font.family: "Microsoft YaHei"
                    color: "#7A8CA5"
                }

                RowLayout {
                    anchors.fill: parent
                    spacing: 14
                    visible: displayedCurveCount > 0

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
                                        model: displayedCurves
                                        CurveItem {
                                            anchors.fill: parent
                                            lineColor: modelData.color
                                            lineWidth: 2
                                            autoScale: false
                                            minXValue: lightIntensityPanel.chartMinX
                                            maxXValue: lightIntensityPanel.chartMaxX
                                            minYValue: lightIntensityPanel.transmissionRange.minValue
                                            maxYValue: lightIntensityPanel.transmissionRange.maxValue
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
                                        model: displayedCurves
                                        CurveItem {
                                            anchors.fill: parent
                                            lineColor: modelData.color
                                            lineWidth: 2
                                            autoScale: false
                                            minXValue: lightIntensityPanel.chartMinX
                                            maxXValue: lightIntensityPanel.chartMaxX
                                            minYValue: lightIntensityPanel.backscatterRange.minValue
                                            maxYValue: lightIntensityPanel.backscatterRange.maxValue
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
                                    model: displayedCurves
                                    CurveItem {
                                        anchors.fill: parent
                                        lineColor: modelData.color
                                        lineWidth: 2
                                        autoScale: false
                                        minXValue: lightIntensityPanel.chartMinX
                                        maxXValue: lightIntensityPanel.chartMaxX
                                        minYValue: lightIntensityPanel.singleModeRange.minValue
                                        maxYValue: lightIntensityPanel.singleModeRange.maxValue
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
                                        function onDisplayedCurvesChanged() { heatLegendCanvas.requestPaint() }
                                    }

                                    onWidthChanged: requestPaint()
                                    onHeightChanged: requestPaint()

                                    onPaint: {
                                        var ctx = getContext("2d")
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
                        text: qsTr("参比线")
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
                        text: qsTr("自定义高度分段")
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
