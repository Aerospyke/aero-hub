import QtQuick 2.0

BaseTankGauge {
  minimumValue: 0
  maximumValue: 30
  value: 0
  style: BaseTankGaugeStyle {
    minimumValueAngle: 45
    maximumValueAngle: -45
  }
}
