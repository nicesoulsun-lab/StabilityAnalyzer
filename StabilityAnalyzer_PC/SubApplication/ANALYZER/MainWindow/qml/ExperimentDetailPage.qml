import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12

Item {
    id: detailPage
    objectName: "ExperimentDetailPage"

    property var experimentData: ({})
    property int currentTabIndex: 0

    signal backRequested()

    // 统一管理详情页上方 bar 的 3 个切换项，后续接真实图表时只需要补对应子页。
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

    // 用 Loader 承载下方内容区，当前先提供占位框架，默认显示光强曲线页。
    function currentTabComponent() {
        if (currentTabIndex === 1)
            return instabilityCurveComponent
        if (currentTabIndex === 2)
            return uniformityIndexComponent
        return lightIntensitySwitchComponent
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

                // 只给顶部状态栏铺背景图，避免被下方内容区覆盖。
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

                    Item {
                        Layout.fillWidth: true
                    }

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
        id: lightIntensitySwitchComponent

        // 光强曲线的 3 个按钮共用同一张图表容器，只切换数据类型或曲线数量。
        Rectangle {
            id: lightIntensityPanel
            color: "#FFFFFF"
            property int currentLightModeIndex: 0
            property var lightModeTitles: [qsTr("背射光"), qsTr("透射光"), qsTr("背射光+透射光")]

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

                // 这里先放一个共用图表框架，后续只需要按按钮切换数据源和曲线数量。
                Rectangle {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    color: "#F8FBFF"
                    border.color: "#D8E4F0"
                    border.width: 1

                    Column {
                        anchors.centerIn: parent
                        spacing: 10

                        Text {
                            anchors.horizontalCenter: parent.horizontalCenter
                            text: lightIntensityPanel.lightModeTitles[lightIntensityPanel.currentLightModeIndex]
                            font.pixelSize: 18
                            font.family: "Microsoft YaHei"
                            color: "#4A5D75"
                        }

                        Text {
                            anchors.horizontalCenter: parent.horizontalCenter
                            text: qsTr("共用图表框架，后续只切换数据或曲线数量")
                            font.pixelSize: 14
                            font.family: "Microsoft YaHei"
                            color: "#7A8CA5"
                        }
                    }
                }
            }
        }
    }

    Component {
        id: instabilityCurveComponent

        // 不稳定性曲线页先保留占位区域，后续按同一 Loader 切换机制继续扩展。
        Rectangle {
            id: instabilityPanel
            color: "#FFFFFF"
            property int currentInstabilityModeIndex: 0
            property var instabilityModeTitles: [qsTr("整体"), qsTr("局部"), qsTr("自定义"), qsTr("总览")]

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
                        model: instabilityPanel.instabilityModeTitles

                        delegate: Button {
                            id: instabilityModeButton
                            width: 116
                            height: 28
                            text: modelData
                            onClicked: instabilityPanel.currentInstabilityModeIndex = index

                            contentItem: Text {
                                text: instabilityModeButton.text
                                font.pixelSize: 12
                                font.family: "Microsoft YaHei"
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                                color: instabilityPanel.currentInstabilityModeIndex === index ? "#FFFFFF" : "#4A89DC"
                            }

                            background: Rectangle {
                                color: instabilityPanel.currentInstabilityModeIndex === index ? "#4A89DC" : "#FFFFFF"
                                border.color: "#4A89DC"
                                border.width: 1
                            }
                        }
                    }
                }

                // 这里先放一个共用图表框架，后续只需要按按钮切换数据源和曲线数量。
                Rectangle {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    color: "#F8FBFF"
                    border.color: "#D8E4F0"
                    border.width: 1

                    Column {
                        anchors.centerIn: parent
                        spacing: 10

                        Text {
                            anchors.horizontalCenter: parent.horizontalCenter
                            text: instabilityPanel.instabilityModeTitles[instabilityPanel.currentInstabilityModeIndex]
                            font.pixelSize: 18
                            font.family: "Microsoft YaHei"
                            color: "#4A5D75"
                        }

                        Text {
                            anchors.horizontalCenter: parent.horizontalCenter
                            text: qsTr("共用图表框架，后续只切换数据或曲线数量")
                            font.pixelSize: 14
                            font.family: "Microsoft YaHei"
                            color: "#7A8CA5"
                        }
                    }
                }
            }
        }
    }

    Component {
        id: uniformityIndexComponent

        // 均匀度指数页同样只生成框架，保证后续可以独立接入页面内容。
        Rectangle {
            color: "#FFFFFF"

            Rectangle {
                anchors.fill: parent
                anchors.margins: 18
                color: "#F8FBFF"
                border.color: "#D8E4F0"
                border.width: 1

                Text {
                    anchors.centerIn: parent
                    text: qsTr("均匀度指数页面框架")
                    font.pixelSize: 16
                    font.family: "Microsoft YaHei"
                    color: "#7A8CA5"
                }
            }
        }
    }
}
