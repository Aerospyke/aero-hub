import QtQuick 2.15

CircularGauge {
  id: pressure_gauge_root
  minimumValue: 0
  maximumValue: 115
  stepSize: 0.1
  value: 0

  style: CircularGaugeStyle {
    id: style
    minimumValueAngle: 135
    maximumValueAngle: 225
    tickmarkStepSize: 5
    labelStepSize: 5
    minorTickmarkCount: 0
    tickmarkHeight: 0.115 * style.outerRadius
    labelInset: 0.2 * style.outerRadius
    tickmarkInset: 0.02 * style.outerRadius

    tickmark: Rectangle {
      property real value: 0

      color: value === 115 || value === 20 ? "#e30000" : "#ffffff"
      width: 0.03 * style.outerRadius
      height: style.tickmarkHeight
      radius: 0.01 * style.outerRadius
      antialiasing: true
      visible: value === 0 ||
          value === 20 ||
          value === 40 ||
          value === 60 ||
          value === 80 ||
          value === 100 ||
          value === 115
    }

    tickmarkLabel: Component {
      Text {
        property real value: 0

        text: value
        color: "#ffffff"
        font.family: "Century Gothic"
        font.weight: Font.Black
        antialiasing: true
        visible: value === 0 ||
            value === 20 ||
            value === 60 ||
            value === 100 ||
            value === 115
      }
    }

    needle: GaugeNeedleStandard {
      width: 0.075 * style.outerRadius
      height: 0.95 * style.outerRadius
    }

    background: Item {
      id: background

      GaugeCanvas {
        anchors.fill: parent
        onPaint: {
          if (context) {
            context.reset()
            context.lineWidth = 0.075 * style.outerRadius
            context.beginPath()
            context.arc(style.outerRadius,
                style.outerRadius,
                style.outerRadius - context.lineWidth / 2,
                (style.valueToAngle(90)) * Math.PI / 180.0,
                (style.valueToAngle(50)) * Math.PI / 180.0, true)
            context.strokeStyle = "#00c300"
            context.stroke()
          }
        }
      }
    }
  }
}
