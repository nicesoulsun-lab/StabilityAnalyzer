import QtQuick 2.12
import QtQuick.Window 2.12
import QtQuick.Controls 2.12
import QtQuick.Controls.impl 2.12
import QtQuick.Templates 2.12 as T

TextField {
    id: tf

    implicitWidth: implicitBackgroundWidth;
    implicitHeight: implicitBackgroundHeight;

    property color border_color: "#58ACEE"
    property color bg_color: "#FFFFFF"
    //property var keyboard_inner: keyboard // 虚拟键盘必须传入
    property var input_rules: RegExpValidator { regExp: /.*/ }
    property int m_radius: 5
    signal textChangedEx(string newText)
    property bool input_en: true

    //padding: 2;
    //leftPadding: padding + 4;

    placeholderText: ""

    color: "#000000";
    font.pixelSize: 14;

    //font.family: notoSansSCRegular.name

    selectByMouse: true;
    selectionColor: palette.highlight;
    selectedTextColor: palette.highlightedText;
    placeholderTextColor: Color.transparent(color, 0.5);

    verticalAlignment: TextInput.AlignVCenter;
    horizontalAlignment: TextInput.AlignLeft;

    leftPadding: 10
    rightPadding: 0

    bottomPadding: 0
    topPadding: 0

    width: 128
    height: 40

    // 文本输入框背景属性设置
    background: Rectangle {
        radius: m_radius
        implicitWidth: implicitBackgroundWidth
        implicitHeight: implicitBackgroundHeight
        color: bg_color
        border.color: border_color
        border.width: 1

    }

    //禁止编辑
    readOnly: !input_en

    // 正则输入限制
    validator: input_rules

    // 键盘预选
    inputMethodHints: Qt.ImhNoAutoUppercase

    /*
        Qt.ImhNone: 没有任何特殊提示。
        Qt.ImhHiddenText: 输入文本隐藏。
        Qt.ImhSensitiveData: 输入的是敏感数据，例如密码。
        Qt.ImhNoAutoUppercase: 禁用自动大写字母功能。
        Qt.ImhPreferNumbers: 首选数字键盘。
        Qt.ImhPreferUppercase: 首选大写字母键盘。
        Qt.ImhPreferLowercase: 首选小写字母键盘。
        Qt.ImhNoPredictiveText: 禁用输入预测文本功能。
        Qt.ImhDigitsOnly: 只允许输入数字。
        Qt.ImhFormattedNumbersOnly: 只允许输入格式化的数字。
        Qt.ImhUppercaseOnly: 只允许输入大写字母。
        Qt.ImhLowercaseOnly: 只允许输入小写字母。
        Qt.ImhNoEditMenu: 禁用编辑菜单。
        Qt.ImhDialableCharactersOnly: 只允许输入可拨号字符。
    */

    // 动态测量文本宽度，填满就不再输入
    TextMetrics {
        id: textMetrics
        font: tf.font
        text: tf.text
    }

    onTextChanged: {
        textMetrics.text = text
        var maxWidth = width - leftPadding - rightPadding
        if (textMetrics.width > maxWidth) {
            // 超过宽度，截断
            var truncated = text
            do {
                truncated = truncated.slice(0, -1)
                textMetrics.text = truncated
            } while (textMetrics.width > maxWidth && truncated.length > 0)
            if (truncated !== text) {
                tf.text = truncated
            }
        }
        textChangedEx(tf.text)
    }

    // 选择输入框时才显示键盘
//    onFocusChanged: {
//        if (focus && input_en) {
//            keyboard_inner.lase_focus_obj = this;

//            var pos_g = getGlobalPosition(this)
//            keyboard_inner.inputX = pos_g.x
//            keyboard_inner.inputY = pos_g.y
//            keyboard_inner.inputHeight = height
//            keyboard_inner.visible = true
//        }
//    }

    function getGlobalPosition(targetObject) {
        var positionX = 0
        var positionY = 0
        var obj = targetObject

        while (obj !== null) {
            positionX += obj.x
            positionY += obj.y
            obj = obj.parent
        }
        positionY += height
        return {"x": positionX, "y": positionY}
    }
}
