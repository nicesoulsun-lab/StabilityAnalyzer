import QtQuick 2.12
import QtQuick.Controls 2.12

Slider {
      id: brightness_slider
      width: 400
      height: 40
      from: 0
      to: 9999
      value: 50

      background: Rectangle {
          x: brightness_slider.leftPadding
          y: brightness_slider.topPadding + brightness_slider.availableHeight / 2 - height / 2
          implicitWidth: 200
          implicitHeight: 4 // Thin track height
          width: brightness_slider.availableWidth
          height: implicitHeight
          radius: 2
          color: "#E0E0E0" // Light gray for the inactive part (right side)

          Rectangle {
              width: brightness_slider.visualPosition * parent.width
              height: parent.height
              color: "#2196F3" // Blue color matches the handle border
              radius: 2
          }
      }

      handle: Rectangle {
          x: brightness_slider.leftPadding + brightness_slider.visualPosition * (brightness_slider.availableWidth - width)
          y: brightness_slider.topPadding + brightness_slider.availableHeight / 2 - height / 2
          implicitWidth: 24
          implicitHeight: 24
          radius: 12
          color: "#FFFFFF" // White center
          border.color: "#2196F3" // Blue border
          border.width: 2
      }
  }

