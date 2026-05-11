import QtQuick 2.9
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3

ApplicationWindow {
    id: mainApplicationWindow

    width: 1024
    height: 600
    minimumWidth: 1024
    minimumHeight: 600
    maximumWidth: 1024
    maximumHeight: 600
    title: "绋冲畾鎬у垎鏋愪华"
    visible: true

    property var mainStackView: null

    property alias rootRectangle: root

    Rectangle{
        id:root
        width: parent.width; height: parent.height
        color: "transparent"

        // 宓屽瀛愰」鐩殑鐣岄潰
        Loader {
            id: main_loader
            anchors.fill: parent
            source: "qrc:/qml/MainWindow.qml"

            // 2. 鍏抽敭锛氬湪鍔犺浇瀹屾垚鍚庯紝灏嗗唴閮ㄧ殑瀵硅薄璧嬪€肩粰澶栭儴鍙橀噺
            onLoaded: {
                console.log("瀛愰」鐩晫闈㈠姞杞藉畬鎴?)
                if (item) {
                    mainStackView = item.mainStackView;
                    console.log("MainStackView 宸茬粦瀹?", mainStackView);
                }
            }

            onStatusChanged: {
                if (status === Loader.Error) {
                    console.log("鍔犺浇瀛愰」鐩晫闈㈠け璐?", source)
                }
            }
        }
    }

    InfoPop{
        id: info_pop
    }

    NewProjectPop{
        id: new_project_pop
    }

    SelectChannelsPop{
        id:select_channel_pop
    }

    CustomPop{
        id: custom_pop
    }

//    Keyboard{
//        id: keyboard
//        window: mainApplicationWindow
//        root: root
//    }
}

