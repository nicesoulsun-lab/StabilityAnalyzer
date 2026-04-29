import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import "component"

Item {
    id: homepage
    objectName: "HomePage"

    property string contentPageUrl: ""
    property string contentPageKey: ""
    property string currentPageKey: ""

    function channelStatus(index) {
        if (!data_transmit_ctrl || !data_transmit_ctrl.experimentChannels || index < 0 || index >= data_transmit_ctrl.experimentChannels.length) {
            return ({ running: false, hasSample: false, isCovered: false, remainingSeconds: 0 })
        }
        return data_transmit_ctrl.experimentChannels[index]
    }

    function formatRemainingTime(seconds) {
        if (seconds === undefined || seconds === null || seconds <= 0) {
            return qsTr("剩余时间：00:00:00")
        }

        var total = Math.floor(seconds)
        var hours = Math.floor(total / 3600)
        var minutes = Math.floor((total % 3600) / 60)
        var remainSeconds = total % 60

        function pad(value) {
            return value < 10 ? "0" + value : "" + value
        }

        return qsTr("剩余时间：") + pad(hours) + ":" + pad(minutes) + ":" + pad(remainSeconds)
    }

    function openPage(pageUrl, pageKey) {
        if (pageKey !== undefined && pageKey !== "") {
            currentPageKey = pageKey
        }
        if (mainLoader.source !== pageUrl) {
            mainLoader.source = pageUrl
        } else if (mainLoader.item && mainLoader.item.forceActiveFocus) {
            mainLoader.item.forceActiveFocus()
        }
    }

    function openContentPage(pageUrl, pageKey) {
        contentPageUrl = pageUrl
        contentPageKey = pageKey || ""
        openPage(pageUrl, contentPageKey)
    }

    function openRealtimePage(channelIndex) {
        var statusData = channelStatus(channelIndex)
        if (!statusData || !statusData.running) {
            return
        }

        var experimentId = 0
        if (experiment_ctrl && experiment_ctrl.getCurrentExperimentId) {
            experimentId = Number(experiment_ctrl.getCurrentExperimentId(channelIndex))
        }
        if (experimentId <= 0 && statusData.experiment_id !== undefined) {
            experimentId = Number(statusData.experiment_id)
        }
        if (experimentId <= 0 && statusData.experimentId !== undefined) {
            experimentId = Number(statusData.experimentId)
        }
        if (experimentId <= 0) {
            info_pop.openDialog(qsTr("当前通道暂无可查看的实时实验数据"))
            return
        }

        var experimentData = ({ id: experimentId })

        console.log("[HomePage][open realtime]",
                    "channelIndex=", channelIndex,
                    "experimentId=", experimentId,
                    "hasExperimentRange=", experimentData && experimentData.scan_range_start !== undefined)

        currentPageKey = "realtime"
        mainLoader.setSource("qrc:/qml/RealtimeExperimentPage.qml", {
                                 "channelIndex": channelIndex,
                                 "experimentData": experimentData
                             })
    }

    function openInstructionPage() {
        openPage("qrc:/qml/ManualViewerPage.qml", "help")
    }

    RowLayout {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            id: sidePanel
            Layout.preferredWidth: 122
            Layout.minimumWidth: 122
            Layout.fillHeight: true
            color: "#F5F5F5"

            gradient: Gradient {
                GradientStop { position: 0.0; color: "#BBDEFB" }
                GradientStop { position: 1.0; color: "#E3F2FD" }
            }

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 12
                spacing: 6

                ChannelPanel {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 150
                    property var statusData: homepage.channelStatus(0)
                    title: qsTr("A通道")
                    isRunning: !!statusData.running
                    hasSample: !!statusData.hasSample
                    isCovered: !!statusData.isCovered
                    //remainingTimeText: homepage.formatRemainingTime(statusData.remainingSeconds || 0)
                    onClicked: homepage.openRealtimePage(0)
                }

                ChannelPanel {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 150
                    property var statusData: homepage.channelStatus(1)
                    title: qsTr("B通道")
                    isRunning: !!statusData.running
                    hasSample: !!statusData.hasSample
                    isCovered: !!statusData.isCovered
                    //remainingTimeText: homepage.formatRemainingTime(statusData.remainingSeconds || 0)
                    onClicked: homepage.openRealtimePage(1)
                }

                ChannelPanel {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 150
                    property var statusData: homepage.channelStatus(2)
                    title: qsTr("C通道")
                    isRunning: !!statusData.running
                    hasSample: !!statusData.hasSample
                    isCovered: !!statusData.isCovered
                    //remainingTimeText: homepage.formatRemainingTime(statusData.remainingSeconds || 0)
                    onClicked: homepage.openRealtimePage(2)
                }

                ChannelPanel {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 150
                    property var statusData: homepage.channelStatus(3)
                    title: qsTr("D通道")
                    isRunning: !!statusData.running
                    hasSample: !!statusData.hasSample
                    isCovered: !!statusData.isCovered
                    //remainingTimeText: homepage.formatRemainingTime(statusData.remainingSeconds || 0)
                    onClicked: homepage.openRealtimePage(3)
                }

                Item { Layout.fillHeight: true }
            }
        }

        Rectangle {
            id: mainContent
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.margins: 2
            Layout.leftMargin: 0
            color: "#FFFFFF"
            radius: 4
            border.color: "#E0E8F0"
            border.width: 0

            ColumnLayout {
                anchors.fill: parent
                spacing: 0

                OptionsBar {
                    id: optionsBar
                    Layout.fillWidth: true
                    Layout.preferredHeight: 40
                    activePage: homepage.currentPageKey
                    deviceAvailable: data_transmit_ctrl
                                     && data_transmit_ctrl.deviceUiConnectionStateText === "Connected"
                    onImportRecordRequested: importRecordPop.open()
                    onInstrumentCheckRequested: instrumentCheckPop.open()
                    onNewExperimentRequested: newExperimentPop.open()
                    onExperimentRecordRequested: homepage.openContentPage("qrc:/qml/ExperimentRecordPage.qml", "record")
                    onUserManagementRequested: homepage.openContentPage("qrc:/qml/UserManagementPage.qml", "user")
                    onInstructionRequested: homepage.openInstructionPage()
                    onRecycleBinRequested: homepage.openContentPage("qrc:/qml/RecycleBinPage.qml", "recycle")
                }

                Loader {
                    id: mainLoader
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                }

                Connections {
                    target: mainLoader.item
                    ignoreUnknownSignals: true
                    onBackRequested: homepage.openContentPage(homepage.contentPageUrl, homepage.contentPageKey)
                }
            }
        }
    }

    InstrumentCheckPop {
        id: instrumentCheckPop
    }

    ImportRecordPop {
        id: importRecordPop
        onImportStarted: importProgressPop.open()
        onImportFinished: {
            importProgressPop.visible = false
            importProgressPop.close()
        }
    }

    ImportProgressPop {
        id: importProgressPop
    }

    NewExperimentPop {
        id: newExperimentPop
    }

    CustomPop {
        id: custom_pop
    }
}
