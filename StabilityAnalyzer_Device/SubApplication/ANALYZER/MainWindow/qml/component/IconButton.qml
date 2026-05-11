import QtQuick 2.9
import QtQuick.Controls 2.2

Button {
    id: root

    // =========================
    // 鑷畾涔夊睘鎬?(淇濇寔涓嶅彉)
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
    property string family: "Microsoft YaHei" // 寤鸿缁欎釜榛樿瀛椾綋锛岄槻鎶ラ敊

    property bool vertical: false
    property color border_color: "transparent"
    property int border_width: 0

    // ============================================================
    // 銆愭牳蹇冧慨澶嶃€?鍔ㄧ敾閫昏緫閲嶅啓
    // ============================================================

    // 1. 缁戝畾缂╂斁姣斾緥鍒版寜閽殑鎸変笅鐘舵€?(down)
    //    down 鏄?Button 鐨勫師鐢熷睘鎬э細鎸変笅涓?true锛屾澗寮€涓?false
    //    鎸変笅鏃跺彉灏?(0.92)锛屾澗寮€鎭㈠ (1.0)
    scale: root.down ? 0.92 : 1.0

    // 2. 浣跨敤 Behavior 鑷姩骞虫粦杩囨浮
    //    鏃犺浣犵偣鍑诲蹇紝Qt 閮戒細鑷姩澶勭悊鏁板€肩殑杩囨浮锛屼笉浼氬崱椤?
    Behavior on scale {
        NumberAnimation {
            duration: 100
            easing.type: Easing.OutQuad
        }
    }

    // (绉婚櫎浜嗗師鏉ョ殑 transform: Scale 鍜?SequentialAnimation 浠ｇ爜)

    // =========================
    // 鑳屾櫙鏍峰紡
    // =========================
    background: Rectangle {
        anchors.fill: parent
        radius: root.button_radius

        // 鑷姩澶勭悊棰滆壊鍙樺寲锛氭寜涓嬪彉娣憋紝鎮仠鍙樻祬锛岄粯璁ょ敤 button_color
        color: root.down ? Qt.darker(root.button_color, 1.1) :
                           (root.hovered ? Qt.lighter(root.button_color, 1.05) : root.button_color)

        clip: true
        border.color: root.border_color
        border.width: root.border_width

        // 鑳屾櫙鍥剧墖鏀寔
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
    // 鍐呭甯冨眬 (Loader 鍔ㄦ€佸姞杞芥í/绔栧竷灞€)
    // =========================
    contentItem: Item {
        anchors.fill: parent

        Loader {
            id: layoutLoader
            anchors.centerIn: parent
            sourceComponent: root.vertical ? columnComponent : rowComponent
        }
    }
    // --- 妯悜甯冨眬缁勪欢 (鍥炬爣鍦ㄥ乏锛屾枃瀛楀湪鍙? ---
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
                anchors.verticalCenter: parent.verticalCenter // 纭繚鍥炬爣涔熷瀭鐩村眳涓?
            }

            Text {
                id: labelText_Row
                text: root.button_text
                font.pixelSize: root.pixelSize
                color: root.text_color
                font.family: root.family
                anchors.verticalCenter: parent.verticalCenter

                // --- 鎹㈣鏍稿績閫昏緫 ---
                // 璁＄畻 8 涓瓧绗︾殑澶ц嚧瀹藉害闄愬埗
                width: text.length > 8 ? font.pixelSize * 8 : undefined
                wrapMode: Text.WrapAnywhere
                horizontalAlignment: Text.AlignHCenter
            }
        }
    }

    // --- 绾靛悜甯冨眬缁勪欢 (鍥炬爣鍦ㄤ笂锛屾枃瀛楀湪涓? ---
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

                // --- 鎹㈣鏍稿績閫昏緫 ---
                // 绾靛悜甯冨眬涓嬶紝鎹㈣閫氬父闇€瑕佹洿鏄庣‘鐨勫榻?
                width: text.length > 8 ? font.pixelSize * 8 : undefined
                wrapMode: Text.WrapAnywhere
                horizontalAlignment: Text.AlignHCenter
            }
        }
    }
}

