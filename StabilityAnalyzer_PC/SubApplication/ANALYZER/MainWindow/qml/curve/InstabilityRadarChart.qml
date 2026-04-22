import QtQuick 2.12
import QtQuick.Layouts 1.12

Item {
    id: radarChart

    // polygon 数组中的每一项代表一个时间点，
    // values 顺序与 axisLabels 一一对应。
    property var polygons: []
    property var axisLabels: [qsTr("Ius - 整体"), qsTr("Ius - 底部"), qsTr("Ius - 中部"), qsTr("Ius - 顶部")]
    property real maxValue: 1
    property int tickCount: 5
    property color gridColor: "#AEB8C5"
    property color axisColor: "#3C4654"

    onVisibleChanged: {
        gridCanvas.requestPaint()
        legendCanvas.requestPaint()
    }

    function toNumber(value, fallback) {
        var parsed = Number(value)
        return isNaN(parsed) ? fallback : parsed
    }

    function axisPoint(centerX, centerY, radiusValue, axisIndex, axisCount) {
        var angle = (-Math.PI / 2) + axisIndex * 2 * Math.PI / Math.max(axisCount, 1)
        return {
            x: centerX + Math.cos(angle) * radiusValue,
            y: centerY + Math.sin(angle) * radiusValue
        }
    }

    function polygonPath(centerX, centerY, radiusValue, values) {
        // 把一组业务值映射成雷达图四周的顶点坐标。
        var points = []
        var count = Math.min(axisLabels.length, values.length)
        var safeMax = Math.max(0.000001, maxValue)
        for (var i = 0; i < count; ++i) {
            var normalized = Math.max(0, toNumber(values[i], 0)) / safeMax
            points.push(axisPoint(centerX, centerY, radiusValue * normalized, i, count))
        }
        return points
    }

    RowLayout {
        anchors.fill: parent
        spacing: 18

        Item {
            id: chartArea
            Layout.fillWidth: true
            Layout.fillHeight: true

            readonly property real centerX: width * 0.52
            readonly property real centerY: height * 0.5
            readonly property real radius: Math.max(40, Math.min(width * 0.35, height * 0.42))

            Canvas {
                id: gridCanvas
                anchors.fill: parent

                Connections {
                    target: radarChart
                    function onPolygonsChanged() { gridCanvas.requestPaint() }
                    function onMaxValueChanged() { gridCanvas.requestPaint() }
                    function onAxisLabelsChanged() { gridCanvas.requestPaint() }
                    function onTickCountChanged() { gridCanvas.requestPaint() }
                    function onVisibleChanged() { gridCanvas.requestPaint() }
                }

                onWidthChanged: requestPaint()
                onHeightChanged: requestPaint()

                onPaint: {
                    var ctx = getContext("2d")
                    ctx.clearRect(0, 0, width, height)

                    var axisCount = Math.max(radarChart.axisLabels.length, 4)
                    var centerX = chartArea.centerX
                    var centerY = chartArea.centerY
                    var radius = chartArea.radius
                    var tickTotal = Math.max(2, radarChart.tickCount)

                    ctx.strokeStyle = radarChart.gridColor
                    ctx.lineWidth = 1

                    // 先画底层圆环和轴线，再叠加每个时间点的多边形。
                    for (var tickIndex = 1; tickIndex <= tickTotal; ++tickIndex) {
                        var ringRadius = radius * tickIndex / tickTotal
                        ctx.beginPath()
                        ctx.arc(centerX, centerY, ringRadius, 0, Math.PI * 2)
                        ctx.stroke()
                    }

                    ctx.strokeStyle = radarChart.axisColor
                    ctx.lineWidth = 1.2
                    for (var axisIndex = 0; axisIndex < axisCount; ++axisIndex) {
                        var edge = radarChart.axisPoint(centerX, centerY, radius, axisIndex, axisCount)
                        ctx.beginPath()
                        ctx.moveTo(centerX, centerY)
                        ctx.lineTo(edge.x, edge.y)
                        ctx.stroke()
                    }

                    for (var polygonIndex = 0; polygonIndex < radarChart.polygons.length; ++polygonIndex) {
                        var polygon = radarChart.polygons[polygonIndex]
                        var points = radarChart.polygonPath(centerX, centerY, radius, polygon.values || [])
                        if (points.length < 3)
                            continue

                        ctx.beginPath()
                        ctx.moveTo(points[0].x, points[0].y)
                        for (var pointIndex = 1; pointIndex < points.length; ++pointIndex)
                            ctx.lineTo(points[pointIndex].x, points[pointIndex].y)
                        ctx.closePath()
                        ctx.strokeStyle = polygon.color || "#2F7CF6"
                        ctx.lineWidth = polygonIndex === radarChart.polygons.length - 1 ? 1.6 : 1
                        ctx.globalAlpha = polygonIndex === radarChart.polygons.length - 1 ? 0.9 : 0.55
                        ctx.stroke()
                    }

                    ctx.globalAlpha = 1.0
                }
            }

            Repeater {
                model: Math.max(2, radarChart.tickCount)

                delegate: Text {
                    readonly property real safeMaxValue: Math.max(0.000001, radarChart.maxValue)
                    readonly property real displayValue: safeMaxValue * index / Math.max(radarChart.tickCount - 1, 1)

                    x: chartArea.centerX + 6
                    y: chartArea.centerY - chartArea.radius * index / Math.max(radarChart.tickCount - 1, 1) - height / 2
                    text: index === 0 ? "0" : radarChart.toNumber(displayValue, 0).toFixed(0)
                    font.pixelSize: 12
                    font.family: "Microsoft YaHei"
                    color: "#2F3A4A"
                }
            }

            Repeater {
                model: radarChart.axisLabels

                delegate: Text {
                    readonly property var edge: radarChart.axisPoint(chartArea.centerX, chartArea.centerY, chartArea.radius + 16, index, radarChart.axisLabels.length)
                    x: edge.x - width / 2
                    y: edge.y - height / 2
                    text: modelData
                    font.pixelSize: 13
                    font.family: "Microsoft YaHei"
                    color: "#2F3A4A"
                }
            }
        }

        Item {
            Layout.preferredWidth: 108
            Layout.fillHeight: true

            Repeater {
                model: radarChart.polygons.length

                delegate: Text {
                    anchors.right: parent.right
                    y: radarChart.polygons.length <= 1 ? 0 : index * ((parent.height - height) / Math.max(radarChart.polygons.length - 1, 1))
                    text: radarChart.polygons[index].label || "--"
                    font.pixelSize: 12
                    font.family: "Microsoft YaHei"
                    color: "#2F3A4A"
                }
            }

            Canvas {
                id: legendCanvas
                anchors.top: parent.top
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                width: 32

                Connections {
                    target: radarChart
                    function onPolygonsChanged() { legendCanvas.requestPaint() }
                    function onVisibleChanged() { legendCanvas.requestPaint() }
                }

                onWidthChanged: requestPaint()
                onHeightChanged: requestPaint()

                onPaint: {
                    var ctx = getContext("2d")
                    ctx.clearRect(0, 0, width, height)
                    if (!radarChart.polygons || radarChart.polygons.length === 0)
                        return

                    // 右侧色带和雷达图复用同一批颜色，方便按时间反查。
                    var gradient = ctx.createLinearGradient(0, 0, 0, height)
                    var stopCount = Math.max(radarChart.polygons.length - 1, 1)
                    for (var i = 0; i < radarChart.polygons.length; ++i) {
                        gradient.addColorStop(stopCount === 0 ? 0 : i / stopCount, radarChart.polygons[i].color)
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
