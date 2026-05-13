import QtQuick 2.12
import QtQuick.Layouts 1.12
import QtQuick.Controls 2.12


Item {
    id: instruction
    width: 972
    height: 501

    objectName: "Instrction"

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // --- 顶部标题栏 ---
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 30
            Layout.margins: 15

            color: "transparent"

            RowLayout {
                anchors.fill: parent
                spacing: 10

                Label {
                    Layout.leftMargin: 50
                    text: qsTr("操作说明书")
                    font.pixelSize: 20
                    font.bold: true
                    color: "#005BAC"
                    verticalAlignment: Text.AlignVCenter
                }
            }

            // 底部蓝线
            Rectangle {
                anchors.bottom: parent.bottom
                width: parent.width
                height: 2
                color: "#005BAC"
            }
        }

        // --- 显示区域 ---
        Rectangle {
            id: instructions
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: "#333333" // 深灰背景，衬托白色文档
            radius: 4

            Column {
                anchors.fill: parent

                // 2. 核心修改区域：直接使用 ListView
                ListView {
                    id: listView
                    width: parent.width
                    height: parent.height - 40 - 10 // 减去 Row 高度和 spacing
                    clip: true

                    // 数据源：28页
                    model: 25

                    // 间距：对应原先 Column 的 spacing
                    spacing: 50

                    // 性能优化关键属性
                    cacheBuffer: 1000       // 预加载上下 1000 像素区域的内容，保证滑动流畅
                    delegate: Image {
                        width: listView.width
                        height: width * 1.414
                        fillMode: Image.PreserveAspectFit

                        // TODO:图片实际路径
                        source: "qrc:/instructions/output_images/page_" + (index + 1) + ".png"

                        // 进一步节省内存：如果图片太大，可以限制源尺寸
                        sourceSize.width: width

                        asynchronous: true

                    }
                    //当完成加载后默认在 0 的位置
                    Component.onCompleted: {
                        listView.positionViewAtIndex(0, ListView.Beginning)
                    }
                }
            }
        }
    }
}
