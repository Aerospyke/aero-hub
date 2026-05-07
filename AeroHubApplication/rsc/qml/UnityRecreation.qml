import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtGraphs
import Qt.labs.qmlmodels

Flickable {
  anchors.fill: parent
  contentWidth: parent.width
  contentHeight: mainColumn.height
  clip: true

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
          width: 52
          height: 52
          radius: 10
          color: "green"
          Text {
            anchors.centerIn: parent; text: "🦫"; font.pixelSize: 34; color: "white"
          }
        }

        Text {
          text: "System State - Backward Facing";
          font.pixelSize: 36;
          font.bold: true;
          color: "#1f2a44"
        }

        Item {
          Layout.fillWidth: true
        }

        Text {
          text: "Portfolio revenue summary over time";
          font.pixelSize: 20;
          color: "#555555"
        }
      }
    }

    // CHART WITH LEGEND ON THE RIGHT
    Rectangle {
      Layout.fillWidth: true
      Layout.preferredHeight: 520
      Layout.leftMargin: 20
      Layout.rightMargin: 20
      color: "white"
      radius: 12
      border.color: "#e0e0e0"

      RowLayout {
        anchors.fill: parent
        anchors.margins: 24
        spacing: 8

        Text {
          text: "USD"
          font.pixelSize: 20
          font.bold: true
          horizontalAlignment: Text.AlignHCenter
          verticalAlignment: Text.AlignVCenter
          Layout.preferredWidth: 40
          Layout.fillHeight: true
          rotation: -90
        }


        GraphsView {
          Layout.fillWidth: true
          Layout.fillHeight: true

          theme: GraphsTheme {
            colorScheme: GraphsTheme.ColorScheme.Light
            backgroundColor: "white"
            plotAreaBackgroundColor: "white"
            grid.mainColor: "#e0e0e0"
          }

          axisX: BarCategoryAxis {
            categories: ["-30", "-29", "-28", "-27", "-26", "-25", "-24", "-23", "-22", "-21", "-20", "-19", "-18", "-17", "-16", "-15", "-14", "-13", "-12", "-11", "-10", "-9", "-8", "-7", "-6", "-5", "-4", "-3", "-2", "-1"]
            titleText: "Days"
            titleFont.pixelSize: 20
            titleFont.bold: true
          }

          axisY: ValueAxis {
            min: 0
            max: 800000
            tickInterval: 100000
            labelDecimals: 0
          }

          BarSeries {
            id: revenueBars
            barWidth: 0.75

            BarSet {
              label: "Site Revenue A"; color: "#4285F4"; values: [520000, 680000, 450000, 610000, 590000, 710000, 480000, 550000, 620000, 530000, 490000, 670000, 580000, 640000, 510000, 720000, 460000, 590000, 630000, 570000, 680000, 520000, 610000, 540000, 650000, 480000, 700000, 530000, 590000, 620000]
            }
            BarSet {
              label: "Site Revenue B"; color: "#34A853"; values: [380000, 420000, 510000, 390000, 460000, 430000, 550000, 470000, 400000, 520000, 480000, 450000, 600000, 410000, 490000, 530000, 440000, 570000, 420000, 500000, 460000, 580000, 390000, 510000, 470000, 540000, 430000, 560000, 480000, 510000]
            }
            BarSet {
              label: "Site Revenue C"; color: "purple"; values: [310000, 280000, 390000, 340000, 420000, 370000, 450000, 310000, 480000, 350000, 400000, 460000, 330000, 510000, 380000, 430000, 470000, 360000, 490000, 410000, 450000, 380000, 520000, 340000, 470000, 500000, 390000, 440000, 460000, 420000]
            }
          }

          LineSeries {
            id: budgetLineA
            color: "#4285F4"
            width: 4
            XYPoint {
              x: 0; y: 450000
            }
            XYPoint {
              x: 10; y: 500000
            }
            XYPoint {
              x: 15; y: 425000
            }
            XYPoint {
              x: 20; y: 490000
            }
            XYPoint {
              x: 29; y: 450000
            }
          }
          LineSeries {
            id: budgetLineB
            color: "#34A853"
            width: 4
            XYPoint {
              x: 0; y: 350000
            }
            XYPoint {
              x: 10; y: 400000
            }
            XYPoint {
              x: 15; y: 325000
            }
            XYPoint {
              x: 20; y: 490000
            }
            XYPoint {
              x: 29; y: 350000
            }
          }
          LineSeries {
            id: budgetLineC
            color: "purple"
            width: 4
            XYPoint {
              x: 0; y: 404000
            }
            XYPoint {
              x: 10; y: 454000
            }
            XYPoint {
              x: 15; y: 379000
            }
            XYPoint {
              x: 20; y: 446000
            }
            XYPoint {
              x: 29; y: 403000
            }
          }
        }

        // Legend on the right
        ColumnLayout {
          Layout.preferredWidth: 230
          spacing: 18

          Text {
            text: "Legend"; font.pixelSize: 16; font.bold: true; color: "#333"
          }

          Repeater {
            model: revenueBars.legendData
            delegate: RowLayout {
              spacing: 12
              Rectangle {
                width: 20; height: 20; radius: 5; color: modelData.color
              }
              Text {
                text: modelData.label; font.pixelSize: 14; color: "#444"
              }
            }
          }

          ColumnLayout {
            spacing: 12
            RowLayout {
              spacing: 12
              Rectangle {
                width: 20; height: 4; radius: 2; color: budgetLineA.color
              }
              Text {
                text: "Site Budget A"; font.pixelSize: 14; color: "#444"
              }
            }
            RowLayout {
              spacing: 12
              Rectangle {
                width: 20; height: 4; radius: 2; color: budgetLineB.color
              }
              Text {
                text: "Site Budget B"; font.pixelSize: 14; color: "#444"
              }
            }
            RowLayout {
              spacing: 12
              Rectangle {
                width: 20; height: 4; radius: 2; color: budgetLineC.color
              }
              Text {
                text: "Site Budget C"; font.pixelSize: 14; color: "#444"
              }
            }
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

    Item {
      Layout.preferredHeight: 40
    }
  }
}