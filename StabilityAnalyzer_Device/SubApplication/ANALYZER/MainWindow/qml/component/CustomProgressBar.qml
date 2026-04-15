import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12

RowLayout {
    id: control
    spacing: 10

    // --- 公共属性 ---
    // true = 无限滚动模式 (默认), false = 进度条百分比模式
    property bool indeterminate: true
    // 进度值 0.0 - 1.0
    property real value: 0.0

    // --- 样式属性 ---
    property color colorBackground: "#f0f0f0"
    property color colorFill: "#3498db" // 蓝色
    property color colorText: "#999999" // 灰色文字
    property int barHeight: 6

    // 进度条轨道容器
    Item {
        Layout.fillWidth: true
        Layout.preferredHeight: control.barHeight
        clip: true // 防止动画溢出

        // 1. 背景槽
        Rectangle {
            anchors.fill: parent
            color: control.colorBackground
            radius: height / 2
        }

        // 2. 填充块 (两种模式共用一个Rectangle，行为不同)
        Rectangle {
            id: fillRect
            height: parent.height
            radius: height / 2
            color: control.colorFill

            // --- 宽度逻辑 ---
            // 模式1(无限): 固定宽度 100
            // 模式2(进度): 根据 value 动态计算
            width: control.indeterminate ? 100 : (parent.width * control.value)

            // --- X轴逻辑 ---
            // 模式1: 由动画控制
            // 模式2: 固定在左边 0
            x: control.indeterminate ? x : 0
        }

        // 3. 无限循环动画 (仅在 indeterminate 为 true 时运行)
        SequentialAnimation {
            id: marqueeAnim
            running: control.indeterminate && control.visible
            loops: Animation.Infinite

            // 每次循环开始前重置位置到最左边外部
            ScriptAction { script: fillRect.x = -fillRect.width }

            NumberAnimation {
                target: fillRect
                property: "x"
                from: -100 // 从左侧外部进入
                to: fillRect.parent.width // 移动到最右侧
                duration: 1500
                easing.type: Easing.InOutQuad
            }
        }
    }

    // 4. 百分比文字 (仅在 indeterminate 为 false 时显示)
    UiText {
        visible: !control.indeterminate
        Layout.preferredWidth: 40 // 预留宽度防止跳动
        Layout.alignment: Qt.AlignVCenter
        text: Math.floor(control.value * 100) + "%"
        horizontalAlignment: Text.AlignRight
    }
}
