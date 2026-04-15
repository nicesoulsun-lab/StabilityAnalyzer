import QtQuick 2.12
import QtQuick.Layouts 1.12

// 根元素使用 Item，因为我们需要它作为一个透明的容器，内部包含 Image 和 MouseArea
Item {
    id: root

    // =========================
    // 对外暴露的属性 (Properties)
    // =========================
    
    // 按钮尺寸
    property int btnWidth: 170
    property int btnHeight: 90
    
    // 图片资源
    property url iconSource: ""
    
    // 是否启用悬停效果 (可选)
    property bool enableHover: true

    // =========================
    // 对外暴露的信号 (Signals)
    // =========================
    
    // 定义一个点击信号，供外部连接
    signal clicked()

    // =========================
    // 内部实现
    // =========================

    // 1. 应用尺寸
    width: btnWidth
    height: btnHeight

    // 2. 点击缩放动画逻辑
    // 绑定到内部 mouseArea 的 pressed 状态
    scale: mouseArea.pressed ? 0.92 : 1.0
    transformOrigin: Item.Center // 关键：以中心为轴缩放

    Behavior on scale {
        NumberAnimation {
            duration: 100
            easing.type: Easing.OutQuad
        }
    }

    // 3. 背景/图标图片
    Image {
        anchors.fill: parent
        source: root.iconSource
        fillMode: Image.PreserveAspectFit
        
        // 可选：点击时稍微变暗，增加质感
        opacity: mouseArea.pressed ? 0.85 : 1.0
        Behavior on opacity {
            NumberAnimation { duration: 100 }
        }
    }

    // 4. 鼠标交互区域
    MouseArea {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: root.enableHover
        cursorShape: Qt.PointingHandCursor

        onClicked: {
            // 触发对外暴露的信号
            root.clicked()
        }
    }
}