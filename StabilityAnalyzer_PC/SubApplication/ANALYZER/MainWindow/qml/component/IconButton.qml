import QtQuick 2.12
import QtQuick.Controls 2.12

Button {
    id: root

    // =========================
    // 自定义属性 (保持不变)
    // =========================
    property string button_text: ""
    property color button_color: "transparent"
    property int button_radius: 4

    property url icon_source: ""
    property int icon_source_width: 0
    property int icon_source_height: 0

    property url background_source: ""
    property int button_icon_spacing: 6

    property int pixelSize: 14
    property color text_color: "#005EB6"
    property string family: "Microsoft YaHei" // 建议给个默认字体，防报错

    property bool vertical: false
    property color border_color: "transparent"
    property int border_width: 0

    // ============================================================
    // 【核心修复】 动画逻辑重写
    // ============================================================

    // 1. 绑定缩放比例到按钮的按下状态 (down)
    //    down 是 Button 的原生属性：按下为 true，松开为 false
    //    按下时变小 (0.92)，松开恢复 (1.0)
    scale: root.down ? 0.92 : 1.0

    // 2. 使用 Behavior 自动平滑过渡
    //    无论你点击多快，Qt 都会自动处理数值的过渡，不会卡顿
    Behavior on scale {
        NumberAnimation {
            duration: 100
            easing.type: Easing.OutQuad
        }
    }

    // (移除了原来的 transform: Scale 和 SequentialAnimation 代码)

    // =========================
    // 背景样式
    // =========================
    background: Rectangle {
        anchors.fill: parent
        radius: root.button_radius

        // 自动处理颜色变化：按下变深，悬停变浅，默认用 button_color
        color: root.down ? Qt.darker(root.button_color, 1.1) :
                           (root.hovered ? Qt.lighter(root.button_color, 1.05) : root.button_color)

        clip: true
        border.color: root.border_color
        border.width: root.border_width

        // 背景图片支持
        Image {
            id: bgImage
            anchors.fill: parent
            source: root.background_source
            fillMode: Image.PreserveAspectCrop
            asynchronous: true
            visible: root.background_source !== "" && status === Image.Ready
        }
    }

    // =========================
    // 内容布局 (Loader 动态加载横/竖布局)
    // =========================
    contentItem: Item {
        anchors.fill: parent

        Loader {
            id: layoutLoader
            anchors.centerIn: parent
            sourceComponent: root.vertical ? columnComponent : rowComponent
        }
    }
    // --- 横向布局组件 (图标在左，文字在右) ---
    Component {
        id: rowComponent
        Row {
            spacing: root.button_icon_spacing
            anchors.centerIn: parent

            Image {
                id: iconImage_Row
                source: root.icon_source
                width: root.icon_source_width
                height: root.icon_source_height
                fillMode: Image.PreserveAspectFit
                visible: source !== ""
                anchors.verticalCenter: parent.verticalCenter // 确保图标也垂直居中
            }

            Text {
                id: labelText_Row
                text: root.button_text
                font.pixelSize: root.pixelSize
                color: root.text_color
                font.family: root.family
                anchors.verticalCenter: parent.verticalCenter

                // --- 换行核心逻辑 ---
                // 计算 8 个字符的大致宽度限制
                width: text.length > 8 ? font.pixelSize * 8 : undefined
                wrapMode: Text.WrapAnywhere
                horizontalAlignment: Text.AlignHCenter
            }
        }
    }

    // --- 纵向布局组件 (图标在上，文字在下) ---
    Component {
        id: columnComponent
        Column {
            spacing: root.button_icon_spacing
            anchors.centerIn: parent

            Image {
                id: iconImage_Column
                source: root.icon_source
                width: root.icon_source_width
                height: root.icon_source_height
                fillMode: Image.PreserveAspectFit
                visible: source !== ""
                anchors.horizontalCenter: parent.horizontalCenter
            }

            Text {
                id: labelText_Column
                text: root.button_text
                font.pixelSize: root.pixelSize
                color: root.text_color
                font.family: root.family
                anchors.horizontalCenter: parent.horizontalCenter

                // --- 换行核心逻辑 ---
                // 纵向布局下，换行通常需要更明确的对齐
                width: text.length > 8 ? font.pixelSize * 8 : undefined
                wrapMode: Text.WrapAnywhere
                horizontalAlignment: Text.AlignHCenter
            }
        }
    }
}
