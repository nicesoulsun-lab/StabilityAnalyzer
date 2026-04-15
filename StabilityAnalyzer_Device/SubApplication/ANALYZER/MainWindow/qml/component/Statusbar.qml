/*
      这是一个导航栏组件
    */
import QtQuick 2.12
import QtQuick.Layouts 1.12
import QtQuick.Controls 2.12

Rectangle {
    property int pixelSize : 14
    property string text_color : "#005EB6"
    //property string family : "Noto Sans SC" // 建议写死或确保外部已定义 notoSansSCRegular
    property bool home_show: window.home_show
    property int qml_wifi_strength: -1

    id: stausbar
    anchors {
        left: parent.left
        leftMargin: 23
        right: parent.right
        rightMargin: 23
        top: parent.top
        topMargin: 30
    }
    height: 23
    color: "transparent"

    // 监听 system_ctrl 的 WiFi 状态变化
    Connections {
        target: typeof system_ctrl !== "undefined" ? system_ctrl : null
        onWifiConnectedChanged: {
            updateWifiIcon()
        }
        onWifiIntensityChanged: {
            updateWifiIcon()
        }
    }

    //初始化完成后】
    Component.onCompleted: {
        console.log("刷新 wifi")
        updateWifiIcon()
    }

    // 更新 WiFi 图标显示
    function updateWifiIcon() {
        if (typeof system_ctrl !== "undefined") {
            console.log("=== WiFi 状态检查 ===")
            console.log("system_ctrl 存在:", system_ctrl)
            console.log("wifiConnected 属性:", system_ctrl.wifiConnected)
            console.log("wifiConnected 类型:", typeof system_ctrl.wifiConnected)
            console.log("wifiIntensity 属性:", system_ctrl.wifiIntensity)
            
            // 使用 wifiConnected 判断是否连接（有值表示已连接）
            var isConnected = system_ctrl.wifiConnected && system_ctrl.wifiConnected.toString().length > 0
            wifi.visible = isConnected
            
            if (isConnected) {
                // 使用 wifiIntensity 获取信号强度（字符串转整数）
                var signalStrength = parseInt(system_ctrl.wifiIntensity)
                wifi.source = get_wifi_png_strength(signalStrength)
                console.log("WiFi 已连接:", system_ctrl.wifiConnected, "信号强度:", signalStrength, "图标:", wifi.source)
            } else {
                console.log("WiFi 未连接")
            }
        } else {
            console.log("system_ctrl 未定义！")
        }
    }

    // --- 修改点 1: 适配后端 WIFI 信号逻辑 ---
    Text {
        id:device_name
        text: qsTr("稳定性分析仪")
        height: parent.height
        anchors{
            left: parent.left
        }
        verticalAlignment: Text.AlignVCenter

        font.pixelSize: pixelSize
        color: text_color
        //font.family: family
    }

    IconButton{
        id:home
        width: 120
        height: 40
        button_text: qsTr("返  回")
        button_color: "#FFFFFF"
        button_radius: 5
        icon_source: "qrc:/icon/qml/icon/back.png"
        icon_source_width: 20
        icon_source_height: 20
        button_icon_spacing: 7

        anchors{
            right: parent.right
            verticalCenter: parent.verticalCenter
        }
        visible: home_show

        onClicked:{
            if(mainStackView.currentItem.objectName === "ParaSetting_A" ||
                    mainStackView.currentItem.objectName === "ParaSetting_B" ||
                    mainStackView.currentItem.objectName === "ParaSetting_C" ||
                    mainStackView.currentItem.objectName === "ParaSetting_D"
                    )
            {
                mainStackView.pop()
                select_channel_pop.open()
            }
            else mainStackView.pop()

        }
    }

    Image{
        id:wifi
        source: "qrc:/icon/qml/icon/wifi_1.png"
        width: 21
        height: 18

        anchors{
            right: home_show ? home.left : parent.right
            rightMargin: 25
            verticalCenter: parent.verticalCenter
        }
        
        visible: false  // 初始隐藏，由 updateWifiIcon() 函数控制
    }

    Text{
        id:time_text
        text: qsTr("2025/10/23 13:37 周四")
        height: parent.height
        anchors{
            right: wifi.left
            rightMargin: 25
        }
        verticalAlignment: Text.AlignVCenter

        font.pixelSize: pixelSize
        color: text_color
        //font.family: family
    }

    // --- 修改点 2: 纯前端获取时间 (后端无此函数) ---
    Timer{
        interval: 1000
        running: true
        repeat: true
        triggeredOnStart: true // 启动时立即执行一次
        onTriggered: {
            // 使用 JS 获取时间，无需调用 m_system.get_current_time()
            time_text.text = get_current_date_string()
        }
    }

    // 辅助函数：生成时间字符串
    function get_current_date_string() {
        var date = new Date();
        var weekDays = [qsTr("周日"), qsTr("周一"), qsTr("周二"), qsTr("周三"), qsTr("周四"), qsTr("周五"), qsTr("周六")];
        // 格式化为: 2025/10/23 13:37 周四
        return Qt.formatDateTime(date, "yyyy/MM/dd hh:mm ") + weekDays[date.getDay()];
    }

    function get_wifi_png_strength(strength){
        if(strength <= 0){
            return "qrc:/icon/qml/icon/wifi_4.png"
        }

        if(strength >= 80){
            return "qrc:/icon/qml/icon/wifi_1.png"
        }else if(strength >= 60){
            return "qrc:/icon/qml/icon/wifi_2.png"
        }else if(strength >= 40){
            return "qrc:/icon/qml/icon/wifi_3.png"
        }else{
            return "qrc:/icon/qml/icon/wifi_4.png"
        }
    }
}
