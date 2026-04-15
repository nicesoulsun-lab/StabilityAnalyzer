import QtQuick 2.12
import QtQuick.Window 2.12 // 必须引入，为了使用 Window 属性

Item {
    id: root
    anchors.fill: parent
    z: 999 // 默认最高层级，防止被遮挡

    // --- 对外暴露的属性 ---
    property Window targetWindow // 绑定的目标窗口
    property int edgeSize: 6     // 边缘拉伸触发宽度
    property color borderColor: "#888888" // 边框颜色
    property int borderWidth: 1           // 边框宽度

    // --- 1. 辅助视觉：边框层 ---
    Rectangle {
        anchors.fill: parent
        color: "transparent"
        border.color: root.borderColor
        border.width: root.borderWidth
    }

    // --- 2. 交互：边缘拉伸控制区 ---
    // 左边缘
    MouseArea {
        anchors { left: parent.left; top: parent.top; bottom: parent.bottom; topMargin: root.edgeSize; bottomMargin: root.edgeSize }
        width: root.edgeSize; cursorShape: Qt.SizeHorCursor
        onPositionChanged: {
            if (pressed && targetWindow) {
                var dX = mouseX;
                if (targetWindow.width - dX > targetWindow.minimumWidth) {
                    targetWindow.x += dX; targetWindow.width -= dX
                }
            }
        }
    }
    // 右边缘
    MouseArea {
        anchors { right: parent.right; top: parent.top; bottom: parent.bottom; topMargin: root.edgeSize; bottomMargin: root.edgeSize }
        width: root.edgeSize; cursorShape: Qt.SizeHorCursor
        onPositionChanged: {
            if (pressed && targetWindow) {
                var mappedX = mapToItem(root, mouseX, mouseY).x;
                if (mappedX > targetWindow.minimumWidth) targetWindow.width = mappedX
            }
        }
    }
    // 上边缘
    MouseArea {
        anchors { left: parent.left; right: parent.right; top: parent.top; leftMargin: root.edgeSize; rightMargin: root.edgeSize }
        height: root.edgeSize; cursorShape: Qt.SizeVerCursor
        onPositionChanged: {
            if (pressed && targetWindow) {
                var dY = mouseY;
                if (targetWindow.height - dY > targetWindow.minimumHeight) {
                    targetWindow.y += dY; targetWindow.height -= dY
                }
            }
        }
    }
    // 下边缘
    MouseArea {
        anchors { left: parent.left; right: parent.right; bottom: parent.bottom; leftMargin: root.edgeSize; rightMargin: root.edgeSize }
        height: root.edgeSize; cursorShape: Qt.SizeVerCursor
        onPositionChanged: {
            if (pressed && targetWindow) {
                var mappedY = mapToItem(root, mouseX, mouseY).y;
                if (mappedY > targetWindow.minimumHeight) targetWindow.height = mappedY
            }
        }
    }
    // 左上角
    MouseArea {
        anchors { left: parent.left; top: parent.top }
        width: root.edgeSize; height: root.edgeSize; cursorShape: Qt.SizeFDiagCursor
        onPositionChanged: {
            if (pressed && targetWindow) {
                var dX = mouseX; var dY = mouseY
                if (targetWindow.width - dX > targetWindow.minimumWidth) { targetWindow.x += dX; targetWindow.width -= dX }
                if (targetWindow.height - dY > targetWindow.minimumHeight) { targetWindow.y += dY; targetWindow.height -= dY }
            }
        }
    }
    // 右上角
    MouseArea {
        anchors { right: parent.right; top: parent.top }
        width: root.edgeSize; height: root.edgeSize; cursorShape: Qt.SizeBDiagCursor
        onPositionChanged: {
            if (pressed && targetWindow) {
                var mappedX = mapToItem(root, mouseX, mouseY).x; var dY = mouseY
                if (mappedX > targetWindow.minimumWidth) targetWindow.width = mappedX
                if (targetWindow.height - dY > targetWindow.minimumHeight) { targetWindow.y += dY; targetWindow.height -= dY }
            }
        }
    }
    // 左下角
    MouseArea {
        anchors { left: parent.left; bottom: parent.bottom }
        width: root.edgeSize; height: root.edgeSize; cursorShape: Qt.SizeBDiagCursor
        onPositionChanged: {
            if (pressed && targetWindow) {
                var dX = mouseX; var mappedY = mapToItem(root, mouseX, mouseY).y
                if (targetWindow.width - dX > targetWindow.minimumWidth) { targetWindow.x += dX; targetWindow.width -= dX }
                if (mappedY > targetWindow.minimumHeight) targetWindow.height = mappedY
            }
        }
    }
    // 右下角
    MouseArea {
        anchors { right: parent.right; bottom: parent.bottom }
        width: root.edgeSize; height: root.edgeSize; cursorShape: Qt.SizeFDiagCursor
        onPositionChanged: {
            if (pressed && targetWindow) {
                var pt = mapToItem(root, mouseX, mouseY)
                if (pt.x > targetWindow.minimumWidth) targetWindow.width = pt.x
                if (pt.y > targetWindow.minimumHeight) targetWindow.height = pt.y
            }
        }
    }
}
