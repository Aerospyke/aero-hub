import QtQuick 2.15

CircularGauge {
  id: egt_gauge_root
  minimumValue: 700
  maximumValue: 1900
  stepSize: 1
  value: 0

  style: CircularGaugeStyle {
    id: style
    // progressArcThickness: 14
    minimumValueAngle: 35
    maximumValueAngle: -35
    tickmarkStepSize: 75
    labelStepSize: 1000
    minorTickmarkCount: 0

    labelInset: 0.25 * style.outerRadius
    tickmarkInset: 0.02 * style.outerRadius

    tickmark: Rectangle {
      color: "#ffffff"
      width: 0.02 * style.outerRadius
      height: 0.115 * style.outerRadius
      radius: 0.01 * style.outerRadius
      antialiasing: true
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

      GaugeText {
        color: "#ffffff"
        x: 1.6 * parent.radius
        y: 1.6 * parent.radius
        text: "25 °C\nDIV"
        font.pixelSize: Math.max(6, 0.05 * parent.width)
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
      }
    }
  }
}
