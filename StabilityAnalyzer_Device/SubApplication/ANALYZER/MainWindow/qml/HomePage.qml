import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import "component"

Item {
    id: homepage
    width: 972
    height: 501

    objectName: "HomePage"

    function channelCard(index) {
        return channelCardRepeater.itemAt(index)
    }

    function channelTitle(index) {
        if (experiment_ctrl && experiment_ctrl.channelDisplayName)
            return experiment_ctrl.channelDisplayName(index)
        return "Channel " + (index + 1)
    }

    // 将C++侧通道状态映射到首页单卡片UI。
    // channel: 0/1/2/3 对应 A/B/C/D 通道。
    // status: 来自 ExperimentCtrl::channelStatusUpdated 的 QVariantMap。
    function applyChannelStatus(channel, status) {
        // 先定位目标通道卡片。
        var card = channelCard(channel)
        if (card === null || status === undefined || status === null) return

        card.isRunning = Boolean(status.running)
        card.hasSample = Boolean(status.hasSample)
        card.isCovered = Boolean(status.isCovered)

        // 温度转为一位小数展示（单位：°C）。
        var t = Number(status.currentTemperature)
        if (isNaN(t)) t = 0
        card.temperature = t.toFixed(1)

        // 剩余时间由秒转小时并保留一位小数，便于首页快速查看。
        var remainingSeconds = Number(status.remainingSeconds)
        if (isNaN(remainingSeconds) || remainingSeconds < 0) remainingSeconds = 0

        if(remainingSeconds > 0 && remainingSeconds <= 36) {
            card.remainingHours = "0.01";
        }else{
            card.remainingHours = (remainingSeconds / 3600.0).toFixed(2)
        }
    }

    // 页面初始化时主动拉取一次通道快照，避免等待首个轮询信号。
    function refreshAllChannelStatus() {
        var count = experiment_ctrl ? experiment_ctrl.channelCount : 4
        for (var i = 0; i < count; ++i) {
            applyChannelStatus(i, experiment_ctrl.getChannelStatus(i))
        }
    }

    Component.onCompleted: {
        refreshAllChannelStatus()
    }

    // 持续监听C++推送状态，实现首页实时刷新。
    Connections {
        target: experiment_ctrl
        onChannelStatusUpdated: {
            applyChannelStatus(channel, status)
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 34

        Item {
            id: channelCardContainer
            Layout.fillWidth: true
            Layout.preferredHeight: 330

            Row {
                id: channelCardRow
                anchors.verticalCenter: parent.verticalCenter
                anchors.left: (experiment_ctrl && experiment_ctrl.channelCount === 1)
                              ? undefined
                              : parent.left
                anchors.horizontalCenter: (experiment_ctrl && experiment_ctrl.channelCount === 1)
                                          ? parent.horizontalCenter
                                          : undefined
                spacing: 37

                Repeater {
                    id: channelCardRepeater
                    model: experiment_ctrl ? experiment_ctrl.channelCount : 4

                    delegate: HP_comp {
                        width: 212
                        height: channelCardContainer.height
                        title: homepage.channelTitle(index)
                        isRunning: false
                        hasSample: false
                        isCovered: true
                    }
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: 90
            spacing: 28

            IconCardButton {
                btnWidth: 170
                btnHeight: 90
                iconSource: "qrc:/icon/qml/icon/build_project_btn.png"
                onClicked: {
                    user_ctrl.logOperation("进入创建工程页面")
                    new_project_pop.open()
                }
            }

            IconCardButton {
                btnWidth: 170
                btnHeight: 90
                iconSource: "qrc:/icon/qml/icon/start_test_btn.png"
                onClicked: {
                    user_ctrl.logOperation("进入开始实验页面")
                    select_channel_pop.open()
                }
            }

            IconCardButton {
                btnWidth: 170
                btnHeight: 90
                iconSource: "qrc:/icon/qml/icon/sys_set_btn.png"
                onClicked: {
                    user_ctrl.logOperation("进入系统设置页面")
                    mainStackView.push("qrc:/qml/system_page.qml")
                }
            }

            IconCardButton {
                btnWidth: 170
                btnHeight: 90
                iconSource: "qrc:/icon/qml/icon/data_mng.png"
                onClicked: {
                    user_ctrl.logOperation("进入数据管理页面")
                    mainStackView.push(Qt.resolvedUrl("qrc:/qml/data_page.qml"))
                }
            }

            IconCardButton {
                btnWidth: 170
                btnHeight: 90
                iconSource: "qrc:/icon/qml/icon/usr_mng.png"
                onClicked: {
                    user_ctrl.logOperation("进入用户管理页面")
                    mainStackView.push("qrc:/qml/user_page.qml")
                }
            }
        }
    }
}
