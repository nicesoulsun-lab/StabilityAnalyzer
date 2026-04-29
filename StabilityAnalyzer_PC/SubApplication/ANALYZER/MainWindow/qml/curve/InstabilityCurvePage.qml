import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import "../component"
import ".."


//涓嶇ǔ瀹氭€ф洸绾?
Rectangle {
    id: instabilityPanel

    // 涓嶇ǔ瀹氭€ч〉鎶婃渶閲嶇殑璁＄畻绉诲洖 C++/鏁版嵁搴撳眰锛?
    // QML 渚у彧璐熻矗鎸夋ā寮忔噿鍔犺浇缁撴灉鍜岀粍缁囧睍绀恒€?
    property var detailPage
    readonly property var experimentData: detailPage ? detailPage.experimentData : ({})
    property int currentModeIndex: 0
    property var modeTitles: [qsTr("\u6574\u4f53"), qsTr("\u5c40\u90e8"), qsTr("\u81ea\u5b9a\u4e49"), qsTr("\u603b\u89c8")]
    property var overallSeries: createEmptyInstabilitySeries(qsTr("\u6574\u4f53"), detailPage ? detailPage.minHeightValue : 0, detailPage ? detailPage.maxHeightValue : 0)
    property var bottomSeries: createEmptyInstabilitySeries(qsTr("\u5e95\u90e8"), detailPage ? detailPage.minHeightValue : 0, detailPage ? detailPage.minHeightValue : 0)
    property var middleSeries: createEmptyInstabilitySeries(qsTr("\u4e2d\u90e8"), detailPage ? detailPage.minHeightValue : 0, detailPage ? detailPage.minHeightValue : 0)
    property var topSeries: createEmptyInstabilitySeries(qsTr("\u9876\u90e8"), detailPage ? detailPage.minHeightValue : 0, detailPage ? detailPage.maxHeightValue : 0)
    property var customSeries: createEmptyInstabilitySeries(qsTr("\u81ea\u5b9a\u4e49"), detailPage ? detailPage.minHeightValue : 0, detailPage ? detailPage.maxHeightValue : 0)
    property var radarPolygons: []
    property real radarMaxValue: 1
    property real customLowerBound: 0
    property real customUpperBound: 0
    property bool overallLoaded: false
    property bool localLoaded: false
    property bool customLoaded: false
    property bool overviewLoading: false
    property bool customLoading: false
    property int overviewRequestId: 0
    property int customRequestId: 0

    color: "#FFFFFF"

    function createEmptyInstabilitySeries(title, lowerBound, upperBound) {
        // 鎵€鏈夋ā寮忓厛浣跨敤鍚屼竴绉嶇┖缁撴瀯锛岄伩鍏嶇晫闈㈠垵娆¤繘鍏ユ椂鍙嶅鍒ょ┖銆?
        return {
            title: title,
            rangeLabel: detailPage ? detailPage.formatNumber(lowerBound, 1) + " - " + detailPage.formatNumber(upperBound, 1) + " mm" : "",
            points: [],
            chartMinX: 0,
            chartMaxX: 1,
            chartMinY: 0,
            chartMaxY: 1,
            xAxisTickValues: [0, 1],
            yAxisLabels: detailPage ? detailPage.makeAxisLabels(0, 1, 6, 1) : [0, 1]
        }
    }


    function resetInstabilityState() {
        overallLoaded = false
        localLoaded = false
        customLoaded = false
        overviewLoading = false
        customLoading = false
        overviewRequestId = 0
        customRequestId = 0
        overallSeries = createEmptyInstabilitySeries(qsTr("\u6574\u4f53"), detailPage ? detailPage.minHeightValue : 0, detailPage ? detailPage.maxHeightValue : 0)
        bottomSeries = createEmptyInstabilitySeries(qsTr("\u5e95\u90e8"), detailPage ? detailPage.minHeightValue : 0, detailPage ? detailPage.minHeightValue : 0)
        middleSeries = createEmptyInstabilitySeries(qsTr("\u4e2d\u90e8"), detailPage ? detailPage.minHeightValue : 0, detailPage ? detailPage.minHeightValue : 0)
        topSeries = createEmptyInstabilitySeries(qsTr("\u9876\u90e8"), detailPage ? detailPage.maxHeightValue : 0, detailPage ? detailPage.maxHeightValue : 0)
        customSeries = createEmptyInstabilitySeries(qsTr("\u81ea\u5b9a\u4e49"), detailPage ? detailPage.minHeightValue : 0, detailPage ? detailPage.maxHeightValue : 0)
        radarPolygons = []
        radarMaxValue = 1
    }

    function cancelDetailRequests(reason) {
        if (!detail_ctrl)
            return

        if (overviewRequestId > 0)
            detail_ctrl.cancelInstabilityRequest(overviewRequestId)
        if (customRequestId > 0)
            detail_ctrl.cancelInstabilityRequest(customRequestId)

        if (overviewRequestId > 0 || customRequestId > 0) {
            console.log("[DetailInstability][release]",
                        "experimentId=", experimentData && experimentData.id !== undefined ? Number(experimentData.id) : 0,
                        "reason=", reason,
                        "overviewRequestId=", overviewRequestId,
                        "customRequestId=", customRequestId)
        }

        overviewRequestId = 0
        customRequestId = 0
        overviewLoading = false
        customLoading = false
    }

    function applyOverviewPayload(payload) {
        overallSeries = payload && payload.overallSeries ? payload.overallSeries : createEmptyInstabilitySeries(qsTr("\u6574\u4f53"), detailPage.minHeightValue, detailPage.maxHeightValue)
        bottomSeries = payload && payload.bottomSeries ? payload.bottomSeries : createEmptyInstabilitySeries(qsTr("\u5e95\u90e8"), detailPage.minHeightValue, detailPage.minHeightValue)
        middleSeries = payload && payload.middleSeries ? payload.middleSeries : createEmptyInstabilitySeries(qsTr("\u4e2d\u90e8"), detailPage.minHeightValue, detailPage.minHeightValue)
        topSeries = payload && payload.topSeries ? payload.topSeries : createEmptyInstabilitySeries(qsTr("\u9876\u90e8"), detailPage.maxHeightValue, detailPage.maxHeightValue)

        var radarData = payload && payload.radarData ? payload.radarData : ({})
        radarPolygons = radarData.polygons || []
        radarMaxValue = detailPage ? detailPage.toNumber(radarData.maxValue, 1) : 1

        overviewLoading = false
        overallLoaded = true
        localLoaded = true

        console.log("[DetailInstability][overview ready]",
                    "experimentId=", Number(experimentData.id),
                    "overallPoints=", overallSeries && overallSeries.points ? overallSeries.points.length : 0,
                    "bottomPoints=", bottomSeries && bottomSeries.points ? bottomSeries.points.length : 0,
                    "middlePoints=", middleSeries && middleSeries.points ? middleSeries.points.length : 0,
                    "topPoints=", topSeries && topSeries.points ? topSeries.points.length : 0,
                    "radarPolygons=", radarPolygons.length)
    }

    function applyCustomPayload(payload) {
        customSeries = payload && payload.series ? payload.series : createEmptyInstabilitySeries(qsTr("\u81ea\u5b9a\u4e49"), customLowerBound, customUpperBound)
        customLoading = false
        customLoaded = true

        console.log("[DetailInstability][custom ready]",
                    "experimentId=", Number(experimentData.id),
                    "lowerMm=", customLowerBound,
                    "upperMm=", customUpperBound,
                    "pointCount=", customSeries && customSeries.points ? customSeries.points.length : 0)
    }

    function requestOverviewData() {
        if (!detailPage || !experimentData || experimentData.id === undefined || !detail_ctrl)
            return

        overviewLoading = true
        overviewRequestId = detail_ctrl.requestInstabilityOverview(Number(experimentData.id),
                                                                  detailPage.minHeightValue,
                                                                  detailPage.maxHeightValue)
        console.log("[DetailInstability][overview request]",
                    "experimentId=", Number(experimentData.id),
                    "requestId=", overviewRequestId,
                    "minHeightMm=", detailPage.minHeightValue,
                    "maxHeightMm=", detailPage.maxHeightValue)
        if (overviewRequestId <= 0)
            overviewLoading = false
    }

    function requestCustomData() {
        if (!detailPage || !experimentData || experimentData.id === undefined || !detail_ctrl)
            return

        customLowerBound = Math.max(detailPage.minHeightValue, Math.min(customLowerBound, detailPage.maxHeightValue))
        customUpperBound = Math.max(detailPage.minHeightValue, Math.min(customUpperBound, detailPage.maxHeightValue))

        customLoading = true
        customLoaded = false
        customRequestId = detail_ctrl.requestInstabilityCustomSeries(Number(experimentData.id),
                                                                     customLowerBound,
                                                                     customUpperBound)
        console.log("[DetailInstability][custom request]",
                    "experimentId=", Number(experimentData.id),
                    "requestId=", customRequestId,
                    "lowerMm=", customLowerBound,
                    "upperMm=", customUpperBound)
        if (customRequestId <= 0)
            customLoading = false
    }

    function loadOverallData() {
        if (!detailPage || !experimentData || experimentData.id === undefined || !detail_ctrl)
            return

        if (!overallLoaded && !overviewLoading)
            requestOverviewData()
    }

    function loadLocalData() {
        // 灞€閮ㄦā寮忓浐瀹氭寜搴?涓?椤朵笁娈靛垏鍒嗭紝缁撴灉浼氳鍚庣鎸夊尯闂寸紦瀛樸€?
        if (!detailPage || !experimentData || experimentData.id === undefined || localLoaded)
            return

        if (!overviewLoading && !overallLoaded)
            requestOverviewData()
    }
    function loadCustomData() {
        // 鑷畾涔夋ā寮忓彧鏈夌偣鍑烩€滃簲鐢ㄢ€濆悗鎵嶉噸鏂板彇鏁帮紝閬垮厤杈撳叆妗嗙紪杈戞椂棰戠箒瑙﹀彂璁＄畻銆?
        if (!detailPage || !experimentData || experimentData.id === undefined || !detail_ctrl)
            return

        requestCustomData()
    }

    function activeSeriesList() {
        if (currentModeIndex === 0)
            return [overallSeries]
        if (currentModeIndex === 1)
            return [topSeries, middleSeries, bottomSeries]
        if (currentModeIndex === 2)
            return [customSeries]
        return [overallSeries, topSeries, middleSeries, bottomSeries]
    }

    function hasVisibleSeries() {
        var seriesList = activeSeriesList()
        for (var i = 0; i < seriesList.length; ++i) {
            if (seriesList[i].points && seriesList[i].points.length > 0)
                return true
        }
        return false
    }

    function buildRadarOverview() {
        // 闆疯揪鍥剧殑姣忎竴灞傚搴斿悓涓€涓椂闂寸偣鍦ㄥ洓涓尯闂翠笂鐨?Ius 鍊硷紝
        // 鍥犳杩欓噷鎸夋椂闂寸储寮曟妸鏁翠綋/搴曢儴/涓儴/椤堕儴鎷兼垚涓€缁?polygon銆?
        radarPolygons = []
        radarMaxValue = 1
        return
    }

    function applyCustomRange() {
        loadCustomData()
    }

    function ensureModeData() {
        // 棣栨杩涘叆椤甸潰鍙姞杞芥暣浣擄紱
        // 鍏朵粬妯″紡鍦ㄧ湡姝ｅ垏鎹㈣繃鍘绘椂鍐嶈ˉ鏁版嵁锛岄伩鍏嶈繘椤靛崱椤裤€?
        if (!overallLoaded)
            loadOverallData()

        if (currentModeIndex === 1 || currentModeIndex === 3)
            loadLocalData()
        else if (currentModeIndex === 2 && !customLoaded && !customLoading)
            loadCustomData()

        if (currentModeIndex === 3 && radarPolygons.length === 0)
            radarPolygons = []
    }

    function loadInstabilityData() {
        if (!detailPage)
            return

        cancelDetailRequests("reload")
        resetInstabilityState()
        customLowerBound = detailPage.floorToStep(detailPage.minHeightValue, 1)
        customUpperBound = detailPage.ceilToStep(detailPage.maxHeightValue, 1)

        requestOverviewData()
    }

    function normalizedTextToNumber(textValue, fallback) {
        var parsed = Number(textValue)
        return isNaN(parsed) ? fallback : parsed
    }

    onDetailPageChanged: loadInstabilityData()
    onCurrentModeIndexChanged: ensureModeData()
    Component.onCompleted: {
        if (detailPage)
            loadInstabilityData()
    }
    Component.onDestruction: cancelDetailRequests("destroyed")

    Connections {
        target: detailPage
        onExperimentDataChanged: instabilityPanel.loadInstabilityData()
    }

    Connections {
        target: detail_ctrl
        ignoreUnknownSignals: true
        onInstabilityOverviewRequestFinished: {
            console.log("[DetailInstability][overview finished signal]",
                        "requestId=", requestId,
                        "activeRequestId=", instabilityPanel.overviewRequestId,
                        "experimentId=", experimentId,
                        "activeExperimentId=", instabilityPanel.experimentData && instabilityPanel.experimentData.id !== undefined ? Number(instabilityPanel.experimentData.id) : 0)
            if (requestId !== instabilityPanel.overviewRequestId)
                return
            if (!instabilityPanel.experimentData || Number(instabilityPanel.experimentData.id) !== Number(experimentId))
                return

            instabilityPanel.overviewRequestId = 0
            instabilityPanel.applyOverviewPayload(payload)
        }
        onInstabilityOverviewRequestFailed: {
            if (requestId !== instabilityPanel.overviewRequestId)
                return
            if (!instabilityPanel.experimentData || Number(instabilityPanel.experimentData.id) !== Number(experimentId))
                return

            instabilityPanel.overviewRequestId = 0
            instabilityPanel.overviewLoading = false
            console.log("[DetailInstability][overview failed]",
                        "experimentId=", experimentId,
                        "message=", message)
        }
        onInstabilityOverviewRequestCancelled: {
            if (requestId !== instabilityPanel.overviewRequestId)
                return

            instabilityPanel.overviewRequestId = 0
            instabilityPanel.overviewLoading = false
            console.log("[DetailInstability][overview cancelled]",
                        "experimentId=", experimentId,
                        "reason=", reason)
        }
        onInstabilityCustomSeriesRequestFinished: {
            console.log("[DetailInstability][custom finished signal]",
                        "requestId=", requestId,
                        "activeRequestId=", instabilityPanel.customRequestId,
                        "experimentId=", experimentId,
                        "activeExperimentId=", instabilityPanel.experimentData && instabilityPanel.experimentData.id !== undefined ? Number(instabilityPanel.experimentData.id) : 0)
            if (requestId !== instabilityPanel.customRequestId)
                return
            if (!instabilityPanel.experimentData || Number(instabilityPanel.experimentData.id) !== Number(experimentId))
                return

            instabilityPanel.customRequestId = 0
            instabilityPanel.applyCustomPayload(payload)
        }
        onInstabilityCustomSeriesRequestFailed: {
            if (requestId !== instabilityPanel.customRequestId)
                return
            if (!instabilityPanel.experimentData || Number(instabilityPanel.experimentData.id) !== Number(experimentId))
                return

            instabilityPanel.customRequestId = 0
            instabilityPanel.customLoading = false
            console.log("[DetailInstability][custom failed]",
                        "experimentId=", experimentId,
                        "message=", message)
        }
        onInstabilityCustomSeriesRequestCancelled: {
            if (requestId !== instabilityPanel.customRequestId)
                return

            instabilityPanel.customRequestId = 0
            instabilityPanel.customLoading = false
            console.log("[DetailInstability][custom cancelled]",
                        "experimentId=", experimentId,
                        "reason=", reason)
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 18
        spacing: 16

        Row {
            spacing: 8

            Repeater {
                model: instabilityPanel.modeTitles

                delegate: Button {
                    id: instabilityModeButton
                    width: 88
                    height: 28
                    text: modelData
                    onClicked: instabilityPanel.currentModeIndex = index

                    contentItem: Text {
                        text: instabilityModeButton.text
                        font.pixelSize: 12
                        font.family: "Microsoft YaHei"
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        color: instabilityPanel.currentModeIndex === index ? "#FFFFFF" : "#4A89DC"
                    }

                    background: Rectangle {
                        color: instabilityPanel.currentModeIndex === index ? "#4A89DC" : "#FFFFFF"
                        border.color: "#4A89DC"
                        border.width: 1
                    }
                }
            }
        }

        Rectangle {
            id: customHeightArea
            visible: instabilityPanel.currentModeIndex === 2
            Layout.fillWidth: true
            Layout.preferredHeight: 48
            radius: 4
            color: "#FFFFFF"
            border.color: "#D8E4F0"
            border.width: 1

            Row {
                anchors.fill: parent
                anchors.leftMargin: 12
                anchors.rightMargin: 12
                spacing: 8

                Label {
                    anchors.verticalCenter: parent.verticalCenter
                    text: qsTr("楂樺害涓嬮檺:")
                    font.pixelSize: 12
                    font.family: "Microsoft YaHei"
                    color: "#2F3A4A"
                }

                TextField {
                    id: instabilityLowerField
                    anchors.verticalCenter: parent.verticalCenter
                    width: 56
                    height: 28
                    text: detailPage ? detailPage.formatNumber(instabilityPanel.customLowerBound, 0) : "0"
                    horizontalAlignment: Text.AlignHCenter
                    font.pixelSize: 12
                    font.family: "Microsoft YaHei"
                    validator: IntValidator {
                        bottom: detailPage ? Math.floor(detailPage.minHeightValue) : 0
                        top: detailPage ? Math.ceil(detailPage.maxHeightValue) : 100
                    }
                }

                Label {
                    anchors.verticalCenter: parent.verticalCenter
                    text: qsTr("楂樺害涓婇檺:")
                    font.pixelSize: 12
                    font.family: "Microsoft YaHei"
                    color: "#2F3A4A"
                }

                TextField {
                    id: instabilityUpperField
                    anchors.verticalCenter: parent.verticalCenter
                    width: 56
                    height: 28
                    text: detailPage ? detailPage.formatNumber(instabilityPanel.customUpperBound, 0) : "0"
                    horizontalAlignment: Text.AlignHCenter
                    font.pixelSize: 12
                    font.family: "Microsoft YaHei"
                    validator: IntValidator {
                        bottom: detailPage ? Math.floor(detailPage.minHeightValue) : 0
                        top: detailPage ? Math.ceil(detailPage.maxHeightValue) : 100
                    }
                }

                Label {
                    anchors.verticalCenter: parent.verticalCenter
                    text: "mm"
                    font.pixelSize: 12
                    font.family: "Microsoft YaHei"
                    color: "#2F3A4A"
                }

                IconButton {
                    anchors.verticalCenter: parent.verticalCenter
                    width: 80
                    height: 28
                    button_text: qsTr("搴旂敤")
                    button_color: "#4A89DC"
                    text_color: "#FFFFFF"
                    pixelSize: 12
                    family: "Microsoft YaHei"
                    button_radius: 4
                    onClicked: {
                        instabilityPanel.customLowerBound = instabilityPanel.normalizedTextToNumber(instabilityLowerField.text, instabilityPanel.customLowerBound)
                        instabilityPanel.customUpperBound = instabilityPanel.normalizedTextToNumber(instabilityUpperField.text, instabilityPanel.customUpperBound)
                        if (instabilityPanel.customUpperBound < instabilityPanel.customLowerBound) {
                            var tempHeight = instabilityPanel.customLowerBound
                            instabilityPanel.customLowerBound = instabilityPanel.customUpperBound
                            instabilityPanel.customUpperBound = tempHeight
                        }
                        instabilityPanel.applyCustomRange()
                        instabilityLowerField.text = detailPage.formatNumber(instabilityPanel.customLowerBound, 0)
                        instabilityUpperField.text = detailPage.formatNumber(instabilityPanel.customUpperBound, 0)
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
                visible: !instabilityPanel.hasVisibleSeries()
                text: qsTr("\u6570\u636e\u5e93\u4e2d\u6682\u65e0\u8be5\u5b9e\u9a8c\u7684\u4e0d\u7a33\u5b9a\u6027\u66f2\u7ebf\u6570\u636e")
                font.family: "Microsoft YaHei"
                color: "#7A8CA5"
            }

            Item {
                anchors.fill: parent
                anchors.margins: 12
                visible: instabilityPanel.hasVisibleSeries()

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 12
                    visible: instabilityPanel.currentModeIndex !== 3

                    Repeater {
                        model: instabilityPanel.activeSeriesList()

                        delegate: Rectangle {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            Layout.preferredHeight: instabilityPanel.currentModeIndex === 1 ? 150 : 220
                            radius: 6
                            color: "#FFFFFF"
                            border.color: "#DCE6F2"
                            border.width: 1

                            Text {
                                anchors.left: parent.left
                                anchors.top: parent.top
                                anchors.leftMargin: 16
                                anchors.topMargin: 10
                                text: modelData.title + "  " + modelData.rangeLabel
                                font.pixelSize: 12
                                font.family: "Microsoft YaHei"
                                color: "#4A5D75"
                                font.bold: true
                            }

                            TrendChart {
                                anchors.fill: parent
                                anchors.margins: 10
                                anchors.topMargin: 34
                                leftMargin: 72
                                rightMargin: 22
                                topMargin: 18
                                yAxisTitleOffset: 64
                                dataPoints: modelData.points
                                lineColor: modelData.title === qsTr("搴曢儴") ? "#2F7CF6"
                                          : modelData.title === qsTr("涓儴") ? "#21A366"
                                          : modelData.title === qsTr("椤堕儴") ? "#F28C28"
                                          : "#2F7CF6"
                                minXValue: modelData.chartMinX
                                maxXValue: modelData.chartMaxX
                                minYValue: modelData.chartMinY
                                maxYValue: modelData.chartMaxY
                                xAxisTickValues: modelData.xAxisTickValues
                                yAxisLabels: modelData.yAxisLabels
                                yAxisTitle: "Ius"
                                xAxisTitle: qsTr("鏃堕棿(min)")
                                formatXLabel: function(value) {
                                    return detailPage ? detailPage.formatNumber(value, 1) : Number(value).toFixed(1)
                                }
                            }
                        }
                    }
                }

                Item {
                    anchors.fill: parent
                    visible: instabilityPanel.currentModeIndex === 3

                    InstabilityRadarChart {
                        // 鎬昏妯″紡浣跨敤闆疯揪鍥惧睍绀哄悓涓€鏃堕棿鐐瑰湪鍥涗釜楂樺害鍖洪棿鐨勬í鍚戝姣斻€?
                        anchors.fill: parent
                        polygons: instabilityPanel.radarPolygons
                        maxValue: instabilityPanel.radarMaxValue
                        tickCount: 6
                        axisLabels: [qsTr("Ius - 鏁翠綋"), qsTr("Ius - 搴曢儴"), qsTr("Ius - 涓儴"), qsTr("Ius - 椤堕儴")]
                    }
                }
            }

            Item {
                anchors.fill: parent
                visible: instabilityPanel.overviewLoading || instabilityPanel.customLoading
                z: 10

                Rectangle {
                    anchors.fill: parent
                    color: instabilityPanel.hasVisibleSeries() ? "#66FFFFFF" : "#F8FBFF"
                }

                Column {
                    anchors.centerIn: parent
                    spacing: 8

                    BusyIndicator {
                        anchors.horizontalCenter: parent.horizontalCenter
                        running: parent.parent.visible
                        width: 36
                        height: 36
                    }

                    Text {
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: instabilityPanel.customLoading
                              ? qsTr("\u6b63\u5728\u52a0\u8f7d\u81ea\u5b9a\u4e49\u4e0d\u7a33\u5b9a\u6027\u6570\u636e")
                              : qsTr("\u6b63\u5728\u52a0\u8f7d\u4e0d\u7a33\u5b9a\u6027\u66f2\u7ebf")
                        font.pixelSize: 12
                        font.family: "Microsoft YaHei"
                        color: "#4A5D75"
                    }
                }
            }
        }
    }
}
