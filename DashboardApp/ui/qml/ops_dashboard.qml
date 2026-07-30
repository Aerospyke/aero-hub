import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15

// Ops wall: 2×3 grid — Video top-right, Tracking controls directly below it.
Item {
  id: root

  GridLayout {
    anchors.fill: parent
    anchors.margins: 16
    columns: 3
    rows: 2
    columnSpacing: 16
    rowSpacing: 16

    // Row 1
    EnvironmentModelPanel {
      Layout.fillWidth: true
      Layout.fillHeight: true
      Layout.row: 0
      Layout.column: 0
    }

    HubPanel {
      Layout.fillWidth: true
      Layout.fillHeight: true
      Layout.row: 0
      Layout.column: 1
    }

    VisualizationPanel {
      Layout.fillWidth: true
      Layout.fillHeight: true
      Layout.row: 0
      Layout.column: 2
    }

    // Row 2
    AutopilotPanel {
      Layout.fillWidth: true
      Layout.fillHeight: true
      Layout.row: 1
      Layout.column: 0
    }

    HighFidelityModelsPanel {
      Layout.fillWidth: true
      Layout.fillHeight: true
      Layout.row: 1
      Layout.column: 1
    }

    TrackingPanel {
      Layout.fillWidth: true
      Layout.fillHeight: true
      Layout.row: 1
      Layout.column: 2
    }
  }
}
