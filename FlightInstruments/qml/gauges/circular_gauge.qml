import QtQuick
import QtQuick.Shapes

Item {
  id: root

  // ==================== Core Properties ====================
  property real value: 0
  property real minimumValue: 0
  property real maximumValue: 100
  property real stepSize: 1.0

  property var style: CircularGaugeStyle
  {
  }

  Component.onCompleted: {
    style.parentGauge = Qt.binding(() => root)
  }

  // ==================== Background ====================
  Loader {
    anchors.fill: parent
    sourceComponent: style.background
  }

  // ==================== Progress Arc ====================
  Shape {
    anchors.fill: parent
    antialiasing: true
    visible: style.progressArcThickness > 0

    ShapePath {
      strokeColor: style.progressColor
      strokeWidth: style.progressArcThickness
      fillColor: "transparent"
      capStyle: ShapePath.FlatCap

      PathAngleArc {
        centerX: width / 2
        centerY: height / 2
        radiusX: style.outerRadius - 10
        radiusY: style.outerRadius - 10
        startAngle: style.valueToAngle(minimumValue)
        sweepAngle: style.valueToAngle(root.value) - startAngle
      }
    }
  }

  // ==================== Minor Tick Marks ====================
  Repeater {
    model: style.minorTickmarkCount > 0
        ? Math.floor((maximumValue - minimumValue) / style.tickmarkStepSize * style.minorTickmarkCount) + 1
        : 0

    Item {
      property real tickValue: root.minimumValue + index * (style.tickmarkStepSize / style.minorTickmarkCount)
      property real angle: style.valueToAngle(tickValue)

      anchors.centerIn: root
      property real tick_x_position: (style.outerRadius - 10) * Math.cos(Math.PI / 180 * angle)
      property real tick_y_position: (style.outerRadius - 10) * Math.sin(Math.PI / 180 * angle)

      Loader {
        sourceComponent: style.minorTickmark

        onLoaded: {
          if (item && item.hasOwnProperty("value")) {
            item.value = parent.tickValue
          }
        }

        rotation: parent.angle + 90
        x: parent.tick_x_position - width * 0.5
        y: parent.tick_y_position - height * 0.5
      }

      // Fallback, if minor Tick Mark not defined
      Rectangle {
        visible: !style.minorTickmark && (index % style.minorTickmarkCount !== 0)
        width: style.minorTickmarkWidth
        height: style.minorTickmarkLength
        color: style.minorTickmarkColor
        anchors.horizontalCenter: parent.horizontalCenter
        y: -style.outerRadius + style.minorTickmarkInset
        transformOrigin: Item.Bottom
        rotation: parent.angle
      }
    }
  }

  // ==================== Major Tick Marks ====================
  Repeater {
    model: Math.floor((maximumValue - minimumValue) / style.tickmarkStepSize) + 1

    Item {
      anchors.centerIn: root
      property real tickValue: root.minimumValue + index * style.tickmarkStepSize
      property real angle: style.valueToAngle(tickValue)
      property real tick_x_position: (style.outerRadius - 10) * Math.cos(Math.PI / 180 * angle)
      property real tick_y_position: (style.outerRadius - 10) * Math.sin(Math.PI / 180 * angle)

      Loader {
        sourceComponent: style.tickmark
        rotation: parent.angle + 90
        x: parent.tick_x_position - width * 0.5
        y: parent.tick_y_position - height * 0.5
        onLoaded: {
          if (item && item.hasOwnProperty("value")) {
            item.value = parent.tickValue
          }
        }
      }
    }
  }

  // ==================== Major Tick Mark Labels ====================
  Repeater {
    model: Math.floor((maximumValue - minimumValue) / style.labelStepSize) + 1

    Item {
      id: labelContainer

      property real labelValue: root.minimumValue + index * style.labelStepSize
      property real angleDeg: style.valueToAngle(labelValue)
      property real labelRadius: style.outerRadius - style.labelInset
      property real xPos: root.width / 2 + 1.03 * labelRadius * Math.cos(angleDeg * Math.PI / 180)
      property real yPos: root.height / 2 + 1.03 * labelRadius * Math.sin(angleDeg * Math.PI / 180)

      Loader {
        id: tickLabelLoader
        sourceComponent: style.tickmarkLabel

        onLoaded: {
          if (!item)
            return
          if (item.hasOwnProperty("value"))
            item.value = labelContainer.labelValue
          if (item.font)
            item.font.pixelSize = Qt.binding(function () {
              return style.tickmarkLabelPixelSize
            })
        }

        x: labelContainer.xPos - width / 2
        y: labelContainer.yPos - height / 2
        rotation: 0
      }
    }
  }

  // ==================== Needle ====================
  Item {
    anchors.centerIn: parent
    rotation: 90.0 + style.valueToAngle(root.value)

    Loader {
      sourceComponent: style.needle
      anchors.horizontalCenter: parent.horizontalCenter
      anchors.bottom: parent.verticalCenter
    }

    // Fallback, if no needle style is defined
    Rectangle {
      visible: !style.needle
      width: style.needleWidth
      height: style.needleLength * style.outerRadius
      color: style.needleColor
      radius: 2
      anchors.horizontalCenter: parent.horizontalCenter
      y: -style.needleLength * style.outerRadius + 12
    }

    Rectangle {  // Hub
      width: 26
      height: 26
      radius: 13
      color: "#1e1e1e"
      border.color: style.needleColor
      border.width: 3
      anchors.centerIn: parent
    }
  }

  // Foreground
  Loader {
    anchors.fill: parent
    sourceComponent: style.foreground
  }
}