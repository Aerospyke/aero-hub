import QtQuick 2.15

Item {
  id: propeller_gauge_root
  property double radius: 300
  property double rpm: 0

  width: 2 * radius
  height: 2 * radius

  CircularGauge {
    width: parent.width
    height: parent.height
    anchors.centerIn: parent
    scale: 0.77
    minimumValue: 0
    maximumValue: 3500
    stepSize: 1
    value: rpm
    style: PropellerGaugeStyle {
      parentGauge: propeller_gauge_root
    }
  }

  GaugeImage {
    anchors.fill: parent
    source: "qrc:/images/case.svg"
  }

}
