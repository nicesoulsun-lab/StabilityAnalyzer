import QtQuick 2.9
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3
import "component"

Item {
    id: homepage
    width: 972
    height: 501

    objectName: "ParaSetting_D"

    // 椤圭洰鍚嶇О鍒楄〃
    property var nameList: data_ctrl.getProjectName()
    // 閫氶亾鏍囪瘑锛?-閫氶亾D
    property int channel: 3
    // 鍗曟鎵弿鎵€闇€鏃堕棿锛堢锛夛紝渚夸簬鍚庣画淇敼
    property int scanTime: 20
    // 闃叉寰幆璁＄畻鐨勬爣蹇?
    property bool isCalculating: false

    // 鑾峰彇鎸佺画鏃堕棿鎬荤鏁?
    function getDurationSeconds() {
        var days = parseInt(continue_time_day_edit.text) || 0
        var hours = parseInt(continue_time_hour_edit.text) || 0
        var minutes = parseInt(continue_time_min_edit.text) || 0
        var seconds = parseInt(continue_time_sec_edit.text) || 0
        return days * 86400 + hours * 3600 + minutes * 60 + seconds
    }

    // 鑾峰彇闂撮殧鏃堕棿鎬荤鏁?
    function getIntervalSeconds() {
        var hours = parseInt(interval_time_hour_edit.text) || 0
        var minutes = parseInt(interval_time_min_edit.text) || 0
        var seconds = parseInt(interval_time_sec_edit.text) || 0
        return hours * 3600 + minutes * 60 + seconds
    }

    // 妫€鏌ユ槸鍚﹀～鍐欎簡鎵€鏈夊繀濉瓧娈?
    function hasAllRequiredFields() {
        // 鏍峰搧鍚嶇О涓嶈兘涓虹┖
        if (sample_name_edit.text === "") return false
        // 娴嬭瘯鑰呬笉鑳戒负绌?
        if (sampler_edit.text === "") return false
        // 鎸佺画鏃堕棿蹇呴』澶т簬0
        if (getDurationSeconds() <= 0) return false
        // 闂撮殧鏃堕棿蹇呴』澶т簬0
        if (getIntervalSeconds() <= 0) return false
        return true
    }

    // 鏍规嵁鎸佺画鏃堕棿鍜岄棿闅旀椂闂磋嚜鍔ㄨ绠楁壂鎻忔鏁?
    function calculateScanCount() {
        if (isCalculating) return
        isCalculating = true

        var duration = getDurationSeconds()
        var interval = getIntervalSeconds()

        // 鍙湁褰撴寔缁椂闂村拰闂撮殧鏃堕棿閮藉ぇ浜?鏃舵墠璁＄畻
        if (duration > 0 && interval > 0) {
            var scanCount = Math.floor(duration / interval)
            // 纭繚鑷冲皯鎵弿1娆?
            if (scanCount < 1) {
                scanCount = 1
            }
            _count_edit.text = scanCount.toString()
        }

        isCalculating = false
    }

    // 鎵弿鍖洪棿涓嬫媺妗嗘暟鎹ā鍨嬶紙bottom锛?~55锛?
    property var scanRangeBottomModel: []
    // 鎵弿鍖洪棿涓嬫媺妗嗘暟鎹ā鍨嬶紙top锛?5~0锛?
    property var scanRangeTopModel: []

    // 鍒濆鍖栨壂鎻忓尯闂存暟鎹ā鍨?
    function initScanRangeModels() {
        // bottom妯″瀷锛?~55
        scanRangeBottomModel = []
        for (var i = 0; i <= 55; i++) {
            scanRangeBottomModel.push(i)
        }
        region_bottom_combo.model = scanRangeBottomModel

        // top妯″瀷锛?5~0
        scanRangeTopModel = []
        for (var j = 55; j >= 0; j--) {
            scanRangeTopModel.push(j)
        }
        region_top_combo.model = scanRangeTopModel
    }

    // 缁勪欢鍔犺浇瀹屾垚鏃讹紝鍔犺浇淇濆瓨鐨勫弬鏁?
    Component.onCompleted: {
        var params = experiment_ctrl.loadParams(channel)
        console.log("鍔犺浇閫氶亾D鍙傛暟:", params)

        //        if (params.projectId > 0) {
        //            project_combo.currentIndex = params.projectId - 1
        //        }
        project_combo.currentIndex = -1
        sample_name_edit.text = params.sampleName || ""
        sampler_edit.text = params.operatorName || ""
        note_edit.text = params.description || ""

        continue_time_day_edit.text = params.durationDays || 0
        continue_time_hour_edit.text = params.durationHours || 0
        continue_time_min_edit.text = params.durationMinutes || 0
        continue_time_sec_edit.text = params.durationSeconds || 0

        interval_time_hour_edit.text = params.intervalHours || 0
        interval_time_min_edit.text = params.intervalMinutes || 0
        interval_time_sec_edit.text = params.intervalSeconds || 0

        _count_edit.text = params.scanCount || 0

        // 鎺ф俯鍊煎彲鑳芥潵鑷?QSettings/Variant锛屾樉寮忓綊涓€鍖栦负甯冨皵鍊硷紝閬垮厤 "false" 琚綋鎴愮湡銆?        var temperatureControlEnabled = (params.temperatureControl === true
                                         || params.temperatureControl === 1
                                         || params.temperatureControl === "true")
        temp_yes.checked = temperatureControlEnabled
        temp_no.checked = !temperatureControlEnabled
        // 鎺ф俯涓哄惁鏃讹紝鐩爣娓╁害杈撳叆妗嗕笉鍙敤
        target_temp_edit.enabled = temp_yes.checked
        target_temp_edit.text = params.targetTemperature || 0

        // 璁剧疆鎵弿鍖洪棿锛岄粯璁や负bottom=0锛宼op=55
        region_bottom_combo.currentIndex = params.scanRangeStart !== undefined ? params.scanRangeStart : 0
        // top涓嬫媺妗嗘槸鍊掑簭鐨勶紝鎵€浠ラ渶瑕佽绠楁纭殑绱㈠紩
        var topValue = params.scanRangeEnd !== undefined ? params.scanRangeEnd : 55
        region_top_combo.currentIndex = 55 - topValue

        if(params.scanStep === 20) var stepIndex = 0
        else if(params.scanStep === 40) stepIndex = 1
        else if(params.scanStep === 100) stepIndex = 2
        else if(params.scanStep === 200) stepIndex = 3
        else stepIndex = 0
        scan_step_combo.currentIndex = stepIndex
    }

    ColumnLayout {
        anchors.fill: parent
        Layout.topMargin: 10

        // 鍒濆鍖栨壂鎻忓尯闂存ā鍨嬶紙绔嬪嵆鍒濆鍖栵紝閬垮厤涓嬫媺妗嗙偣涓嶅紑锛?
        Component.onCompleted: {
            initScanRangeModels()
        }

        Item {
            Layout.fillWidth: true
            Layout.preferredHeight: 426

            Rectangle {
                anchors.fill: parent
                color: "white"
            }

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 0
                spacing: 14

                // 閫氶亾鏍囬
                Text {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 20
                    Layout.leftMargin: 50
                    Layout.topMargin: 14
                    text: qsTr("D閫氶亾")
                    font.pixelSize: 18
                    font.bold: true
                    color: "#005BAC"
                    verticalAlignment: Text.AlignVCenter
                }

                // 鍒嗛殧绾?
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 1
                    color: "#EDEEF0"
                }

                RowLayout {
                    Layout.fillWidth: true; Layout.fillHeight: true
                    Layout.leftMargin: 15; Layout.rightMargin: 15; Layout.bottomMargin: 20
                    spacing: 18

                    // 淇℃伅璁剧疆鍖哄煙
                    Rectangle {
                        Layout.fillHeight: true
                        Layout.preferredWidth: 285
                        color: "white"
                        border.color: "#DEDFE0"

                        ColumnLayout {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            spacing: 20

                            // 淇℃伅璁剧疆鏍囬
                            Label {
                                Layout.preferredWidth: 285
                                Layout.preferredHeight: 36

                                text: qsTr("淇℃伅璁剧疆")
                                font.pixelSize: 16
                                font.bold: true
                                color: "#555557"
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                                background: Rectangle {
                                    color: "#DEDFE0"
                                }
                            }

                            // 閫夋嫨宸ョ▼
                            RowLayout{
                                Layout.preferredWidth: 285
                                Layout.preferredHeight: 36
                                spacing: 8
                                Layout.leftMargin: 14

                                Label {
                                    Layout.preferredWidth: 72
                                    Layout.preferredHeight: 42
                                    text: qsTr("閫夋嫨宸ョ▼")
                                    font.pixelSize: 18
                                    font.bold: true
                                    color: "black"
                                    horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
                                }

                                ComboBox {
                                    id: project_combo
                                    Layout.preferredWidth: 172
                                    Layout.preferredHeight: 42
                                    font.pixelSize: 18
                                    model: nameList
                                    background: Rectangle {
                                        border.color: "#82C1F2"
                                        radius: 4
                                    }
                                }
                                Item { Layout.fillWidth: true }
                            }

                            // 鏍峰搧鍚嶇О锛堝繀濉級
                            RowLayout{
                                Layout.preferredWidth: 285
                                Layout.preferredHeight: 36
                                spacing: 8
                                Layout.leftMargin: 14

                                Label {
                                    Layout.preferredWidth: 72
                                    Layout.preferredHeight: 42
                                    text: qsTr("鏍峰搧鍚嶇О")
                                    font.pixelSize: 18
                                    font.bold: true
                                    color: "black"
                                    horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
                                }

                                LineEdit {
                                    id: sample_name_edit
                                    Layout.preferredWidth: 172
                                    Layout.preferredHeight: 42
                                    font.pixelSize: 18
                                    m_radius: 4
                                    border_color: "#82C1F2"
                                }
                                Item { Layout.fillWidth: true }
                            }

                            // 娴嬭瘯鑰咃紙蹇呭～锛?
                            RowLayout{
                                Layout.preferredWidth: 285
                                Layout.preferredHeight: 36
                                spacing: 8
                                Layout.leftMargin: 14

                                Label {
                                    Layout.preferredWidth: 72
                                    Layout.preferredHeight: 42
                                    text: qsTr("娴嬭瘯鑰?)
                                    font.pixelSize: 18
                                    font.bold: true
                                    color: "black"
                                    horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
                                }

                                LineEdit {
                                    id: sampler_edit
                                    Layout.preferredWidth: 172
                                    Layout.preferredHeight: 42
                                    font.pixelSize: 18
                                    m_radius: 4
                                    border_color: "#82C1F2"
                                }
                                Item { Layout.fillWidth: true }
                            }

                            // 澶囨敞锛堥€夊～锛?
                            RowLayout{
                                Layout.preferredWidth: 285
                                Layout.preferredHeight: 88
                                spacing: 8
                                Layout.leftMargin: 14

                                Label {
                                    Layout.preferredWidth: 72
                                    Layout.preferredHeight: 42
                                    text: qsTr("澶囨敞")
                                    font.pixelSize: 18
                                    font.bold: true
                                    color: "black"
                                    horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
                                }

                                LineEdit_wrap {
                                    id: note_edit
                                    Layout.preferredWidth: 172
                                    Layout.preferredHeight: 88
                                    font.pixelSize: 18
                                    m_radius: 4
                                    border_color: "#82C1F2"
                                    multiLine: true
                                    maxLines: 3
                                }
                                Item { Layout.fillWidth: true }
                            }
                        }
                    }

                    // 鏃堕棿璁剧疆鍖哄煙
                    Rectangle {
                        Layout.fillHeight: true
                        Layout.preferredWidth: 300
                        color: "white"
                        border.color: "#DEDFE0"

                        ColumnLayout {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            spacing: 16

                            // 鏃堕棿璁剧疆鏍囬
                            Label {
                                Layout.preferredWidth: 300
                                Layout.preferredHeight: 36
                                text: qsTr("鏃堕棿璁剧疆")
                                font.pixelSize: 16
                                font.bold: true
                                color: "#555557"
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                                background: Rectangle {
                                    color: "#DEDFE0"
                                }
                            }

                            // 鎸佺画鏃堕棿锛堝繀濉級
                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 8
                                Layout.leftMargin: 14

                                Label {
                                    Layout.preferredWidth: 72
                                    Layout.preferredHeight: 42
                                    text: qsTr("鎸佺画鏃堕棿")
                                    font.pixelSize: 18
                                    font.bold: true
                                    color: "black"
                                    horizontalAlignment: Text.AlignRight
                                    verticalAlignment: Text.AlignVCenter
                                }

                                LineEdit {
                                    id: continue_time_day_edit
                                    Layout.preferredWidth: 50
                                    Layout.preferredHeight: 42
                                    font.pixelSize: 18
                                    m_radius: 4
                                    border_color: "#82C1F2"
                                    horizontalAlignment: TextInput.AlignHCenter
                                    validator: IntValidator { bottom: 0; top: 999 }
                                    onTextChanged: {
                                        calculateScanCount()
                                    }
                                    inputMethodHints:Qt.ImhDigitsOnly
                                }

                                Label {
                                    Layout.preferredWidth: 35
                                    Layout.preferredHeight: 42
                                    text: "day"
                                    font.pixelSize: 16
                                    font.bold: true
                                    color: "black"
                                    verticalAlignment: Text.AlignVCenter
                                    horizontalAlignment: Text.AlignLeft
                                }

                                LineEdit {
                                    id: continue_time_hour_edit
                                    Layout.preferredWidth: 50
                                    Layout.preferredHeight: 42
                                    font.pixelSize: 18
                                    m_radius: 4
                                    border_color: "#82C1F2"
                                    horizontalAlignment: TextInput.AlignHCenter
                                    validator: IntValidator { bottom: 0; top: 23 }
                                    onTextChanged: {
                                        calculateScanCount()
                                    }
                                    inputMethodHints:Qt.ImhDigitsOnly
                                }

                                Label {
                                    Layout.preferredWidth: 15
                                    Layout.preferredHeight: 42
                                    text: "h"
                                    font.pixelSize: 16
                                    font.bold: true
                                    color: "black"
                                    verticalAlignment: Text.AlignVCenter
                                    horizontalAlignment: Text.AlignLeft
                                }

                                Item { Layout.fillWidth: true }
                            }

                            // 鎸佺画鏃堕棿锛堝垎閽熷拰绉掞級
                            RowLayout {
                                Layout.fillWidth: true
                                Layout.leftMargin: 14
                                spacing: 8

                                Item {
                                    Layout.preferredWidth: 72
                                    Layout.preferredHeight: 42
                                }

                                LineEdit {
                                    id: continue_time_min_edit
                                    Layout.preferredWidth: 50
                                    Layout.preferredHeight: 42
                                    font.pixelSize: 18
                                    m_radius: 4
                                    border_color: "#82C1F2"
                                    horizontalAlignment: TextInput.AlignHCenter
                                    validator: IntValidator { bottom: 0; top: 59 }
                                    onTextChanged: {
                                        calculateScanCount()
                                    }
                                    inputMethodHints:Qt.ImhDigitsOnly
                                }

                                Label {
                                    Layout.preferredWidth: 35
                                    Layout.preferredHeight: 42
                                    text: "min"
                                    font.pixelSize: 16
                                    font.bold: true
                                    color: "black"
                                    verticalAlignment: Text.AlignVCenter
                                    horizontalAlignment: Text.AlignLeft
                                }

                                LineEdit {
                                    id: continue_time_sec_edit
                                    Layout.preferredWidth: 50
                                    Layout.preferredHeight: 42
                                    font.pixelSize: 18
                                    m_radius: 4
                                    border_color: "#82C1F2"
                                    horizontalAlignment: TextInput.AlignHCenter
                                    validator: IntValidator { bottom: 0; top: 59 }
                                    onTextChanged: {
                                        calculateScanCount()
                                    }
                                    inputMethodHints:Qt.ImhDigitsOnly
                                }

                                Label {
                                    Layout.preferredWidth: 15
                                    Layout.preferredHeight: 42
                                    text: "s"
                                    font.pixelSize: 16
                                    font.bold: true
                                    color: "black"
                                    verticalAlignment: Text.AlignVCenter
                                    horizontalAlignment: Text.AlignLeft
                                }

                                Item { Layout.fillWidth: true }
                            }

                            // 闂撮殧鏃堕棿锛堝繀濉級
                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 8
                                Layout.leftMargin: 14

                                Label {
                                    Layout.preferredWidth: 72
                                    Layout.preferredHeight: 42
                                    text: qsTr("闂撮殧鏃堕棿")
                                    font.pixelSize: 18
                                    font.bold: true
                                    color: "black"
                                    horizontalAlignment: Text.AlignRight
                                    verticalAlignment: Text.AlignVCenter
                                }

                                LineEdit {
                                    id: interval_time_hour_edit
                                    Layout.preferredWidth: 50
                                    Layout.preferredHeight: 42
                                    font.pixelSize: 18
                                    m_radius: 4
                                    border_color: "#82C1F2"
                                    horizontalAlignment: TextInput.AlignHCenter
                                    validator: IntValidator { bottom: 0; top: 999 }
                                    onTextChanged: {
                                        calculateScanCount()
                                    }
                                    inputMethodHints:Qt.ImhDigitsOnly
                                }

                                Label {
                                    Layout.preferredWidth: 15
                                    Layout.preferredHeight: 42
                                    text: "h"
                                    font.pixelSize: 16
                                    font.bold: true
                                    color: "black"
                                    verticalAlignment: Text.AlignVCenter
                                    horizontalAlignment: Text.AlignLeft
                                }

                                LineEdit {
                                    id: interval_time_min_edit
                                    Layout.preferredWidth: 50
                                    Layout.preferredHeight: 42
                                    font.pixelSize: 18
                                    m_radius: 4
                                    border_color: "#82C1F2"
                                    horizontalAlignment: TextInput.AlignHCenter
                                    validator: IntValidator { bottom: 0; top: 23 }
                                    onTextChanged: {
                                        calculateScanCount()
                                    }
                                    inputMethodHints:Qt.ImhDigitsOnly
                                }

                                Label {
                                    Layout.preferredWidth: 35
                                    Layout.preferredHeight: 42
                                    text: "min"
                                    font.pixelSize: 16
                                    font.bold: true
                                    color: "black"
                                    verticalAlignment: Text.AlignVCenter
                                    horizontalAlignment: Text.AlignLeft
                                }

                                Item { Layout.fillWidth: true }
                            }

                            // 闂撮殧鏃堕棿锛堢锛?
                            RowLayout {
                                Layout.fillWidth: true
                                Layout.leftMargin: 14
                                spacing: 8

                                Item {
                                    Layout.preferredWidth: 72
                                    Layout.preferredHeight: 42
                                }

                                LineEdit {
                                    id: interval_time_sec_edit
                                    Layout.preferredWidth: 50
                                    Layout.preferredHeight: 42
                                    font.pixelSize: 18
                                    m_radius: 4
                                    border_color: "#82C1F2"
                                    horizontalAlignment: TextInput.AlignHCenter
                                    validator: IntValidator { bottom: 0; top: 59 }
                                    onTextChanged: {
                                        calculateScanCount()
                                    }
                                    inputMethodHints:Qt.ImhDigitsOnly
                                }

                                Label {
                                    Layout.preferredWidth: 15
                                    Layout.preferredHeight: 42
                                    text: "s"
                                    font.pixelSize: 16
                                    font.bold: true
                                    color: "black"
                                    verticalAlignment: Text.AlignVCenter
                                    horizontalAlignment: Text.AlignLeft
                                }

                                Item { Layout.fillWidth: true }
                            }

                            // 鎵弿娆℃暟锛堣嚜鍔ㄨ绠楋級
                            RowLayout{
                                Layout.preferredWidth: 285
                                Layout.preferredHeight: 36
                                spacing: 8
                                Layout.leftMargin: 14

                                Label {
                                    Layout.preferredWidth: 72
                                    Layout.preferredHeight: 42
                                    text: qsTr("鎵弿娆℃暟")
                                    font.pixelSize: 18
                                    font.bold: true
                                    color: "black"
                                    horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
                                }

                                LineEdit {
                                    id: _count_edit
                                    Layout.preferredWidth: 138
                                    Layout.preferredHeight: 42
                                    font.pixelSize: 18
                                    m_radius: 4
                                    border_color: "#82C1F2"
                                    readOnly: true
                                }
                                Item { Layout.fillWidth: true }
                            }
                        }
                    }

                    // 鍙傛暟璁剧疆鍖哄煙
                    Rectangle {
                        Layout.fillHeight: true
                        Layout.preferredWidth: 320
                        color: "white"
                        border.color: "#DEDFE0"

                        ColumnLayout {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            spacing: 20

                            // 鍙傛暟璁剧疆鏍囬
                            Label {
                                Layout.preferredWidth: 320
                                Layout.preferredHeight: 36

                                text: qsTr("鍙傛暟璁剧疆")
                                font.pixelSize: 16
                                font.bold: true
                                color: "#555557"
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                                background: Rectangle {
                                    color: "#DEDFE0"
                                }
                            }

                            // 鎺ф俯锛堥粯璁や负鍚︼級
                            RowLayout{
                                Layout.preferredWidth: 285
                                Layout.preferredHeight: 36
                                spacing: 8
                                Layout.leftMargin: 14

                                Label {
                                    Layout.preferredWidth: 72
                                    Layout.preferredHeight: 42
                                    text: qsTr("鎺ф俯")
                                    font.pixelSize: 18
                                    font.bold: true
                                    color: "black"
                                    horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
                                }

                                RadioButton {
                                    id: temp_yes
                                    text: qsTr("鏄?)
                                    Layout.alignment: Qt.AlignVCenter
                                    Layout.preferredHeight: 42
                                    font.pixelSize: 16
                                    onCheckedChanged: {
                                        // 鎺ф俯涓烘槸鏃讹紝鐩爣娓╁害杈撳叆妗嗗彲鐢?
                                        target_temp_edit.enabled = checked
                                    }
                                }

                                RadioButton {
                                    id: temp_no
                                    text: qsTr("鍚?)
                                    Layout.alignment: Qt.AlignVCenter
                                    Layout.preferredHeight: 42
                                    font.pixelSize: 16
                                    checked: true
                                }
                                Item { Layout.fillWidth: true }
                            }

                            // 鐩爣娓╁害
                            RowLayout{
                                Layout.preferredWidth: 285
                                Layout.preferredHeight: 36
                                spacing: 8
                                Layout.leftMargin: 14

                                Label {
                                    Layout.preferredWidth: 72
                                    Layout.preferredHeight: 42
                                    text: qsTr("鐩爣娓╁害")
                                    font.pixelSize: 18
                                    font.bold: true
                                    color: "black"
                                    horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
                                }

                                LineEdit {
                                    id: target_temp_edit
                                    Layout.preferredWidth: 168
                                    Layout.preferredHeight: 42
                                    font.pixelSize: 18
                                    m_radius: 4
                                    border_color: "#82C1F2"
                                    enabled: false
                                    inputMethodHints:Qt.ImhDigitsOnly
                                }

                                Label {
                                    Layout.preferredWidth: 20
                                    Layout.preferredHeight: 42
                                    text: "鈩?
                                    font.pixelSize: 18
                                    font.bold: true
                                    color: "black"
                                    horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
                                }
                                Item { Layout.fillWidth: true }
                            }

                            // 鎵弿鍖洪棿
                            RowLayout{
                                Layout.preferredWidth: 285
                                Layout.preferredHeight: 36
                                spacing: 8
                                Layout.leftMargin: 14

                                Label {
                                    Layout.preferredWidth: 72
                                    Layout.preferredHeight: 42
                                    text: qsTr("鎵弿鍖洪棿")
                                    font.pixelSize: 18
                                    font.bold: true
                                    color: "black"
                                    horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
                                }

                                ComboBox {
                                    id: region_bottom_combo
                                    Layout.preferredWidth: 72
                                    Layout.preferredHeight: 42
                                    font.pixelSize: 18
                                    model: scanRangeBottomModel
                                    background: Rectangle {
                                        border.color: "#82C1F2"
                                        radius: 4
                                    }
                                }

                                Label {
                                    Layout.preferredWidth: 10
                                    Layout.preferredHeight: 42
                                    text: "~"
                                    font.pixelSize: 18
                                    font.bold: true
                                    color: "black"
                                    horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
                                }

                                ComboBox {
                                    id: region_top_combo
                                    Layout.preferredWidth: 72
                                    Layout.preferredHeight: 42
                                    font.pixelSize: 18
                                    model: scanRangeTopModel
                                    background: Rectangle {
                                        border.color: "#82C1F2"
                                        radius: 4
                                    }
                                }

                                Label {
                                    Layout.preferredWidth: 20
                                    Layout.preferredHeight: 42
                                    text: "mm"
                                    font.pixelSize: 18
                                    font.bold: true
                                    color: "black"
                                    horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
                                }
                                Item { Layout.fillWidth: true }
                            }

                            // 鎵弿闂撮殧锛堝繀濉級
                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 8
                                Layout.leftMargin: 14

                                Label {
                                    Layout.preferredWidth: 72
                                    Layout.preferredHeight: 42
                                    text: qsTr("鎵弿闂撮殧")
                                    font.pixelSize: 18
                                    font.bold: true
                                    color: "black"
                                    horizontalAlignment: Text.AlignRight
                                    verticalAlignment: Text.AlignVCenter
                                }

                                ComboBox {
                                    id: scan_step_combo
                                    Layout.preferredWidth: 168
                                    Layout.preferredHeight: 42
                                    font.pixelSize: 18
                                    model: ["20","40","50","100","200"]
                                    background: Rectangle {
                                        border.color: "#82C1F2"
                                        radius: 4
                                    }
                                }

                                Label {
                                    Layout.preferredWidth: 20
                                    Layout.preferredHeight: 42
                                    text: "um"
                                    font.pixelSize: 18
                                    font.bold: true
                                    color: "black"
                                    horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
                                }

                                Item { Layout.fillWidth: true }
                            }
                        }
                    }
                }
            }
        }

        // 搴旂敤鎸夐挳
        IconButton {
            button_text: qsTr("搴?  鐢?)
            Layout.preferredWidth: 160
            Layout.preferredHeight: 45
            Layout.alignment: Qt.AlignHCenter
            Layout.topMargin: 10
            button_color: "#3B87E4"
            text_color: "#FFFFFF"
            pixelSize: 18

            onClicked: {
                console.log("搴旂敤閫氶亾D鍙傛暟璁剧疆")

                // 楠岃瘉锛氭牱鍝佸悕绉板繀椤诲～鍐?
                if (project_combo.currentIndex < 0) {
                    info_pop.openDialog("璇烽€夋嫨宸ョ▼锛岃嫢鏆傛棤宸ョ▼璇峰厛娣诲姞宸ョ▼")
                    return
                }
                if (sample_name_edit.text === "") {
                    info_pop.openDialog("璇峰～鍐欐牱鍝佸悕绉?)
                    return
                }

                // 楠岃瘉锛氭祴璇曡€呭繀椤诲～鍐?
                if (sampler_edit.text === "") {
                    info_pop.openDialog("璇峰～鍐欐祴璇曡€?)
                    return
                }

                // 楠岃瘉锛氭寔缁椂闂村繀椤诲～鍐?
                if (getDurationSeconds() <= 0) {
                    info_pop.openDialog("璇峰～鍐欐寔缁椂闂?)
                    return
                }

                // 楠岃瘉锛氶棿闅旀椂闂村繀椤诲～鍐?
                if (getIntervalSeconds() <= 0) {
                    info_pop.openDialog("璇峰～鍐欓棿闅旀椂闂?)
                    return
                }

                // 楠岃瘉锛氭壂鎻忓尯闂翠笂闄愬繀椤诲ぇ浜庝笅闄?
                var rangeStart = region_bottom_combo.currentIndex
                var rangeEnd = scanRangeTopModel[region_top_combo.currentIndex]
                if (rangeEnd <= rangeStart) {
                    info_pop.openDialog("鎵弿鍖洪棿涓婇檺蹇呴』澶т簬涓嬮檺")
                    return
                }

                // 鏋勫缓鍙傛暟瀵硅薄
                var params = {
                    projectId: project_combo.currentIndex + 1,
                    sampleName: sample_name_edit.text,
                    operatorName: sampler_edit.text,
                    description: note_edit.text,
                    durationDays: parseInt(continue_time_day_edit.text) || 0,
                    durationHours: parseInt(continue_time_hour_edit.text) || 0,
                    durationMinutes: parseInt(continue_time_min_edit.text) || 0,
                    durationSeconds: parseInt(continue_time_sec_edit.text) || 0,
                    intervalHours: parseInt(interval_time_hour_edit.text) || 0,
                    intervalMinutes: parseInt(interval_time_min_edit.text) || 0,
                    intervalSeconds: parseInt(interval_time_sec_edit.text) || 0,
                    scanCount: parseInt(_count_edit.text) || 0,
                    temperatureControl: temp_yes.checked,
                    targetTemperature: parseFloat(target_temp_edit.text) || 0,
                    scanRangeStart: rangeStart,
                    scanRangeEnd: rangeEnd,
                    scanStep: scan_step_combo.currentText
                }

                console.log("淇濆瓨鍙傛暟:", params)
                experiment_ctrl.saveParams(channel, params)

                if(mainStackView.currentItem.objectName === "ParaSetting_D")
                {
                    mainStackView.pop()
                    select_channel_pop.open()
                }
            }
        }
    }
}

