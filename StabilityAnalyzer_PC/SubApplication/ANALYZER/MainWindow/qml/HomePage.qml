import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import "component"

Item {
    id: homepage
    objectName: "HomePage"

    property string contentPageUrl: "qrc:/qml/data_page.qml"

    function openPage(pageUrl) {
        if (mainLoader.source !== pageUrl) {
            mainLoader.source = pageUrl
        } else if (mainLoader.item && mainLoader.item.forceActiveFocus) {
            mainLoader.item.forceActiveFocus()
        }
    }

    function openContentPage(pageUrl) {
        contentPageUrl = pageUrl
        openPage(pageUrl)
    }

    function openInstructionPage() {
        openPage("qrc:/qml/ManualViewerPage.qml")
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
                    onInstrumentCheckRequested: instrumentCheckPop.open()
                    onNewExperimentRequested: newExperimentPop.open()
                    onUserManagementRequested: homepage.openContentPage("qrc:/qml/UserManagementPage.qml")
                    onInstructionRequested: homepage.openInstructionPage()
                }

                Loader {
                    id: mainLoader
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                }

                Connections {
                    target: mainLoader.item
                    ignoreUnknownSignals: true
                    onBackRequested: homepage.openContentPage(homepage.contentPageUrl)
                }
            }
        }
    }

    InstrumentCheckPop {
        id: instrumentCheckPop
    }

    NewExperimentPop {
        id: newExperimentPop
    }

    Component.onCompleted: {
        openContentPage("qrc:/qml/data_page.qml")
    }
}
