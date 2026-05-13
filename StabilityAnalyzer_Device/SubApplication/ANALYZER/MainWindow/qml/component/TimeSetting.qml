import QtQuick 2.0
import QtQuick.Controls 2.3


// Main TimeSetting component
Dialog {
    id: timeSet
    visible: false
    width: 650
    height: 400
    anchors.centerIn: parent
    modal: true

    // ===== 核心控制属性 =====
    property bool showTime: true  // true=完整时间选择, false=仅日期选择
    property var curyear: 0
    property var curmonth: 0
    property var curday: 0
    property var hour: 0
    property var minutes: 0
    property string fieldType: "start"  // 添加标记属性,默认为开始时间

    // 添加信号
    signal timeSelected(string selected_time, string field_type)

    Rectangle {
        width: 650
        height: 424
        anchors.centerIn: parent
        border.color: "#E5E5E5"
        border.width: 1
        color: "#FFFFFF"
        radius: 10

        // ===== 动态布局容器 =====
        Row {
            id: tumblerRow
            anchors.horizontalCenter: parent.horizontalCenter
            y: 65
            spacing: 0  // 紧密排列实现重叠效果

            // 动态计算宽度：5个Tumbler=420px, 3个Tumbler=260px
            width: showTime ? 420 : 260

            // ===== 年选择器 =====
            Tumbler {
                id: yearTumbler
                width: 100
                height: 240
                delegate: Item {
                    width: yearTumbler.width
                    height: 40
                    Rectangle {
                        anchors.fill: parent
                        color: index === yearTumbler.currentIndex ? "#EDE6F7" : "transparent"
                        opacity: 0.5
                    }
                    Text {
                        y: 10
                        text: modelData + qsTr("年")
                        font.pixelSize: 24 - Math.abs(index - yearTumbler.currentIndex) * 4
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        color: index === yearTumbler.currentIndex ? "#005BAC" : Qt.rgba(0, 0, 0, 1 - Math.abs(index - yearTumbler.currentIndex) * 0.2)
                    }
                }
                model: {
                    var array = [];
                    for (var i = 2020; i < 2100; ++i) {
                        array.push(i.toString());
                    }
                    return array;
                }
                onCurrentIndexChanged: {
                    if (monthTumbler.currentIndex === 1) {
                        dayTumbler.updateModel();
                    }
                    curyear = currentIndex;
                }
            }

            // ===== 月选择器 =====
            Tumbler {
                id: monthTumbler
                width: 80
                height: 240
                delegate: Item {
                    width: monthTumbler.width
                    height: 40
                    Rectangle {
                        anchors.fill: parent
                        color: index === monthTumbler.currentIndex ? "#EDE6F7" : "transparent"
                        opacity: 0.5
                    }
                    Text {
                        y: 10
                        text: modelData + qsTr("月")
                        font.pixelSize: 24 - Math.abs(index - monthTumbler.currentIndex) * 4
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        color: index === monthTumbler.currentIndex ? "#005BAC" : Qt.rgba(0, 0, 0, 1 - Math.abs(index - monthTumbler.currentIndex) * 0.2)
                    }
                }
                model: {
                    var array = [];
                    for (var i = 1; i <= 12; ++i) {
                        array.push(i < 10 ? "0" + i : i.toString());
                    }
                    return array;
                }
                onCurrentIndexChanged: {
                    curmonth = currentIndex;
                    dayTumbler.updateModel();
                }
            }

            // ===== 日选择器 =====
            Tumbler {
                id: dayTumbler
                width: 80
                height: 240
                delegate: Item {
                    width: dayTumbler.width
                    height: 40
                    Rectangle {
                        anchors.fill: parent
                        color: index === dayTumbler.currentIndex ? "#EDE6F7" : "transparent"
                        opacity: 0.5
                    }
                    Text {
                        y: 10
                        text: modelData + qsTr("日")
                        font.pixelSize: 24 - Math.abs(index - dayTumbler.currentIndex) * 4
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        color: index === dayTumbler.currentIndex ? "#005BAC" : Qt.rgba(0, 0, 0, 1 - Math.abs(index - dayTumbler.currentIndex) * 0.2)
                    }
                }
                readonly property var days: [31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31]

                // 初始 model 可设为空或默认，但会在 onCompleted 中更新
                model: []

                function updateModel() {
                    var previousDay = dayTumbler.currentIndex + 1; // 当前选中的日（1-based）
                    var month = monthTumbler.currentIndex; // 0-based
                    var year = yearTumbler.currentIndex + 2020;
                    var newDays = dayTumbler.days[month];
                    if (month === 1) { // February
                        if ((year % 4 === 0 && year % 100 !== 0) || (year % 400 === 0)) {
                            newDays = 29;
                        }
                    }
                    var array = [];
                    for (var i = 1; i <= newDays; ++i) {
                        array.push(i < 10 ? "0" + i : i.toString());
                    }
                    dayTumbler.model = array;
                    // 保持原日期，但不超过最大天数
                    var newIdx = Math.min(previousDay - 1, newDays - 1);
                    dayTumbler.currentIndex = Math.max(0, newIdx);
                    curday = dayTumbler.currentIndex;
                }

                onCurrentIndexChanged: {
                    curday = dayTumbler.currentIndex;
                }
            }

            // ===== 时选择器 (条件显示) =====
            Tumbler {
                id: hoursTumbler
                width: 80
                height: 240
                visible: showTime  // 关键：控制显示
                delegate: Item {
                    width: hoursTumbler.width
                    height: 40
                    Rectangle {
                        anchors.fill: parent
                        color: index === hoursTumbler.currentIndex ? "#EDE6F7" : "transparent"
                        opacity: 0.5
                    }
                    Text {
                        y: 10
                        text: modelData + qsTr("时")
                        font.pixelSize: 24 - Math.abs(index - hoursTumbler.currentIndex) * 4
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        color: index === hoursTumbler.currentIndex ? "#005BAC" : Qt.rgba(0, 0, 0, 1 - Math.abs(index - hoursTumbler.currentIndex) * 0.2)
                    }
                }
                model: {
                    var array = [];
                    for (var i = 0; i < 24; ++i) {
                        array.push(i < 10 ? "0" + i : i.toString());
                    }
                    return array;
                }
                onCurrentIndexChanged: {
                    hour = currentIndex;
                }
            }

            // ===== 分选择器 (条件显示) =====
            Tumbler {
                id: minutesTumbler
                width: 80
                height: 240
                visible: showTime  // 关键：控制显示
                delegate: Item {
                    width: minutesTumbler.width
                    height: 40
                    Rectangle {
                        anchors.fill: parent
                        color: index === minutesTumbler.currentIndex ? "#EDE6F7" : "transparent"
                        opacity: 0.5
                    }
                    Text {
                        y: 10
                        text: modelData + qsTr("分")
                        font.pixelSize: 24 - Math.abs(index - minutesTumbler.currentIndex) * 4
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        color: index === minutesTumbler.currentIndex ? "#005BAC" : Qt.rgba(0, 0, 0, 1 - Math.abs(index - minutesTumbler.currentIndex) * 0.2)
                    }
                }
                model: {
                    var array = [];
                    for (var i = 0; i < 60; ++i) {
                        array.push(i < 10 ? "0" + i : i.toString());
                    }
                    return array;
                }
                onCurrentIndexChanged: {
                    minutes = currentIndex;
                }
            }
        }

        // ===== 确定按钮 (自动居中) =====
        Button {
            anchors.horizontalCenter: parent.horizontalCenter
            y: 345
            width: 150
            height: 45
            background: Rectangle {
                radius: 10
                color: "transparent"
                Image {
                    source: "qrc:/icon/qml/icon/ensure.png"
                    width: parent.width
                    height: parent.height
                }
            }
            Text {
                anchors.centerIn: parent
                text: qsTr("确 定")
                font.pixelSize: 19
                color: "#FFFFFF"
            }
            onClicked: {
                // ===== 智能时间处理：仅日期模式根据fieldType设置时分秒 =====
                var hourValue, minuteValue, secondValue;

                if (showTime) {
                    // 显示时分模式：使用用户选择的值
                    hourValue = hour;
                    minuteValue = minutes;
                    secondValue = "00";
                } else {
                    // 不显示时分模式：根据fieldType自动设置
                    if (fieldType === "start") {
                        hourValue = 0;
                        minuteValue = 0;
                        secondValue = "00";
                    } else { // fieldType === "end"
                        hourValue = 23;
                        minuteValue = 59;
                        secondValue = "59";
                    }
                }

                var selected_time = (curyear + 2020).toString() + "-" +
                        (curmonth + 1).toString().padStart(2, '0') + "-" +
                        (curday + 1).toString().padStart(2, '0') + " " +
                        hourValue.toString().padStart(2, '0') + ":" +
                        minuteValue.toString().padStart(2, '0') + ":" +
                        secondValue;

                timeSelected(selected_time, fieldType);
                timeSet.close();
            }
        }
    }

    onVisibleChanged: {
        if (!visible) return;

        // ===== 初始化时间逻辑 =====
        var year = 2020, month = 0, day = 1, hourVal = 0, minuteVal = 0;
        var dateTimeStr = "";

        // 1. 尝试从 system_ctrl 获取时间
        // 假设 system_ctrl 是一个已注册的单例对象或上下文属性
        if (typeof system_ctrl !== "undefined" && typeof system_ctrl.getDateTime === "function") {
            dateTimeStr = system_ctrl.getDateTime();
            // 预期格式: "yyyy-MM-dd hh:mm:ss" (例如: "2023-10-27 14:30:59")
        }

        // 2. 解析字符串 (如果获取失败或为空，则保持默认值 2020-01-01，或者你可以选择回退到 new Date())
        if (dateTimeStr && dateTimeStr.length >= 16) {
            // 解析日期部分: "yyyy-MM-dd"
            // substring(0, 4) -> year
            // substring(5, 7) -> month
            // substring(8, 10) -> day
            year = parseInt(dateTimeStr.substring(0, 4));
            month = parseInt(dateTimeStr.substring(5, 7)) - 1; // JS Month is 0-11
            day = parseInt(dateTimeStr.substring(8, 10));

            // 解析时间部分: "hh:mm:ss" (仅当需要时分时才解析)
            if (showTime) {
                // substring(11, 13) -> hour
                // substring(14, 16) -> minute
                hourVal = parseInt(dateTimeStr.substring(11, 13));
                minuteVal = parseInt(dateTimeStr.substring(14, 16));
            }
        } else {
            // 【可选】如果 system_ctrl 不可用，回退到系统 JS 时间，防止界面显示错误默认值
            console.warn("system_ctrl.getDateTime() unavailable or invalid, falling back to JS Date.");
            var now = new Date();
            year = now.getFullYear();
            month = now.getMonth();
            day = now.getDate();
            if (showTime) {
                hourVal = now.getHours();
                minuteVal = now.getMinutes();
            }
        }

        // 3. 设置 Tumbler 索引
        // 年: 模型从 2020 开始，所以 currentIndex = year - 2020
        // 限制范围防止越界 (2020-2099)
        yearTumbler.currentIndex = Math.max(0, Math.min(79, year - 2020));

        // 月: 模型 0-11 对应 1-12月
        monthTumbler.currentIndex = Math.max(0, Math.min(11, month));

        // 日: 必须先更新模型以匹配当前年月天数，再设置索引
        dayTumbler.updateModel();
        dayTumbler.currentIndex = Math.max(0, Math.min(dayTumbler.model.length - 1, day - 1));

        // 4. 设置时间部分
        if (showTime) {
            hoursTumbler.currentIndex = Math.max(0, Math.min(23, hourVal));
            minutesTumbler.currentIndex = Math.max(0, Math.min(59, minuteVal));
        } else {
            // 仅日期模式：根据 fieldType 自动设置时分 (保持原有逻辑)
            if (fieldType === "start") {
                hoursTumbler.currentIndex = 0;
                minutesTumbler.currentIndex = 0;
            } else { // fieldType === "end"
                hoursTumbler.currentIndex = 23;
                minutesTumbler.currentIndex = 59;
            }
        }

        // 5. 同步内部属性变量
        curyear = yearTumbler.currentIndex;
        curmonth = monthTumbler.currentIndex;
        curday = dayTumbler.currentIndex;
        hour = hoursTumbler.currentIndex;
        minutes = minutesTumbler.currentIndex;
    }

    // 保留辅助函数（虽然不再用于初始化）
    function getCurrentYear() { return new Date().getFullYear(); }
    function getCurrentMonth() { return new Date().getMonth() + 1; }
    function getCurrentDate() { return new Date().getDate(); }
    function getCurrentHour() { return new Date().getHours(); }
    function getCurrentMinute() { return new Date().getMinutes(); }
}
