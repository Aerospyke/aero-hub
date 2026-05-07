// KpiCard.qml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtGraphs

Rectangle {
  id: root

  property string title: "Title"
  property string value: "0"
  property string change: "-5.35%"
  property color changeColor: "#EA4335"
  property color lineColor: "#4285F4"
  property var dataPoints: []

  // implicitHeight: 225
  color: "white"
  radius: 12
  border.color: "#e0e0e0"

  ColumnLayout {
    anchors.fill: parent
    anchors.margins: 20
    spacing: 8

    RowLayout {
      Layout.fillWidth: true
      spacing: 20

      // Left side: Title + Value
      ColumnLayout {
        Layout.fillWidth: true
        spacing: 8

        Text {
          text: root.title
          font.pixelSize: 16
          color: "#666666"
        }

        Text {
          text: root.value
          font.pixelSize: 26
          font.bold: true
          color: "#1f2a44"
        }
      }

      // Spacer - pushes the right box to the far right
      Item {
        Layout.fillWidth: true      // This is the key
      }

      // Right side: Dark change box
      Rectangle {
        Layout.preferredWidth: 220
        Layout.preferredHeight: 48
        color: "#1f2a44"
        radius: 8

        RowLayout {
          anchors.fill: parent
          anchors.margins: 12
          spacing: 8

          Text {
            text: root.change
            color: root.changeColor
            font.pixelSize: 16
            font.bold: true
            Layout.alignment: Qt.AlignVCenter
          }

          Text {
            text: "vs. previous period"
            color: "#cccccc"
            font.pixelSize: 14
            Layout.alignment: Qt.AlignVCenter
          }
        }
      }
    } // End: Above Chart Row Layout

    // Chart (only if dataPoints provided)
    Loader {
      id: loader_chart
      active: false
      Layout.fillWidth: true
      Layout.preferredHeight: 65

      sourceComponent: GraphsView {
        theme: GraphsTheme {
          colorScheme: GraphsTheme.ColorScheme.Light
          backgroundColor: "transparent"
          plotAreaBackgroundColor: "transparent"
          grid.mainColor: "transparent"
          grid.subColor: "transparent"
        }

        axisX: ValueAxis {
          visible: false; labelsVisible: false; lineVisible: false
        }
        axisY: ValueAxis {
          visible: false; labelsVisible: false; lineVisible: false; min: 0; max: 100
        }

        LineSeries {
          color: root.lineColor
          width: 3.5

          Component.onCompleted: {
            if (root.dataPoints.length > 0) {
              for (var i = 0; i < root.dataPoints.length; i++) {
                append(i, root.dataPoints[i])
              }
            }
          }
        }
      }
    }

    // Spacer when no chart
    Item {
      id: spacer_when_no_chart
      Layout.fillHeight: true
      visible: root.dataPoints.length === 0
    }
  }
}
