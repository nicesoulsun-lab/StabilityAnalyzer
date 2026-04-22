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
                    title: qsTr("A通道")
                    isRunning: true
                    hasSample: true
                    isCovered: true
                }

                ChannelPanel {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 150
                    title: qsTr("B通道")
                    isRunning: true
                    hasSample: true
                    isCovered: false
                }

                ChannelPanel {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 150
                    title: qsTr("C通道")
                    isRunning: false
                    hasSample: false
                    isCovered: true
                }

                ChannelPanel {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 150
                    title: qsTr("D通道")
                    isRunning: false
                    hasSample: false
                    isCovered: false
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
        onImportFinished: importProgressPop.close()
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
