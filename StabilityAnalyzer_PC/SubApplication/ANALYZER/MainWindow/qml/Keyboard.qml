import QtQuick 2.12
import QtQuick.VirtualKeyboard 2.1
import QtQuick.Window 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import QtQuick.Controls.Material 2.12
import QtQuick.VirtualKeyboard.Settings 2.1

// 虚拟键盘，满足存在window顶层对象和root根对象
//editing

InputPanel {
    z: 9999
    x: 0
    y: window.height
    anchors.left: parent.left
    anchors.right: parent.right
    parent: Overlay.overlay

    property var vkb_this: this

    // last focus 对象
    property  var lase_focus_obj : null

    property var window: window
    property var root: root

    property var accepted: lase_focus_obj ? lase_focus_obj.acceptableInput : null

    property int inputX: 0
    property int inputY: 0
    property int inputWidth: 0
    property int inputHeight: 0

    visible: false
    externalLanguageSwitchEnabled: false

    // 这个配置可以放到输入框内部，在输入框加载完成后配置，特别是针对全局键盘的情况
    Component.onCompleted: {
        // 允许的语言，重要！
        VirtualKeyboardSettings.activeLocales = ["en_GB","zh_CN"]
        // 默认的语言，重要！
        VirtualKeyboardSettings.locale = "en_GB"
    }

    onVisibleChanged: {
        if(visible){
            animation_keyboard_show.start()
            //y = window.height - vkb_this.height

            if((window.height - height) < (inputY + 75)){
                animation_root_show.start()
                //root.y = -(inputY - (window.height - vkb_this.height)) - inputHeight
            }

            // lase_focus_obj.editing = false ;
        }else{
            if(accepted){
                lase_focus_obj = true ;
            }
        }
    }

    // /*
    // Component.onCompleted: {
    //     //VirtualKeyboardSettings.locale = "eesti" // 复古键盘样式
    //     VirtualKeyboardSettings.wordCandidateList.alwaysVisible = true
    // }
    // */


    // 这种集成方式下点击隐藏键盘的按钮是没有效果的，只会改变active，因此我们自己处理一下
    onActiveChanged: {
        if(!active) {
            animation_root_hind.start()
            animation_keyboard_hind.start()

            //root.y = 0
            //y = window.height
            //lase_focus_obj.focus = false
            //visible = false
        }
    }


    NumberAnimation {
        id: animation_keyboard_show
        target: vkb_this // 动画的目标对象
        property: "y" // 要动画的属性
        duration: 200 // 动画持续时间（毫秒）
        from: window.height
        to:window.height - vkb_this.height
        easing.type: Easing.OutQuad // 缓动类型（可选）
    }
    NumberAnimation {
        id: animation_root_show
        target: root // 动画的目标对象
        property: "y" // 要动画的属性
        duration: 200 // 动画持续时间（毫秒）
        from: 0
        to: -(inputY - (window.height - vkb_this.height)) - inputHeight - 30
        easing.type: Easing.OutQuad // 缓动类型（可选）
    }

    NumberAnimation {
        id: animation_keyboard_hind
        target: vkb_this // 动画的目标对象
        property: "y" // 要动画的属性
        duration: 200 // 动画持续时间（毫秒）
        from: y
        to: window.height
        easing.type: Easing.OutQuad // 缓动类型（可选）

        onStopped : {
            if(lase_focus_obj){
                lase_focus_obj.focus = false
            }
            visible = false
        }
    }

    NumberAnimation {
        id: animation_root_hind
        target: root // 动画的目标对象
        property: "y" // 要动画的属性
        duration: 200 // 动画持续时间（毫秒）
        from: root.y
        to: 0
        easing.type: Easing.OutQuad // 缓动类型（可选）
    }
}
