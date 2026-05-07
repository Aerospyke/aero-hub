import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qt.labs.qmlmodels

Rectangle {
  id: "table_container"
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
      text: "Recommended Actions"
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

      model: ["Description", "Apply To Prognosis"]
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
      Layout.preferredWidth: 800
      Layout.fillHeight: true
      Layout.alignment: Qt.AlignHCenter
      Layout.bottomMargin: 20
      clip: true
      columnSpacing: 1
      rowSpacing: 1
      resizableColumns: false

      property var defaultColumnWidths: [600, 200]

      columnWidthProvider: function (column) {
        let w = explicitColumnWidth(column)
        return (w >= 0) ? w : defaultColumnWidths[column]
      }

      model: TableModel {
        TableModelColumn {
          display: "Action Description"
        }
        TableModelColumn {
          display: "Apply Action to Prognosis"
        }

        rows: [
          {
            "Action Description": "A Very Good Idea",
            "Apply Action to Prognosis": "true"
          },
          {
            "Action Description": "Another Very Good Idea",
            "Apply Action to Prognosis": "false"
          },
          {
            "Action Description": "Wow, such brilliant Suggestions!",
            "Apply Action to Prognosis": "true"
          },
          {
            "Action Description": "These Actions are almost too good!",
            "Apply Action to Prognosis": "false"
          },
        ]
      }

      delegate: DelegateChooser {
        DelegateChoice {
          column: 0
          Rectangle {
            implicitHeight: 48
            implicitWidth: 100
            color: row % 2 === 0 ? "#fafafa" : "white"
            Text {
              text: model.display
              anchors.fill: parent
              anchors.leftMargin: 8
              anchors.rightMargin: 8
              horizontalAlignment: Text.AlignLeft
              verticalAlignment: Text.AlignVCenter
              font.pixelSize: 18
              color: "#444444"
              elide: Text.ElideRight
            }
          }
        }
        DelegateChoice {
          column: 1
          SwitchDelegate {
            checked: model.display
            background: Rectangle {
              color: row % 2 === 0 ? "#fafafa" : "white"
            }
            onToggled: model.display = checked
          }
        }
      }
    }
  }
}