import QtQuick 2.12
import CustomComponents 1.0

Item {
    id: root

    property var dataPoints: []
    property color lineColor: "#2F7CF6"
    property real lineWidth: 2
    property real minXValue: 0
    property real maxXValue: 1
    property real minYValue: 0
    property real maxYValue: 1
    property var xAxisTickValues: [0, 1]
    property var yAxisLabels: ["1.0", "0.0"]
    property string yAxisTitle: ""
    property string xAxisTitle: ""
    property bool showXAxisLabels: true
    property bool showXAxisTitle: true
    property int leftMargin: 58
    property int rightMargin: 16
    property int topMargin: 16
    property int bottomMargin: showXAxisLabels || showXAxisTitle ? 45 : 12
    property int yAxisTitleOffset: 50
    property int xAxisTitleOffset: 38
    property var formatXLabel: function(value) { return value }
    property bool zoomActive: false
    property real visibleMinX: 0
    property real visibleMaxX: 1
    property real visibleMinY: 0
    property real visibleMaxY: 1
    property bool cursorVisible: false
    property point cursorPoint: Qt.point(0, 0)
    property point focusedDataPoint: Qt.point(0, 0)
    property bool isSelecting: false
    property point selectionStart: Qt.point(0, 0)
    property point selectionEnd: Qt.point(0, 0)

    function syncVisibleRange() {
        if (!zoomActive) {
            visibleMinX = minXValue
            visibleMaxX = maxXValue
            visibleMinY = minYValue
            visibleMaxY = maxYValue
        }
    }

    function tickCount(source) {
        return Math.max(2, source && source.length ? source.length : 2)
    }

    function makeTicks(minValue, maxValue, count) {
        var result = []
        var actualCount = Math.max(2, count)
        var range = maxValue - minValue
        if (Math.abs(range) < 0.000001)
            range = 1

        for (var i = 0; i < actualCount; ++i) {
            var ratio = actualCount === 1 ? 0 : i / (actualCount - 1)
            result.push(minValue + range * ratio)
        }
        return result
    }

    function labelDigits(source) {
        if (!source || source.length === 0)
            return 1

        var digits = 0
        for (var i = 0; i < source.length; ++i) {
            var label = String(source[i])
            var dotIndex = label.indexOf(".")
            if (dotIndex >= 0)
                digits = Math.max(digits, label.length - dotIndex - 1)
        }
        return digits
    }

    function makeYLabels(minValue, maxValue, source) {
        var ticks = makeTicks(minValue, maxValue, tickCount(source))
        var digits = labelDigits(source)
        var labels = []
        for (var i = 0; i < ticks.length; ++i)
            labels.push(Number(ticks[i]).toFixed(digits))
        return labels
    }

    function clampToPlotArea(point) {
        return Qt.point(
            Math.max(0, Math.min(plotArea.width, point.x)),
            Math.max(0, Math.min(plotArea.height, point.y))
        )
    }

    function xToPosition(value) {
        return (value - visibleMinX) / Math.max(visibleMaxX - visibleMinX, 0.000001) * plotArea.width
    }

    function yToPosition(value) {
        return plotArea.height - (value - visibleMinY) / Math.max(visibleMaxY - visibleMinY, 0.000001) * plotArea.height
    }

    function positionToX(position) {
        return visibleMinX + position / Math.max(plotArea.width, 1) * (visibleMaxX - visibleMinX)
    }

    function positionToY(position) {
        return visibleMaxY - position / Math.max(plotArea.height, 1) * (visibleMaxY - visibleMinY)
    }

    function resetView() {
        zoomActive = false
        cursorVisible = false
        syncVisibleRange()
    }

    function toDataPoint(value) {
        if (value === undefined || value === null)
            return null

        if (value.x !== undefined && value.y !== undefined) {
            var pointX = Number(value.x)
            var pointY = Number(value.y)
            if (!isNaN(pointX) && !isNaN(pointY))
                return Qt.point(pointX, pointY)
        }

        if (value.timestamp !== undefined && value.value !== undefined) {
            var timestampX = Number(value.timestamp)
            var valueY = Number(value.value)
            if (!isNaN(timestampX) && !isNaN(valueY))
                return Qt.point(timestampX, valueY)
        }

        return null
    }

    function findNearestDataPoint(plotPoint) {
        if (!dataPoints || dataPoints.length === 0)
            return null

        var nearestPoint = null
        var nearestDistance = Number.POSITIVE_INFINITY
        for (var i = 0; i < dataPoints.length; ++i) {
            var candidate = toDataPoint(dataPoints[i])
            if (!candidate)
                continue

            if (candidate.x < visibleMinX || candidate.x > visibleMaxX || candidate.y < visibleMinY || candidate.y > visibleMaxY)
                continue

            var screenX = xToPosition(candidate.x)
            var screenY = yToPosition(candidate.y)
            var dx = screenX - plotPoint.x
            var dy = screenY - plotPoint.y
            var distance = dx * dx + dy * dy
            if (distance < nearestDistance) {
                nearestDistance = distance
                nearestPoint = candidate
            }
        }
        return nearestPoint
    }

    readonly property var currentXAxisTickValues: makeTicks(visibleMinX, visibleMaxX, tickCount(xAxisTickValues))
    readonly property var currentYAxisLabels: makeYLabels(visibleMinY, visibleMaxY, yAxisLabels)

    onMinXValueChanged: syncVisibleRange()
    onMaxXValueChanged: syncVisibleRange()
    onMinYValueChanged: syncVisibleRange()
    onMaxYValueChanged: syncVisibleRange()
    Component.onCompleted: syncVisibleRange()

    Rectangle {
        anchors.fill: parent
        radius: 6
        color: "#FFFFFF"
        border.color: "#DCE6F2"
        border.width: 1
    }

    Item {
        id: plotArea
        anchors.fill: parent
        anchors.leftMargin: root.leftMargin
        anchors.rightMargin: root.rightMargin
        anchors.topMargin: root.topMargin
        anchors.bottomMargin: root.bottomMargin

        Repeater {
            model: root.currentYAxisLabels.length
            Rectangle {
                width: parent.width
                height: 1
                color: "#EEF3F8"
                y: index * (parent.height / Math.max(root.currentYAxisLabels.length - 1, 1))
            }
        }

        Repeater {
            model: root.currentXAxisTickValues
            Rectangle {
                width: 1
                height: parent.height
                color: "#EEF3F8"
                x: root.xToPosition(modelData)
            }
        }

        Item {
            anchors.fill: parent
            clip: true

            CurveItem {
                anchors.fill: parent
                lineColor: root.lineColor
                lineWidth: root.lineWidth
                autoScale: false
                minXValue: root.visibleMinX
                maxXValue: root.visibleMaxX
                minYValue: root.visibleMinY
                maxYValue: root.visibleMaxY
                dataPoints: root.dataPoints
            }

            Rectangle {
                visible: root.cursorVisible
                x: root.cursorPoint.x
                y: 0
                width: 1
                height: parent.height
                color: "#FF6B6B"
            }

            Rectangle {
                visible: root.cursorVisible
                x: 0
                y: root.cursorPoint.y
                width: parent.width
                height: 1
                color: "#FF6B6B"
            }

            Rectangle {
                visible: root.cursorVisible
                x: root.cursorPoint.x - width / 2
                y: root.cursorPoint.y - height / 2
                width: 8
                height: 8
                radius: 4
                color: "#FF6B6B"
                border.color: "#FFFFFF"
                border.width: 1
            }

            Rectangle {
                visible: root.isSelecting
                x: Math.min(root.selectionStart.x, root.selectionEnd.x)
                y: Math.min(root.selectionStart.y, root.selectionEnd.y)
                width: Math.abs(root.selectionEnd.x - root.selectionStart.x)
                height: Math.abs(root.selectionEnd.y - root.selectionStart.y)
                color: "#4A89DC33"
                border.color: "#4A89DC"
                border.width: 1
            }

            MouseArea {
                anchors.fill: parent
                hoverEnabled: true
                acceptedButtons: Qt.LeftButton

                property point pressPoint: Qt.point(0, 0)
                property bool dragTriggered: false

                onPressed: {
                    pressPoint = root.clampToPlotArea(Qt.point(mouse.x, mouse.y))
                    root.selectionStart = pressPoint
                    root.selectionEnd = pressPoint
                    root.isSelecting = false
                    dragTriggered = false
                }

                onPositionChanged: {
                    if (!(pressedButtons & Qt.LeftButton))
                        return

                    var currentPoint = root.clampToPlotArea(Qt.point(mouse.x, mouse.y))
                    var deltaX = currentPoint.x - pressPoint.x
                    var deltaY = currentPoint.y - pressPoint.y
                    if (!dragTriggered && (Math.abs(deltaX) > 6 || Math.abs(deltaY) > 6))
                        dragTriggered = true

                    if (dragTriggered) {
                        root.isSelecting = true
                        root.selectionEnd = currentPoint
                    }
                }

                onReleased: {
                    var releasePoint = root.clampToPlotArea(Qt.point(mouse.x, mouse.y))
                    if (dragTriggered && root.isSelecting) {
                        root.selectionEnd = releasePoint
                        var left = Math.min(root.selectionStart.x, root.selectionEnd.x)
                        var right = Math.max(root.selectionStart.x, root.selectionEnd.x)
                        var top = Math.min(root.selectionStart.y, root.selectionEnd.y)
                        var bottom = Math.max(root.selectionStart.y, root.selectionEnd.y)

                        if (right - left > 8 && bottom - top > 8) {
                            root.visibleMinX = root.positionToX(left)
                            root.visibleMaxX = root.positionToX(right)
                            root.visibleMaxY = root.positionToY(top)
                            root.visibleMinY = root.positionToY(bottom)
                            root.zoomActive = true
                            root.cursorVisible = false
                        }
                        root.isSelecting = false
                    } else {
                        var nearestPoint = root.findNearestDataPoint(releasePoint)
                        if (nearestPoint) {
                            root.focusedDataPoint = nearestPoint
                            root.cursorPoint = Qt.point(root.xToPosition(nearestPoint.x), root.yToPosition(nearestPoint.y))
                            root.cursorVisible = true
                        } else {
                            root.cursorVisible = false
                        }
                    }
                }

                onDoubleClicked: root.resetView()
            }
        }

        Text {
            visible: root.yAxisTitle !== ""
            anchors.left: parent.left
            anchors.leftMargin: -root.yAxisTitleOffset
            anchors.verticalCenter: parent.verticalCenter
            text: root.yAxisTitle
            font.pixelSize: 12
            font.family: "Microsoft YaHei"
            color: "#6E8096"
            font.bold: true
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }

        Repeater {
            model: root.currentYAxisLabels
            delegate: Text {
                anchors.right: parent.left
                anchors.rightMargin: 10
                y: index * (parent.height / Math.max(root.currentYAxisLabels.length - 1, 1)) - height / 2
                text: root.currentYAxisLabels[root.currentYAxisLabels.length - 1 - index]
                font.pixelSize: 11
                font.family: "Microsoft YaHei"
                color: "#7A8CA5"
            }
        }

        Repeater {
            model: root.showXAxisLabels ? root.currentXAxisTickValues : []
            delegate: Text {
                y: parent.height + 6
                x: root.xToPosition(modelData) - width / 2
                text: root.formatXLabel(modelData)
                font.pixelSize: 11
                font.family: "Microsoft YaHei"
                color: "#7A8CA5"
            }
        }

        Rectangle {
            visible: root.cursorVisible
            x: Math.min(Math.max(root.cursorPoint.x + 10, 0), Math.max(parent.width - width, 0))
            y: Math.max(root.cursorPoint.y - height - 8, 0)
            width: 112
            height: 40
            radius: 4
            color: "#2F3A4A"
            border.color: "#FF6B6B"
            border.width: 1

            Column {
                anchors.centerIn: parent
                spacing: 2

                Text {
                    text: "X: " + root.formatXLabel(root.focusedDataPoint.x)
                    font.pixelSize: 10
                    font.family: "Microsoft YaHei"
                    color: "#FFFFFF"
                }

                Text {
                    text: "Y: " + Number(root.focusedDataPoint.y).toFixed(labelDigits(yAxisLabels))
                    font.pixelSize: 10
                    font.family: "Microsoft YaHei"
                    color: "#FFFFFF"
                }
            }
        }

        Text {
            visible: root.showXAxisTitle && root.xAxisTitle !== ""
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.bottom: parent.bottom
            anchors.bottomMargin: -root.xAxisTitleOffset
            text: root.xAxisTitle
            font.pixelSize: 12
            font.family: "Microsoft YaHei"
            color: "#6E8096"
            font.bold: true
        }
    }
}
