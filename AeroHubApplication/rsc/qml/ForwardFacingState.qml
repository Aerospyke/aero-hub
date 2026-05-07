import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtGraphs
import Qt.labs.qmlmodels

Flickable {
  id: forward_facing_state_root
  anchors.fill: parent
  contentWidth: parent.width
  contentHeight: mainColumn.height
  clip: true

  property color site_a_color: "#4285F4"
  property color site_b_color: "#34A853"
  property color site_c_color: "purple"

  ColumnLayout {
    id: mainColumn
    width: parent.width
    spacing: 20

    // HEADER
    Rectangle {
      Layout.fillWidth: true
      height: 80
      color: "white"

      RowLayout {
        anchors.fill: parent
        anchors.margins: 28
        spacing: 20

        Rectangle {
          id: beaver_icon_area
          Layout.preferredWidth: 52
          Layout.preferredHeight: 52
          radius: 10
          color: "green"
          Text {
            anchors.centerIn: parent; text: "🦫"; font.pixelSize: 34; color: "white"
          }
          transform: Scale {
            xScale: -1.0
            origin.x: (0.5 * beaver_icon_area.width)
          }
        }

        Text {
          text: "System State - Forward Facing";
          font.pixelSize: 36;
          font.bold: true;
          color: "#1f2a44"
        }

      }
    }


    // Graphs Section
    RowLayout {
      Layout.fillHeight: true
      ColumnLayout {
        Layout.fillWidth: true
        Layout.preferredHeight: 1080
        spacing: 8

        // Graph Downtime
        KpiGraph {
          graphTitle: "Downtime (Hours)"
          yAxisMax: 24
          yAxisTickInterval: 3
          xAxisVisible: false
          seriesAData: [{x: 0, y: 2}, {x: 10, y: 2}, {x: 15, y: 2.5}, {x: 20, y: 1.75}, {x: 29, y: 2}]
          seriesBData: [{x: 0, y: 0.5}, {x: 10, y: 5}, {x: 15, y: 8}, {x: 20, y: 4}, {x: 29, y: 3}]
          seriesCData: [{x: 0, y: 0.75}, {x: 10, y: 1.25}, {x: 15, y: 0.5}, {x: 20, y: 2.4}, {x: 29, y: 3}]
        }

        // Unscheduled Maintenance Graph View
        KpiGraph {
          graphTitle: "Unscheduled Maintenance \n% Probability"
          yAxisMax: 6
          yAxisTickInterval: 3
          xAxisVisible: false
          seriesAData: [{x: 0, y: 0.0}, {x: 10, y: 0.25}, {x: 15, y: 0.75}, {x: 20, y: 0.75}, {x: 29, y: 0.75}]
          seriesBData: [{x: 0, y: 0.25}, {x: 10, y: 0.25}, {x: 15, y: 0.5}, {x: 20, y: 0.75}, {x: 29, y: 0.75}]
          seriesCData: [{x: 0, y: 0.0}, {x: 10, y: 0.25}, {x: 15, y: 0.5}, {x: 20, y: 0.75}, {x: 29, y: 1.0}]
        }

        // Cost Graph View
        KpiGraph {
          graphTitle: "Cost (Euros)"
          yAxisMax: 3000
          yAxisTickInterval: 500
          xAxisVisible: false
          seriesAData: [{x: 0, y: 1000}, {x: 10, y: 500}, {x: 15, y: 1500}, {x: 20, y: 1750}, {x: 29, y: 1250}]
          seriesBData: [{x: 0, y: 750}, {x: 10, y: 750}, {x: 15, y: 500}, {x: 20, y: 250}, {x: 29, y: 1500}]
          seriesCData: [{x: 0, y: 1000}, {x: 10, y: 500}, {x: 15, y: 250}, {x: 20, y: 125}, {x: 29, y: 125}]
        }

        // Revenue Graph View
        KpiGraph {
          graphTitle: "Revenue (Euros)"
          yAxisMax: 6000
          yAxisTickInterval: 1000
          xAxisVisible: true
          seriesAData: [{x: 0, y: 3000}, {x: 10, y: 4000}, {x: 15, y: 4250}, {x: 20, y: 3750}, {x: 29, y: 3750}]
          seriesBData: [{x: 0, y: 2000}, {x: 10, y: 2000}, {x: 15, y: 500}, {x: 20, y: 4500}, {x: 29, y: 4500}]
          seriesCData: [{x: 0, y: 3000}, {x: 10, y: 3250}, {x: 15, y: 2750}, {x: 20, y: 2500}, {x: 29, y: 3250}]
        }


      }

      // Legend on the right
      ColumnLayout {
        Layout.preferredWidth: 230
        Layout.alignment: Qt.AlignHCenter
        spacing: 12
        RowLayout {
          Layout.alignment: Qt.AlignHCenter
          spacing: 12
          Rectangle {
            width: 30; height: 6; radius: 2; color: forward_facing_state_root.site_a_color
          }
          Text {
            text: "Site A"; font.pixelSize: 18; font.bold: true; color: "#444"
          }
        }
        RowLayout {
          Layout.alignment: Qt.AlignHCenter
          spacing: 12
          Rectangle {
            width: 30; height: 6; radius: 2; color: forward_facing_state_root.site_b_color
          }
          Text {
            text: "Site B"; font.pixelSize: 18; font.bold: true; color: "#444"
          }
        }
        RowLayout {
          Layout.alignment: Qt.AlignHCenter
          spacing: 12
          Rectangle {
            width: 30; height: 6; radius: 2; color: forward_facing_state_root.site_c_color
          }
          Text {
            text: "Site C"; font.pixelSize: 18; font.bold: true; color: "#444"
          }
        }
      }
    }

    // KPI ROW
    KpiRow {
      Layout.fillWidth: true
      Layout.preferredHeight: 225
      Layout.leftMargin: 20
      Layout.rightMargin: 20
    }

    // Table: Key Indicators
    Rectangle {
      id: table_container
      Layout.fillWidth: true
      Layout.preferredHeight: 350
      Layout.leftMargin: 20
      Layout.rightMargin: 20

      color: "white"
      radius: 12
      border.color: "#e0e0e0"

      ColumnLayout {
        anchors.fill: parent
        spacing: 12

        Text {
          text: "Portfolio key revenue indicators"
          font.pixelSize: 24
          font.bold: true
          color: "#1f2a44"
          leftPadding: 20
          topPadding: 20
          Layout.alignment: Qt.AlignLeft
        }

        HorizontalHeaderView {
          id: horizontalHeader
          implicitWidth: tableView.width
          // implicitHeight: 50
          Layout.alignment: Qt.AlignHCenter

          model: ["Site", "AC Capacity", "Revenue", "Lost Revenue", "Performance Index", "Active Export Energy", "Energy Expected", "Active Export Energy Budget"]
          syncView: tableView
          clip: true
          resizableColumns: true
          delegate: Rectangle {
            implicitWidth: 100
            implicitHeight: 50
            // text: model.display
            Text {
              text: modelData  // non abstract models assume data appears in modelData
              anchors.centerIn: parent
              font.pixelSize: 20
              font.bold: true
            }
          }
        }

        TableView {
          id: tableView
          Layout.preferredWidth: 1600
          Layout.fillHeight: true
          Layout.alignment: Qt.AlignHCenter
          Layout.bottomMargin: 20
          clip: true
          columnSpacing: 1
          rowSpacing: 1
          resizableColumns: true

          property var defaultColumnWidths: [90, 190, 190, 180, 170, 210, 210, 300]

          columnWidthProvider: function (column) {
            let w = explicitColumnWidth(column)
            return (w >= 0) ? w : defaultColumnWidths[column]
          }

          model: TableModel {
            TableModelColumn {
              display: "Site"
            }
            TableModelColumn {
              display: "AC Capacity"
            }
            TableModelColumn {
              display: "Revenue"
            }
            TableModelColumn {
              display: "Lost Revenue"
            }
            TableModelColumn {
              display: "Performance Index"
            }
            TableModelColumn {
              display: "Active Export Energy"
            }
            TableModelColumn {
              display: "Energy Expected"
            }
            TableModelColumn {
              display: "Active Export Energy Budget"
            }

            rows: [
              {
                "Site": "A",
                "AC Capacity": "22,500.00 kW",
                "Revenue": "350,000.00 $",
                "Lost Revenue": "45,000.00 $",
                "Performance Index": "92.00 %",
                "Active Export Energy": "133,552.69 kWh",
                "Energy Expected": "56,250,000.00 kWh",
                "Active Export Energy Budget": "41,000,000.00 kWh"
              },
              {
                "Site": "B",
                "AC Capacity": "105,544.00 kW",
                "Revenue": "2,463,731.38 $",
                "Lost Revenue": "447,060.44 $",
                "Performance Index": "83.99 %",
                "Active Export Energy": "49,308,813.20 kWh",
                "Energy Expected": "59,451,037.76 kWh",
                "Active Export Energy Budget": "22,385,600.00 kWh"
              },
              {
                "Site": "C",
                "AC Capacity": "324,450.00 kW",
                "Revenue": "5,056,455.50 $",
                "Lost Revenue": "1,164.61 $",
                "Performance Index": "110.50 %",
                "Active Export Energy": "73,438,262.70 kWh",
                "Energy Expected": "67,858,561.78 kWh",
                "Active Export Energy Budget": "96,946,876.81 kWh"
              },
              {
                "Site": "D",
                "AC Capacity": "300,150.00 kW",
                "Revenue": "3,283,033.70 $",
                "Lost Revenue": "24,019.24 $",
                "Performance Index": "95.06 %",
                "Active Export Energy": "65,809,787.63 kWh",
                "Energy Expected": "74,245,342.30 kWh",
                "Active Export Energy Budget": "65,848,676.32 kWh"
              }
            ]
          }

          delegate: Rectangle {
            implicitHeight: 48
            implicitWidth: 100
            color: row % 2 === 0 ? "#fafafa" : "white"

            Text {
              anchors.fill: parent
              anchors.leftMargin: 8
              anchors.rightMargin: 8
              horizontalAlignment: Text.AlignHCenter      // ← Centers horizontally
              verticalAlignment: Text.AlignVCenter        // ← Centers vertically
              text: display
              font.pixelSize: 18
              color: "#444444"
              elide: Text.ElideRight
            }
          }
        }
      }
    }

    Rectangle {
      id: wind_turbine_metrics_section
      Layout.fillWidth: true
      Layout.preferredHeight: 700
      Layout.leftMargin: 20
      Layout.rightMargin: 20

      color: "white"
      radius: 12
      border.color: "#e0e0e0"
      RowLayout {
        Layout.fillWidth: true
        Layout.fillHeight: true

        spacing: 20
        Layout.leftMargin: 20
        Layout.rightMargin: 20

        WindFarmMapView {
          id: windFarmMap
          Layout.fillWidth: true
          Layout.preferredWidth: 1200
          Layout.maximumWidth: 1200
          site_a_color: site_a_color
          site_b_color: site_b_color
          site_c_color: site_c_color
        }

        Connections {
          target: windFarmMap

          function onSiteClicked(siteName) {
            selectedTurbineMetrics.selectedSite = siteName
            selectedTurbineMetrics.selectedTurbine = siteName + "-T001"
          }
        }

        SelectedTurbineMetrics {
          id: selectedTurbineMetrics
          Layout.preferredWidth: 600
        }
      }
    }


    RecommendedActionsTable {
    }

    Item {
      Layout.preferredHeight: 40
    }
  }
}