import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15

ApplicationWindow {
  width: 1920
  height: 1080
  minimumWidth: width
  minimumHeight: height
  maximumHeight: height
  maximumWidth: width

  visible: true
  title: "AeroHub Dashboard"
  color: "#6171A5"

  FontLoader {
    source: "qrc:/fonts/CenturyGothic.ttf"
  }

  ColumnLayout {
    anchors.fill: parent
    anchors.margins: 16
    spacing: 16

    // Row 1: Environment Model | Flight Instruments (EFIS) | Visualization
    RowLayout {
      Layout.fillWidth: true
      Layout.fillHeight: true
      spacing: 16

      EnvironmentModelPanel {
        Layout.fillWidth: true
        Layout.fillHeight: true
      }

      HubPanel {
        Layout.fillWidth: true
        Layout.fillHeight: true
      }

      VisualizationPanel {
        Layout.fillWidth: true
        Layout.fillHeight: true
      }
    }

    // Row 2: Autopilot (PX4) | High Fidelity Models
    RowLayout {
      Layout.fillWidth: true
      Layout.fillHeight: true
      spacing: 16

      AutopilotPanel {
        Layout.fillWidth: true
        Layout.fillHeight: true
      }

      HighFidelityModelsPanel {
        Layout.fillWidth: true
        Layout.fillHeight: true
      }
    }
  }
}
