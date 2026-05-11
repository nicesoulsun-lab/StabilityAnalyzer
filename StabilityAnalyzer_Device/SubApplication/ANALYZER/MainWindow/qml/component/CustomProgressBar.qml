import QtQuick 2.9
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3

RowLayout {
    id: control
    spacing: 10

    // --- 鍏叡灞炴€?---
    // true = 鏃犻檺婊氬姩妯″紡 (榛樿), false = 杩涘害鏉＄櫨鍒嗘瘮妯″紡
    property bool indeterminate: true
    // 杩涘害鍊?0.0 - 1.0
    property real value: 0.0

    // --- 鏍峰紡灞炴€?---
    property color colorBackground: "#f0f0f0"
    property color colorFill: "#3498db" // 钃濊壊
    property color colorText: "#999999" // 鐏拌壊鏂囧瓧
    property int barHeight: 6

    // 杩涘害鏉¤建閬撳鍣?
    Item {
        Layout.fillWidth: true
        Layout.preferredHeight: control.barHeight
        clip: true // 闃叉鍔ㄧ敾婧㈠嚭

        // 1. 鑳屾櫙妲?
        Rectangle {
            anchors.fill: parent
            color: control.colorBackground
            radius: height / 2
        }

        // 2. 濉厖鍧?(涓ょ妯″紡鍏辩敤涓€涓猂ectangle锛岃涓轰笉鍚?
        Rectangle {
            id: fillRect
            height: parent.height
            radius: height / 2
            color: control.colorFill

            // --- 瀹藉害閫昏緫 ---
            // 妯″紡1(鏃犻檺): 鍥哄畾瀹藉害 100
            // 妯″紡2(杩涘害): 鏍规嵁 value 鍔ㄦ€佽绠?
            width: control.indeterminate ? 100 : (parent.width * control.value)

            // --- X杞撮€昏緫 ---
            // 妯″紡1: 鐢卞姩鐢绘帶鍒?
            // 妯″紡2: 鍥哄畾鍦ㄥ乏杈?0
            x: control.indeterminate ? x : 0
        }

        // 3. 鏃犻檺寰幆鍔ㄧ敾 (浠呭湪 indeterminate 涓?true 鏃惰繍琛?
        SequentialAnimation {
            id: marqueeAnim
            running: control.indeterminate && control.visible
            loops: Animation.Infinite

            // 姣忔寰幆寮€濮嬪墠閲嶇疆浣嶇疆鍒版渶宸﹁竟澶栭儴
            ScriptAction { script: fillRect.x = -fillRect.width }

            NumberAnimation {
                target: fillRect
                property: "x"
                from: -100 // 浠庡乏渚у閮ㄨ繘鍏?
                to: fillRect.parent.width // 绉诲姩鍒版渶鍙充晶
                duration: 1500
                easing.type: Easing.InOutQuad
            }
        }
    }

    // 4. 鐧惧垎姣旀枃瀛?(浠呭湪 indeterminate 涓?false 鏃舵樉绀?
    UiText {
        visible: !control.indeterminate
        Layout.preferredWidth: 40 // 棰勭暀瀹藉害闃叉璺冲姩
        Layout.alignment: Qt.AlignVCenter
        text: Math.floor(control.value * 100) + "%"
        horizontalAlignment: Text.AlignRight
    }
}

