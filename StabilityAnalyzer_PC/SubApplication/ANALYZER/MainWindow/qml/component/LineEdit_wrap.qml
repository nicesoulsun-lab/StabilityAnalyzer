import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Controls.impl 2.12
import QtQuick.Templates 2.12 as T

TextField {
    id: tf

    // ===== 核心控制属性 =====
    property bool multiLine: false       // true: 开启多行换行; false: 单行
    property int maxLines: 6             // 多行模式下最大显示行数
    property color border_color: "#58ACEE"
    property color bg_color: "#FFFFFF"
    //property var keyboard_inner: keyboard
    property var input_rules: RegExpValidator { regExp: /.*/ }
    property int m_radius: 5

    signal textChangedEx(string newText)
    property bool input_en: true

    // 基础样式
    placeholderText: multiLine ? "请输入备注..." : ""
    color: "#000000"
    font.pixelSize: 18
    selectByMouse: true
    selectionColor: palette.highlight
    selectedTextColor: palette.highlightedText
    placeholderTextColor: Color.transparent(color, 0.5)

    // 内边距
    leftPadding: 10
    rightPadding: 10
    topPadding: multiLine ? 10 : 0
    bottomPadding: multiLine ? 10 : 0

    // ===== 关键配置：换行与对齐 =====
    horizontalAlignment: TextInput.AlignLeft
    verticalAlignment: multiLine ? TextInput.AlignTop : TextInput.AlignVCenter
    wrapMode: multiLine ? Text.WrapAnywhere : Text.NoWrap

    // 禁止自动大写
    inputMethodHints: Qt.ImhNoAutoUppercase | Qt.ImhNoPredictiveText

    // 尺寸控制
    width: multiLine ? (parent ? parent.width : 300) : 128

    // 【关键修改】：不再动态计算 height，而是由 updateHeight 锁定最大高度
    // 这样可以避免滚动时高度塌陷

    background: Rectangle {
        radius: m_radius
        implicitWidth: tf.width
        implicitHeight: tf.height
        color: bg_color
        border.color: border_color
        border.width: 1
    }

    readOnly: !input_en
    validator: input_rules

    // 使用 TextMetrics 获取精确的单行高度
    TextMetrics {
        id: metrics
        font: tf.font
        text: "A" // 标准字符测试高度
    }

    // 计算单行实际占用高度 (字体高度 + 行间距)
    function getSingleLineHeight() {
        // Qt Controls 2 的行间距通常是 font.pixelSize 的 20%-25% 左右
        // 这里采用保守估计，或者直接利用 metrics.height (它通常包含基线到基线的距离)
        // 为了保险，我们显式加一点缓冲
        return metrics.height + Math.ceil(tf.font.pixelSize * 0.2);
    }

    function updateHeight() {
        if (!multiLine) {
            // 单行模式：固定高度
            tf.height = 40;
            return;
        }

        // 多行模式策略：
        // 1. 计算最大允许高度 (maxLines * 行高 + 内边距)
        var lineHeight = getSingleLineHeight();
        var maxContentHeight = maxLines * lineHeight;
        var fixedMaxHeight = maxContentHeight + topPadding + bottomPadding;

        // 2. 【核心逻辑】：
        // 在 Qt 5.12 嵌入式开发中，为了避免滚动条出现/消失导致的布局抖动，
        // 以及 paintedHeight 计算不准的问题，建议直接锁定高度为最大高度。
        // 这样无论输入多少文字，高度都不变，内部自动滚动。

        // 如果你非常希望“内容少时变矮，内容多时变高但不超过最大值”，可以使用下面的逻辑：
        /*
        var currentLinesEstimate = 1;
        if (tf.text.length > 0 && tf.contentItem) {
            // 简单的宽度估算行数 (比 paintedHeight 更稳)
            var contentWidth = tf.width - leftPadding - rightPadding;
            var metricsTemp = Qt.createQmlObject("import QtQuick 2.12; TextMetrics {}", tf);
            metricsTemp.font = tf.font;
            metricsTemp.text = tf.text;

            // 这是一个粗略估算，对于复杂换行可能不准，但在嵌入式上性能最好
            // 更好的方式是直接信任 maxLines 作为固定高度，体验更像原生控件
        }
        */

        // 【推荐方案】：只要 multiLine 为真，且有 maxLines 限制，直接设为最大高度。
        // 这能彻底解决“滚动时高度变成一行”的 Bug，因为高度不再依赖动态计算。
        tf.height = fixedMaxHeight;

        // 确保 contentItem 知道它可以滚动
        if (tf.contentItem) {
            tf.contentItem.clip = true;
        }
    }

    // 监听变化
    onTextChanged: {
        textChangedEx(tf.text);
        // 多行模式下，如果采用固定最大高度策略，textChanged 不需要频繁重算高度
        // 但如果需要“由少变多”的效果，可以取消下面这行的注释并配合复杂的行数计算
        // updateHeight();

        // 鉴于你的需求是“超出后高度不要变”，直接锁定是最优解。
        // 如果希望初始为空时高度小一点，可以加一个判断：
        if (multiLine) {
             if (text.length === 0 || text === "\n") {
                 // 空的时候只显示一行高度，避免界面空洞
                 tf.height = getSingleLineHeight() + topPadding + bottomPadding;
             } else {
                 // 只要有内容，就撑开到最大，保证滚动体验一致
                 tf.height = (maxLines * getSingleLineHeight()) + topPadding + bottomPadding;
             }
        }
    }

    onMultiLineChanged: updateHeight()
    onVisibleChanged: { if(visible) updateHeight() }
    onWidthChanged: {
        // 宽度改变可能导致换行行数改变，如果采用固定最大高度策略，这里其实不需要做什么
        // 但如果是动态高度策略，这里需要 updateHeight()
        if (multiLine && text.length > 0) {
             // 保持最大高度，防止宽度变化导致重算出错
             tf.height = (maxLines * getSingleLineHeight()) + topPadding + bottomPadding;
        }
    }

    Component.onCompleted: {
        updateHeight();
    }

    // 键盘交互逻辑 (保持原样)
//    onFocusChanged: {
//        if (focus && input_en) {
//            if (keyboard_inner) {
//                keyboard_inner.lase_focus_obj = this;
//                var pos_g = getGlobalPosition(this);
//                keyboard_inner.inputX = pos_g.x;
//                keyboard_inner.inputY = pos_g.y;
//                keyboard_inner.inputHeight = height;
//                keyboard_inner.visible = true;
//            }
//        }
//    }

    function getGlobalPosition(targetObject) {
        var positionX = 0;
        var positionY = 0;
        var obj = targetObject;
        while (obj !== null) {
            positionX += obj.x;
            positionY += obj.y;
            obj = obj.parent;
        }
        positionY += targetObject.height;
        return {"x": positionX, "y": positionY};
    }
}
