import QtQuick 2.0

BaseTankGauge {
  minimumValue: 0
  maximumValue: 30
  value: 0
  style: BaseTankGaugeStyle {
    minimumValueAngle: 135
    maximumValueAngle: 225
    positiveDirectionIsClockwise: true
  }

}
