import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import CustomComponents 1.0

Rectangle {
    id: curvePage
    color: "#FFFFFF"

    property int channelId: 0
    property int curveModeIndex: 0
    property var modeTitles: [qsTr("散射光"), qsTr("背射光"), qsTr("散射光+背射光")]

    readonly property bool hasData: realtime_curve_ctrl ? realtime_curve_ctrl.hasData : false
    readonly property var tPoints: realtime_curve_ctrl ? realtime_curve_ctrl.transmissionPoints : []
    readonly property var bPoints: realtime_curve_ctrl ? realtime_curve_ctrl.backscatterPoints : []

    readonly property real chartMinX: realtime_curve_ctrl ? realtime_curve_ctrl.minHeight : 0
    readonly property real chartMaxX: realtime_curve_ctrl ? realtime_curve_ctrl.maxHeight : 55
    readonly property real transMinY: realtime_curve_ctrl ? realtime_curve_ctrl.minTransmission : 0
    readonly property real transMaxY: realtime_curve_ctrl ? realtime_curve_ctrl.maxTransmission : 100
    readonly property real backMinY: realtime_curve_ctrl ? realtime_curve_ctrl.minBackscatter : 0
    readonly property real backMaxY: realtime_curve_ctrl ? realtime_curve_ctrl.maxBackscatter : 100

    readonly property bool isTransmission: curveModeIndex === 0
    readonly property var currentYLabels: isTransmission ? transYLabels : backYLabels
    readonly property real currentMinY: isTransmission ? transMinY : backMinY
    readonly property real currentMaxY: isTransmission ? transMaxY : backMaxY

    function buildYLabels(minVal, maxVal) {
        var labels = []
        var safeMin = Number(minVal)
        var safeMax = Number(maxVal)
        if (isNaN(safeMin) || isNaN(safeMax)) return [0, 1]
        var span = Math.max(safeMax - safeMin, 1)
        var step = span <= 10 ? 2 : span <= 50 ? 10 : span <= 200 ? 50 : 100
        var start = Math.floor(safeMin / step) * step
        for (var v = start; v <= safeMax + step * 0.01; v += step) {
            if (v >= safeMin - step * 0.01)
                labels.push(Math.round(v * 100) / 100)
        }
        if (labels.length < 2)
            labels = [safeMin, safeMax]
        return labels
    }

    function buildXTicks(minVal, maxVal) {
        var ticks = []
        var span = Math.max(maxVal - minVal, 1)
        var step = span <= 10 ? 2 : span <= 20 ? 5 : 10
        var start = Math.floor(minVal / step) * step
        for (var v = start; v <= maxVal + step * 0.01; v += step) {
            if (v >= minVal - step * 0.01)
                ticks.push(v)
        }
        if (ticks.length < 2)
            ticks = [minVal, maxVal]
        return ticks
    }

    readonly property var xTicks: buildXTicks(chartMinX, chartMaxX)
    readonly property var transYLabels: buildYLabels(transMinY, transMaxY)
    readonly property var backYLabels: buildYLabels(backMinY, backMaxY)

    Component.onCompleted: {
        if (realtime_curve_ctrl) {
            realtime_curve_ctrl.clearData()
            realtime_curve_ctrl.channel = channelId
        }
    }

    Component.onDestruction: {
        if (realtime_curve_ctrl)
            realtime_curve_ctrl.clearData()
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.leftMargin: 18
        anchors.topMargin: 10
        anchors.rightMargin: 10
        anchors.bottomMargin: 5
        spacing: 10

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Row {
                spacing: 6

                Repeater {
                    model: curvePage.modeTitles

                    delegate: Button {
                        width: 110
                        height: 28
                        text: modelData
                        onClicked: curvePage.curveModeIndex = index

                        contentItem: Text {
                            text: parent.text
                            font.pixelSize: 12
                            font.family: "Microsoft YaHei"
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                            color: curvePage.curveModeIndex === index ? "#FFFFFF" : "#4A89DC"
                        }

                        background: Rectangle {
                            color: curvePage.curveModeIndex === index ? "#4A89DC" : "#FFFFFF"
                            border.color: "#4A89DC"
                            border.width: 1
                        }
                    }
                }
            }

            Item { Layout.fillWidth: true }

//            Label {
//                visible: hasData
//                text: qsTr("通道") + " " + (channelId + 1)
//                font.pixelSize: 13
//                font.family: "Microsoft YaHei"
//                color: "#4A5D75"
//            }
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
                    visible: !hasData
                    text: qsTr("等待扫描数据...")
                    font.pixelSize: 15
                    font.family: "Microsoft YaHei"
                    color: "#7A8CA5"
                }

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 8
                    visible: hasData && curvePage.curveModeIndex === 2

                    Item {
                        Layout.fillWidth: true
                        Layout.fillHeight: true

                        Rectangle {
                            anchors.fill: parent
                            radius: 6
                            color: "#FFFFFF"
                            border.color: "#DCE6F2"
                            border.width: 1
                        }

                        Item {
                            id: dualTransChart
                            anchors.fill: parent
                            anchors.leftMargin: 50
                            anchors.rightMargin: 14
                            anchors.topMargin: 14
                            anchors.bottomMargin: 10

                            Repeater {
                                model: transYLabels.length
                                Rectangle {
                                    width: dualTransChart.width
                                    height: 1
                                    color: "#EEF3F8"
                                    y: index * (dualTransChart.height / Math.max(transYLabels.length - 1, 1))
                                }
                            }

                            Repeater {
                                model: xTicks
                                Rectangle {
                                    width: 1
                                    height: dualTransChart.height
                                    color: "#EEF3F8"
                                    x: (modelData - chartMinX) / Math.max(chartMaxX - chartMinX, 1) * dualTransChart.width
                                }
                            }

                            CurveItem {
                                anchors.fill: parent
                                lineColor: "#E74C3C"
                                lineWidth: 2
                                maxPoints: 5000
                                autoScale: false
                                minXValue: chartMinX
                                maxXValue: chartMaxX
                                minYValue: transMinY
                                maxYValue: transMaxY
                                dataPoints: tPoints
                            }

                            Text {
                                anchors.left: dualTransChart.left
                                anchors.leftMargin: -44
                                anchors.verticalCenter: dualTransChart.verticalCenter
                                text: "T\n(%)"
                                font.pixelSize: 11
                                font.family: "Microsoft YaHei"
                                color: "#6E8096"
                                font.bold: true
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }

                            Repeater {
                                model: transYLabels
                                delegate: Text {
                                    anchors.right: dualTransChart.left
                                    anchors.rightMargin: 6
                                    y: index * (dualTransChart.height / Math.max(transYLabels.length - 1, 1)) - height / 2
                                    text: transYLabels[transYLabels.length - 1 - index]
                                    font.pixelSize: 10
                                    font.family: "Microsoft YaHei"
                                    color: "#7A8CA5"
                                }
                            }
                        }
                    }

                    Item {
                        Layout.fillWidth: true
                        Layout.fillHeight: true

                        Rectangle {
                            anchors.fill: parent
                            radius: 6
                            color: "#FFFFFF"
                            border.color: "#DCE6F2"
                            border.width: 1
                        }

                        Item {
                            id: dualBackChart
                            anchors.fill: parent
                            anchors.leftMargin: 50
                            anchors.rightMargin: 14
                            anchors.topMargin: 14
                            anchors.bottomMargin: 40

                            Repeater {
                                model: backYLabels.length
                                Rectangle {
                                    width: dualBackChart.width
                                    height: 1
                                    color: "#EEF3F8"
                                    y: index * (dualBackChart.height / Math.max(backYLabels.length - 1, 1))
                                }
                            }

                            Repeater {
                                model: xTicks
                                Rectangle {
                                    width: 1
                                    height: dualBackChart.height
                                    color: "#EEF3F8"
                                    x: (modelData - chartMinX) / Math.max(chartMaxX - chartMinX, 1) * dualBackChart.width
                                }
                            }

                            CurveItem {
                                anchors.fill: parent
                                lineColor: "#E74C3C"
                                lineWidth: 2
                                maxPoints: 5000
                                autoScale: false
                                minXValue: chartMinX
                                maxXValue: chartMaxX
                                minYValue: backMinY
                                maxYValue: backMaxY
                                dataPoints: bPoints
                            }

                            Text {
                                anchors.left: dualBackChart.left
                                anchors.leftMargin: -44
                                anchors.verticalCenter: dualBackChart.verticalCenter
                                text: "BS\n(%)"
                                font.pixelSize: 11
                                font.family: "Microsoft YaHei"
                                color: "#6E8096"
                                font.bold: true
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }

                            Repeater {
                                model: backYLabels
                                delegate: Text {
                                    anchors.right: dualBackChart.left
                                    anchors.rightMargin: 6
                                    y: index * (dualBackChart.height / Math.max(backYLabels.length - 1, 1)) - height / 2
                                    text: backYLabels[backYLabels.length - 1 - index]
                                    font.pixelSize: 10
                                    font.family: "Microsoft YaHei"
                                    color: "#7A8CA5"
                                }
                            }

                            Repeater {
                                model: xTicks
                                delegate: Text {
                                    y: dualBackChart.height + 6
                                    x: (modelData - chartMinX) / Math.max(chartMaxX - chartMinX, 1) * dualBackChart.width - width / 2
                                    text: String(Math.round(modelData))
                                    font.pixelSize: 10
                                    font.family: "Microsoft YaHei"
                                    color: "#7A8CA5"
                                }
                            }

                            Text {
                                anchors.horizontalCenter: dualBackChart.horizontalCenter
                                anchors.bottom: dualBackChart.bottom
                                anchors.bottomMargin: -34
                                text: qsTr("高度(mm)")
                                font.pixelSize: 11
                                font.family: "Microsoft YaHei"
                                color: "#6E8096"
                                font.bold: true
                            }
                        }
                    }
                }

                Item {
                    id: singleModeContainer
                    anchors.fill: parent
                    visible: hasData && curvePage.curveModeIndex !== 2

                    Rectangle {
                        anchors.fill: parent
                        radius: 6
                        color: "#FFFFFF"
                        border.color: "#DCE6F2"
                        border.width: 1
                    }

                    Item {
                        id: singleChart
                        anchors.fill: parent
                        anchors.leftMargin: 50
                        anchors.rightMargin: 14
                        anchors.topMargin: 14
                        anchors.bottomMargin: 40

                        Repeater {
                            model: currentYLabels.length
                            Rectangle {
                                width: singleChart.width
                                height: 1
                                color: "#EEF3F8"
                                y: index * (singleChart.height / Math.max(currentYLabels.length - 1, 1))
                            }
                        }

                        Repeater {
                            model: xTicks
                            Rectangle {
                                width: 1
                                height: singleChart.height
                                color: "#EEF3F8"
                                x: (modelData - chartMinX) / Math.max(chartMaxX - chartMinX, 1) * singleChart.width
                            }
                        }

                        CurveItem {
                            anchors.fill: parent
                            lineColor: "#E74C3C"
                            lineWidth: 2
                            maxPoints: 5000
                            autoScale: false
                            minXValue: chartMinX
                            maxXValue: chartMaxX
                            minYValue: currentMinY
                            maxYValue: currentMaxY
                            dataPoints: isTransmission ? tPoints : bPoints
                        }

                        Text {
                            anchors.left: singleChart.left
                            anchors.leftMargin: -44
                            anchors.verticalCenter: singleChart.verticalCenter
                            text: isTransmission ? "T\n(%)" : "BS\n(%)"
                            font.pixelSize: 11
                            font.family: "Microsoft YaHei"
                            color: "#6E8096"
                            font.bold: true
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }

                        Repeater {
                            model: currentYLabels
                            delegate: Text {
                                anchors.right: singleChart.left
                                anchors.rightMargin: 6
                                y: index * (singleChart.height / Math.max(currentYLabels.length - 1, 1)) - height / 2
                                text: currentYLabels[currentYLabels.length - 1 - index]
                                font.pixelSize: 10
                                font.family: "Microsoft YaHei"
                                color: "#7A8CA5"
                            }
                        }

                        Repeater {
                            model: xTicks
                            delegate: Text {
                                y: singleChart.height + 6
                                x: (modelData - chartMinX) / Math.max(chartMaxX - chartMinX, 1) * singleChart.width - width / 2
                                text: String(Math.round(modelData))
                                font.pixelSize: 10
                                font.family: "Microsoft YaHei"
                                color: "#7A8CA5"
                            }
                        }

                        Text {
                            anchors.horizontalCenter: singleChart.horizontalCenter
                            anchors.bottom: singleChart.bottom
                            anchors.bottomMargin: -34
                            text: qsTr("高度(mm)")
                            font.pixelSize: 11
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
