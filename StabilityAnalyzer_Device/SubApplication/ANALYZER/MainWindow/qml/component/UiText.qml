import QtQuick 2.12


Text {
    property int pixelSize : 16
    property bool textBold : false
    property string text_color : "#000000"
    //property string family : notoSansSCRegular.name
    width: 120
    font.pixelSize: pixelSize
    font.bold: textBold
    color: text_color
    //font.family: family
    horizontalAlignment: Text.AlignHCenter
    verticalAlignment: Text.AlignVCenter
}
