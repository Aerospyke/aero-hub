import QtQuick 2.15

CircularGaugeStyle {
  id: style
  minimumValueAngle: 135
  maximumValueAngle: 45
  tickmarkStepSize: 5
  labelStepSize: 10
  minorTickmarkCount: 0
  tickmarkHeight: 0.15 * outerRadius
  labelInset: 0.275 * outerRadius
  tickmarkInset: 0.02 * outerRadius

  tickmark: Rectangle {
    property real value: 0
    color: value === 0 ? "#e30000" : "#ffffff"

    width: 0.02 * style.outerRadius
    height: style.tickmarkHeight
    radius: 0.01 * style.outerRadius
    antialiasing: true
    visible: true
  }

  tickmarkLabel: Component {
    Text {
      property real value: 0
      text: value
      color: "#ffffff"
      font.family: "Century Gothic"
      antialiasing: true
      visible: true
    }
  }

  // foreground: Item {
  // }

  needle: GaugeNeedleStandard {
    width: 0.075 * style.outerRadius
    height: 0.95 * style.outerRadius
  }

  background: Item {
    id: background
    width: 2 * style.outerRadius
    height: 2 * style.outerRadius

    GaugeCanvas {
      anchors.fill: parent
      onPaint: {
        if (context) {
          context.reset()
          context.lineWidth = 0.045 * outerRadius
          context.beginPath()
          context.arc(outerRadius,
              outerRadius,
              outerRadius - style.tickmarkHeight / 2 + context.lineWidth,
              (valueToAngle(maximumValue)) * Math.PI / 180.0,
              (valueToAngle(minimumValue)) * Math.PI / 180.0, style.positiveDirectionIsClockwise)
          context.strokeStyle = "#ffffff"
          context.stroke()
        }
      }
    }
  }
}