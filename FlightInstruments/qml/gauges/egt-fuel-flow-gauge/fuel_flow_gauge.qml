import QtQuick 2.15

CircularGauge {
  id: root
  minimumValue: 0
  maximumValue: 20
  stepSize: 0.001
  value: 0

  style: CircularGaugeStyle {
    id: style
    minimumValueAngle: 145
    maximumValueAngle: 215
    tickmarkStepSize: 2.5
    labelStepSize: 5
    minorTickmarkCount: 0

    labelInset: 0.275 * style.outerRadius
    tickmarkInset: 0.02 * style.outerRadius

    tickmark: Rectangle {
      color: "#ffffff"
      width: 0.02 * style.outerRadius
      height: 0.115 * style.outerRadius
      radius: 0.01 * style.outerRadius
      antialiasing: true
    }

    tickmarkLabel: Component {
      Text {
        property real value: 0

        text: value
        color: "#ffffff"
        font.family: "Century Gothic"
        font.weight: Font.Black
        antialiasing: true
      }
    }

    needle: GaugeNeedleStandard {
      width: 0.075 * style.outerRadius
      height: 0.95 * style.outerRadius
    }

    background: Item {
      id: background
      width: 2 * style.outerRadius
      height: 2 * style.outerRadius
      property double radius: style.outerRadius

      GaugeCanvas {
        anchors.fill: parent
        onPaint: {
          if (context) {
            context.reset()
            context.lineWidth = 7 / 90 * style.outerRadius
            context.beginPath()
            context.arc(style.outerRadius, style.outerRadius, style.outerRadius - context.lineWidth / 2,
                (style.valueToAngle(0)) * Math.PI / 180.0, (style.valueToAngle(11)) * Math.PI / 180.0)
            context.strokeStyle = "#00c300"
            context.stroke()
          }
        }
      }
      GaugeText {
        color: "#ffffff"
        x: 0.1 * radius
        y: 1.6 * radius
        text: "GAL\nHR"
        font.pixelSize: Math.max(6, 0.05 * parent.width)
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
      }
    }
  }
}
