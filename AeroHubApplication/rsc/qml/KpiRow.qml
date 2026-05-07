// KpiRow.qml
import QtQuick
import QtQuick.Layouts

RowLayout {
  spacing: 20

  KpiCard {
    Layout.fillWidth: true
    Layout.fillHeight: true

    title: "Revenue"
    value: "15,048,790 $"
    change: "-5.35%"
    changeColor: "#EA4335"
    lineColor: "#4285F4"
    dataPoints: [35, 60, 42, 75, 48, 68, 55]
  }

  KpiCard {
    Layout.fillWidth: true
    Layout.fillHeight: true

    title: "Energy Production"
    value: "273,670,328 kWh"
    change: "-5.35%"
    changeColor: "#EA4335"
    // lineColor: "#4285F4"
    // dataPoints: [68, 45, 78, 52, 82, 58, 71]
  }

  KpiCard {
    Layout.fillWidth: true
    Layout.fillHeight: true

    title: "Operational Availability"
    value: "93.68 %"
    change: "-5.35%"
    changeColor: "#34A853"
    lineColor: "#34A853"
    dataPoints: [88, 93, 89, 96, 91, 94, 92]
  }
}