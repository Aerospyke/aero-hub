import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
  id: root
  Layout.fillWidth: true
  Layout.preferredHeight: 650
  color: "#fafafa"
  radius: 12
  border.color: "#e0e0e0"

  property string selectedSite: ""
  property string selectedTurbine: ""

  ColumnLayout {
    anchors.fill: parent
    anchors.margins: 16
    spacing: 12
    Layout.preferredWidth: 450

    Text {
      text: selectedSite === "" ? "Selected Turbine Metrics" : selectedSite
      font.pixelSize: 20
      font.bold: true
      color: "#1f2a44"
      Layout.alignment: Qt.AlignHCenter
    }

    ColumnLayout {
      Layout.fillWidth: true
      spacing: 12
      Layout.preferredHeight: 500
      Layout.preferredWidth: 450

      // Site & Turbine Display
      Rectangle {
        Layout.fillWidth: true
        Layout.preferredHeight: 75
        radius: 8
        color: "white"
        border.color: "#e0e0e0"

        ColumnLayout {
          anchors.fill: parent
          anchors.margins: 12
          spacing: 8

          RowLayout {
            Text {
              text: "Site"
              font.pixelSize: 12
              color: "#666666"
            }
            Text {
              text: selectedSite
              font.pixelSize: 16
              font.bold: true
              color: "#1f2a44"
            }
          }


          RowLayout {
            Text {
              text: "Turbine"
              font.pixelSize: 12
              color: "white"
            }
            Text {
              text: selectedTurbine
              font.pixelSize: 16
              font.bold: true
              color: "#1f2a44"
            }
          }
        }
      }

      // Status
      Rectangle {
        Layout.fillWidth: true
        radius: 8
        color: "white"
        border.color: "#e0e0e0"
        Layout.preferredWidth: 450
        Layout.preferredHeight: 75
        RowLayout {
          Layout.preferredWidth: 450
          anchors.fill: parent
          anchors.margins: 12
          spacing: 8

          Text {
            text: "Status"
            font.pixelSize: 12
            color: "#666666"
          }
          Text {
            text: "Operational"
            font.pixelSize: 16
            font.bold: true
            color: "green"
          }
        }
      }

      // Power Output
      Rectangle {
        Layout.fillWidth: true
        radius: 8
        color: "white"
        border.color: "#e0e0e0"
        Layout.preferredWidth: 450
        Layout.preferredHeight: 75
        RowLayout {
          anchors.fill: parent
          anchors.margins: 12
          spacing: 8

          Text {
            text: "Power Output"
            font.pixelSize: 12
            color: "#666666"
          }
          Text {
            text: "2.4 MW"
            font.pixelSize: 16
            font.bold: true
            color: "#1f2a44"
          }
        }
      }

      // RPM
      Rectangle {
        Layout.fillWidth: true
        radius: 8
        color: "white"
        border.color: "#e0e0e0"
        Layout.preferredWidth: 450
        Layout.preferredHeight: 75
        RowLayout {
          anchors.fill: parent
          anchors.margins: 12
          spacing: 8

          Text {
            text: "Rotor RPM"
            font.pixelSize: 12
            color: "#666666"
          }
          Text {
            text: "14.2 RPM"
            font.pixelSize: 16
            font.bold: true
            color: "#1f2a44"
          }
        }
      }

      // Efficiency
      Rectangle {
        Layout.fillWidth: true
        radius: 8
        color: "white"
        border.color: "#e0e0e0"
        Layout.preferredWidth: 450
        Layout.preferredHeight: 75
        RowLayout {
          anchors.fill: parent
          anchors.margins: 12
          spacing: 8

          Text {
            text: "Efficiency"
            font.pixelSize: 12
            color: "#666666"
          }
          Text {
            text: "92.5%"
            font.pixelSize: 16
            font.bold: true
            color: "#1f2a44"
          }
        }
      }

      // Temperature
      Rectangle {
        Layout.fillWidth: true
        radius: 8
        color: "white"
        border.color: "#e0e0e0"
        Layout.preferredWidth: 450
        Layout.preferredHeight: 75
        RowLayout {
          anchors.fill: parent
          anchors.margins: 12
          spacing: 8

          Text {
            text: "Bearing Temp"
            font.pixelSize: 12
            color: "#666666"
          }
          Text {
            text: "45°C"
            font.pixelSize: 16
            font.bold: true
            color: "#1f2a44"
          }
        }
      }
    }
  }
}
