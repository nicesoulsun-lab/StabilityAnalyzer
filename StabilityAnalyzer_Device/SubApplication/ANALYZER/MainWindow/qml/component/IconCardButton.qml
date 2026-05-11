import QtQuick 2.9
import QtQuick.Layouts 1.3

// 鏍瑰厓绱犱娇鐢?Item锛屽洜涓烘垜浠渶瑕佸畠浣滀负涓€涓€忔槑鐨勫鍣紝鍐呴儴鍖呭惈 Image 鍜?MouseArea
Item {
    id: root

    // =========================
    // 瀵瑰鏆撮湶鐨勫睘鎬?(Properties)
    // =========================
    
    // 鎸夐挳灏哄
    property int btnWidth: 170
    property int btnHeight: 90
    
    // 鍥剧墖璧勬簮
    property url iconSource: ""
    
    // 鏄惁鍚敤鎮仠鏁堟灉 (鍙€?
    property bool enableHover: true

    // =========================
    // 瀵瑰鏆撮湶鐨勪俊鍙?(Signals)
    // =========================
    
    // 瀹氫箟涓€涓偣鍑讳俊鍙凤紝渚涘閮ㄨ繛鎺?
    signal clicked()

    // =========================
    // 鍐呴儴瀹炵幇
    // =========================

    // 1. 搴旂敤灏哄
    width: btnWidth
    height: btnHeight

    // 2. 鐐瑰嚮缂╂斁鍔ㄧ敾閫昏緫
    // 缁戝畾鍒板唴閮?mouseArea 鐨?pressed 鐘舵€?
    scale: mouseArea.pressed ? 0.92 : 1.0
    transformOrigin: Item.Center // 鍏抽敭锛氫互涓績涓鸿酱缂╂斁

    Behavior on scale {
        NumberAnimation {
            duration: 100
            easing.type: Easing.OutQuad
        }
    }

    // 3. 鑳屾櫙/鍥炬爣鍥剧墖
    Image {
        anchors.fill: parent
        source: root.iconSource
        fillMode: Image.PreserveAspectFit
        
        // 鍙€夛細鐐瑰嚮鏃剁◢寰彉鏆楋紝澧炲姞璐ㄦ劅
        opacity: mouseArea.pressed ? 0.85 : 1.0
        Behavior on opacity {
            NumberAnimation { duration: 100 }
        }
    }

    // 4. 榧犳爣浜や簰鍖哄煙
    MouseArea {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: root.enableHover
        cursorShape: Qt.PointingHandCursor

        onClicked: {
            // 瑙﹀彂瀵瑰鏆撮湶鐨勪俊鍙?
            root.clicked()
        }
    }
}
