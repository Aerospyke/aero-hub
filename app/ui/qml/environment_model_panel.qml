import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
  id: panelRoot
  color: "#0d1620"
  border.color: "#243140"
  border.width: 1
  radius: 6

  property string contextMenuTargetName: ""
  property bool contextMenuIsGroup: false
  property int contextMenuRow: -1

  ColumnLayout {
    anchors.fill: parent
    anchors.margins: 8
    spacing: 2

    // Small section title above the table
    Text {
      Layout.fillWidth: true
      text: "JSB Settings"
      color: "#c8d1dc"
      font.pixelSize: 16
      font.bold: true
      leftPadding: 4
    }

    HorizontalHeaderView {
      id: headerView
      syncView: treeView
      model: ["Setting", "Value"]
      Layout.fillWidth: true
      clip: true

      delegate: Label {
        required property string modelData
        text: modelData
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        color: "#8fa0b0"
        font.pixelSize: 13
        font.bold: true
        elide: Text.ElideRight

        // Divider line after the "Setting" header column
        Rectangle {
          visible: modelData === "Setting"
          width: 1
          anchors.top: parent.top
          anchors.bottom: parent.bottom
          anchors.right: parent.right
          color: "#243140"
        }
      }
    }

    TreeView {
      id: treeView
      Layout.fillWidth: true
      Layout.fillHeight: true
      clip: true
      model: jsbSettingsModel

      delegate: Item {
        id: treeviewDelegate

        implicitHeight: 22
        implicitWidth: column === 0
            ? treeView.width * 0.5
            : treeView.width * 0.5

        readonly property real indentation: 16
        readonly property real padding: 4

        // Injected by TreeView
        required property TreeView treeView
        required property bool isTreeNode
        required property bool expanded
        required property int hasChildren
        required property int depth
        required property int row
        required property int column
        required property bool current

        // Expand/collapse indicator (▶)
        Label {
          id: indicator
          x: treeviewDelegate.padding + (treeviewDelegate.depth * treeviewDelegate.indentation)
          anchors.verticalCenter: parent.verticalCenter
          visible: treeviewDelegate.isTreeNode && treeviewDelegate.hasChildren
          text: "▶"
          color: "#c8d1dc"
          font.pixelSize: 11
          rotation: treeviewDelegate.expanded ? 90 : 0

          TapHandler {
            onTapped: treeviewDelegate.treeView.toggleExpanded(treeviewDelegate.row)
          }
        }

        // Main label (col 0 = setting name, col 1 = value)
        Label {
          id: label
          x: (treeviewDelegate.isTreeNode
                ? (treeviewDelegate.depth + (treeviewDelegate.hasChildren ? 1 : 0)) * treeviewDelegate.indentation
                : 0)
             + treeviewDelegate.padding + (indicator.visible ? 14 : 0)
          anchors.verticalCenter: parent.verticalCenter
          width: parent.width - x - treeviewDelegate.padding
          text: treeviewDelegate.column === 0 ? model.name : model.value
          color: treeviewDelegate.column === 0 ? "#c8d1dc" : "#a8c0d0"
          font.pixelSize: 12
          elide: Text.ElideRight
          horizontalAlignment: treeviewDelegate.column === 0 ? Text.AlignLeft : Text.AlignHCenter
        }

        // Vertical dividing line between columns (drawn from the first column's cell)
        Rectangle {
          visible: treeviewDelegate.column === 0
          width: 1
          anchors.top: parent.top
          anchors.bottom: parent.bottom
          anchors.right: parent.right
          color: "#243140"
        }

        // Right-click context menu support (covers both columns' delegates)
        MouseArea {
          anchors.fill: parent
          acceptedButtons: Qt.RightButton

          onClicked: (mouse) => {
            if (mouse.button === Qt.RightButton) {
              panelRoot.contextMenuTargetName = model.name || ""
              panelRoot.contextMenuIsGroup = treeviewDelegate.hasChildren
              panelRoot.contextMenuRow = treeviewDelegate.row

              const pos = mapToItem(panelRoot, mouse.x, mouse.y)
              if (panelRoot.contextMenuIsGroup) {
                groupContextMenu.x = pos.x
                groupContextMenu.y = pos.y
                groupContextMenu.open()
              } else {
                settingContextMenu.x = pos.x
                settingContextMenu.y = pos.y
                settingContextMenu.open()
              }
            }
          }
        }
      }
    }
  }

  // Context menus - separate menus so height matches exact item count
  Menu {
    id: groupContextMenu
    // x/y set dynamically before open()

    palette.window: "#0d1620"
    palette.text: "#c8d1dc"
    palette.highlight: "#243140"

    MenuItem {
      text: "Add Setting to " + panelRoot.contextMenuTargetName
      onTriggered: {
        console.log("Add Setting to group:", panelRoot.contextMenuTargetName, "row:", panelRoot.contextMenuRow)
        // TODO: implement add setting dialog/logic
      }
    }
  }

  Menu {
    id: settingContextMenu
    // x/y set dynamically before open()

    palette.window: "#0d1620"
    palette.text: "#c8d1dc"
    palette.highlight: "#243140"

    MenuItem {
      text: "Change " + panelRoot.contextMenuTargetName
      onTriggered: {
        console.log("Change setting:", panelRoot.contextMenuTargetName, "row:", panelRoot.contextMenuRow)
        // TODO: implement change value dialog/logic
      }
    }

    MenuItem {
      text: "Remove " + panelRoot.contextMenuTargetName
      onTriggered: {
        console.log("Remove setting:", panelRoot.contextMenuTargetName, "row:", panelRoot.contextMenuRow)
        // TODO: implement remove logic (and confirm?)
      }
    }
  }
}
