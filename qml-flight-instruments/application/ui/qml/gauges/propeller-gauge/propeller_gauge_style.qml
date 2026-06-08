import QtQuick 2.15

CircularGaugeStyle {
  id: propeller_gauge_style
  minimumValueAngle: -216
  maximumValueAngle: 36
  tickmarkLabelPixelSize: Math.max(6, Math.round(0.15 * outerRadius))

  tickmarkStepSize: 500
  labelStepSize: 500
  minorTickmarkCount: 5
  labelInset: 25 / 90 * outerRadius
  tickmarkInset: 6 / 90 * outerRadius
  minorTickmarkInset: 6 / 90 * outerRadius

  tickmark: Rectangle {
    property real value: 0

    color: "#ffffff"
    width: 3 / 90 * outerRadius
    height: propeller_gauge_style.tickmarkHeight
    radius: 3 / 90 * outerRadius
    antialiasing: true
  }

  minorTickmark: Rectangle {
    property real value: 0

    color: value === 2700 ? "#e30000" : "#BBBBBB"
    height: value === 2700 ? propeller_gauge_style.tickmarkHeight : propeller_gauge_style.minorTickmarkHeight
    width: 1.5 / 90 * outerRadius
    radius: 3 / 90 * outerRadius
    antialiasing: true
  }

  tickmarkLabel: Component {
    Text {
      id: labelText

      property real value: 0

      font.family: "Century Gothic"
      text: (value / 100).toFixed(0)
      antialiasing: true
      color: "#ffffff"
      font.weight: Font.Black

      horizontalAlignment: Text.AlignHCenter
      verticalAlignment: Text.AlignVCenter

    }
  }

  foreground: Item {
    anchors.fill: parent
    SingularGaugeForeground {
      anchors.centerIn: parent
      radius: 10 / 90 * outerRadius
    }
  }

  needle: GaugeNeedleStandard {
    width: 8 / 90 * outerRadius
    height: 65 / 90 * outerRadius
  }

  background: Rectangle {
    id: background
    radius: outerRadius
    color: "#181818"

    // Green Range On Dial
    GaugeCanvas {
      anchors.fill: parent
      onPaint: {
        if (context) {
          context.reset()
          context.lineWidth = 7 / 90 * outerRadius
          context.beginPath()
          context.arc(outerRadius, outerRadius, outerRadius - 0.35 * propeller_gauge_style.minorTickmarkHeight,
              propeller_gauge_style.valueToAngle(2100) * Math.PI / 180, propeller_gauge_style.valueToAngle(2700) * Math.PI / 180)
          context.strokeStyle = "#00c300"
          context.stroke()
        }
      }
    }

    Text {
      x: 0
      y: 0.65 * outerRadius
      width: 2 * outerRadius
      text: "x100 RPM"
      color: "#ffffff"
      font.family: "Century Gothic"
      font.pixelSize: Math.max(6, outerRadius * 0.125)
      antialiasing: true
      horizontalAlignment: Text.AlignHCenter
    }
  }
}
