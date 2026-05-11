/*
      杩欐槸涓€涓鑸爮缁勪欢
    */
import QtQuick 2.9
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.2

Rectangle {
    property int pixelSize : 14
    property string text_color : "#005EB6"
    //property string family : "Noto Sans SC" // 寤鸿鍐欐鎴栫‘淇濆閮ㄥ凡瀹氫箟 notoSansSCRegular
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

    // 鐩戝惉 system_ctrl 鐨?WiFi 鐘舵€佸彉鍖?
    Connections {
        target: typeof system_ctrl !== "undefined" ? system_ctrl : null
        onWifiConnectedChanged: {
            updateWifiIcon()
        }
        onWifiIntensityChanged: {
            updateWifiIcon()
        }
    }

    //鍒濆鍖栧畬鎴愬悗銆?
    Component.onCompleted: {
        console.log("鍒锋柊 wifi")
        updateWifiIcon()
    }

    // 鏇存柊 WiFi 鍥炬爣鏄剧ず
    function updateWifiIcon() {
        if (typeof system_ctrl !== "undefined") {
            console.log("=== WiFi 鐘舵€佹鏌?===")
            console.log("system_ctrl 瀛樺湪:", system_ctrl)
            console.log("wifiConnected 灞炴€?", system_ctrl.wifiConnected)
            console.log("wifiConnected 绫诲瀷:", typeof system_ctrl.wifiConnected)
            console.log("wifiIntensity 灞炴€?", system_ctrl.wifiIntensity)
            
            // 浣跨敤 wifiConnected 鍒ゆ柇鏄惁杩炴帴锛堟湁鍊艰〃绀哄凡杩炴帴锛?
            var isConnected = system_ctrl.wifiConnected && system_ctrl.wifiConnected.toString().length > 0
            wifi.visible = isConnected
            
            if (isConnected) {
                // 浣跨敤 wifiIntensity 鑾峰彇淇″彿寮哄害锛堝瓧绗︿覆杞暣鏁帮級
                var signalStrength = parseInt(system_ctrl.wifiIntensity)
                wifi.source = get_wifi_png_strength(signalStrength)
                console.log("WiFi 宸茶繛鎺?", system_ctrl.wifiConnected, "淇″彿寮哄害:", signalStrength, "鍥炬爣:", wifi.source)
            } else {
                console.log("WiFi 鏈繛鎺?)
            }
        } else {
            console.log("system_ctrl 鏈畾涔夛紒")
        }
    }

    // --- 淇敼鐐?1: 閫傞厤鍚庣 WIFI 淇″彿閫昏緫 ---
    Text {
        id:device_name
        text: qsTr("绋冲畾鎬у垎鏋愪华")
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
        button_text: qsTr("杩? 鍥?)
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
        
        visible: false  // 鍒濆闅愯棌锛岀敱 updateWifiIcon() 鍑芥暟鎺у埗
    }

    Text{
        id:time_text
        text: qsTr("2025/10/23 13:37 鍛ㄥ洓")
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

    // --- 淇敼鐐?2: 绾墠绔幏鍙栨椂闂?(鍚庣鏃犳鍑芥暟) ---
    Timer{
        interval: 1000
        running: true
        repeat: true
        triggeredOnStart: true // 鍚姩鏃剁珛鍗虫墽琛屼竴娆?
        onTriggered: {
            // 浣跨敤 JS 鑾峰彇鏃堕棿锛屾棤闇€璋冪敤 m_system.get_current_time()
            time_text.text = get_current_date_string()
        }
    }

    // 杈呭姪鍑芥暟锛氱敓鎴愭椂闂村瓧绗︿覆
    function get_current_date_string() {
        var date = new Date();
        var weekDays = [qsTr("鍛ㄦ棩"), qsTr("鍛ㄤ竴"), qsTr("鍛ㄤ簩"), qsTr("鍛ㄤ笁"), qsTr("鍛ㄥ洓"), qsTr("鍛ㄤ簲"), qsTr("鍛ㄥ叚")];
        // 鏍煎紡鍖栦负: 2025/10/23 13:37 鍛ㄥ洓
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

