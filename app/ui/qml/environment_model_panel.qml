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

  property int settingColumnWidth: 180

  TextMetrics {
    id: textMetrics
    font.pixelSize: 12
  }

  function updateSettingColumnWidth() {
    // Full traversal - forces column to absolute max needed by entire tree
    if (!jsbSettingsModel) {
      settingColumnWidth = 180
      return
    }

    let maxW = 60

    function visit(parentIndex, depth) {
      const rows = jsbSettingsModel.rowCount(parentIndex)
      for (let r = 0; r < rows; r++) {
        const idx = jsbSettingsModel.index(r, 0, parentIndex)
        const name = jsbSettingsModel.data(idx, Qt.DisplayRole) || ""
        const childCount = jsbSettingsModel.rowCount(idx)
        const hasChildren = childCount > 0

        textMetrics.text = name
        const textW = textMetrics.width

        // Match the x calculation in the delegate label exactly
        const x = (depth + (hasChildren ? 1 : 0)) * 16 + 4 + (hasChildren ? 14 : 0)
        const needed = x + textW + 16  // right padding + margin/buffer so text isn't pressed against the divider
        if (needed > maxW)
          maxW = needed

        if (hasChildren) {
          visit(idx, depth + 1)
        }
      }
    }

    visit(jsbSettingsModel.index(-1, -1), 0)

    settingColumnWidth = Math.max(100, maxW)

    Qt.callLater(function() {
      if (treeView && treeView.forceLayout)
        treeView.forceLayout()
    })
  }

  function considerSettingWidth(name, depth, hasChildren) {
    // Kept for backward compatibility with some delegate paths
    textMetrics.text = name || ""
    const hasCh = hasChildren
    // Match the x calculation in the delegate label exactly
    const x = (depth + (hasCh ? 1 : 0)) * 16 + 4 + (hasCh ? 14 : 0)
    const needed = x + textMetrics.width + 16  // right padding + margin/buffer so text isn't pressed against the divider
    if (needed > settingColumnWidth) {
      settingColumnWidth = needed
      Qt.callLater(function() {
        if (treeView && treeView.forceLayout)
          treeView.forceLayout()
      })
    }
  }

  function recomputeVisibleSettingWidth() {
    // Computes required width using only currently visible (expanded) rows.
    // This allows the Setting column to shrink when branches are collapsed.
    if (!treeView || !jsbSettingsModel) {
      settingColumnWidth = 100
      return
    }

    let maxW = 60
    const numVisualRows = treeView.rows || 0

    for (let visualRow = 0; visualRow < numVisualRows; visualRow++) {
      const modelIdx = treeView.index(visualRow, 0)
      if (!modelIdx || !modelIdx.valid) continue

      const name = jsbSettingsModel.data(modelIdx, Qt.DisplayRole) || ""

      // Walk parents in the model to determine depth
      let depth = 0
      let current = jsbSettingsModel.parent(modelIdx)
      while (current && current.valid) {
        depth++
        current = jsbSettingsModel.parent(current)
      }

      textMetrics.text = name
      const textW = textMetrics.width

      const hasChildren = jsbSettingsModel.rowCount(modelIdx) > 0
      // Match the x calculation in the delegate label exactly
      const x = (depth + (hasChildren ? 1 : 0)) * 16 + 4 + (hasChildren ? 14 : 0)
      const needed = x + textW + 16  // right padding + margin/buffer so text isn't pressed against the divider
      if (needed > maxW) maxW = needed
    }

    settingColumnWidth = Math.max(100, maxW)

    Qt.callLater(function() {
      if (treeView && treeView.forceLayout)
        treeView.forceLayout()
    })
  }

  ColumnLayout {
    anchors.fill: parent
    anchors.margins: 8
    spacing: 2

    // Main panel title (restored from earlier placeholder version)
    Text {
      Layout.fillWidth: true
      text: "Environment Model (JSB, Gazebo, X-Plane)"
      color: "#c8d1dc"
      font.pixelSize: 20
      font.bold: true
      horizontalAlignment: Text.AlignHCenter
    }

    // Small section title above the table
    Text {
      Layout.fillWidth: true
      text: "JSB Settings"
      color: "#c8d1dc"
      font.pixelSize: 16
      font.bold: true
      horizontalAlignment: Text.AlignHCenter
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

      // Make column 0 (Setting names + tree indentation) size to content.
      // Value column takes the remaining space.
      columnWidthProvider: function(column) {
        if (column === 0)
          return panelRoot.settingColumnWidth
        return Math.max(120, treeView.width - panelRoot.settingColumnWidth)
      }

      onWidthChanged: Qt.callLater(forceLayout)

      Component.onCompleted: Qt.callLater(panelRoot.recomputeVisibleSettingWidth)

      delegate: Item {
        id: treeviewDelegate

        implicitHeight: 22
        implicitWidth: column === 0 ? panelRoot.settingColumnWidth : 0

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

        Component.onCompleted: {
          if (column === 0) {
            panelRoot.recomputeVisibleSettingWidth()
          }
        }

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

        onExpandedChanged: {
          indicator.rotation = expanded ? 90 : 0
          // Recompute visible content width on both expand (grow) and collapse (shrink)
          panelRoot.recomputeVisibleSettingWidth()
        }

        // Main label (col 0 = setting name, col 1 = value)
        Label {
          id: label
          x: (treeviewDelegate.isTreeNode
                ? (treeviewDelegate.depth + (treeviewDelegate.hasChildren ? 1 : 0)) * treeviewDelegate.indentation
                : 0)
             + treeviewDelegate.padding + (indicator.visible ? 14 : 0)
          anchors.verticalCenter: parent.verticalCenter
          width: parent.width - x - treeviewDelegate.padding - 8  // extra right margin before the column divider line
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
                textMetrics.text = "Add Setting to " + panelRoot.contextMenuTargetName
                const w = textMetrics.width + 48
                groupContextMenu.implicitWidth = w
                groupContextMenu.width = w
                groupContextMenu.x = pos.x
                groupContextMenu.y = pos.y
                groupContextMenu.open()
              } else {
                const t1 = "Change " + panelRoot.contextMenuTargetName
                const t2 = "Remove " + panelRoot.contextMenuTargetName
                textMetrics.text = t1.length > t2.length ? t1 : t2
                const w = textMetrics.width + 48
                settingContextMenu.implicitWidth = w
                settingContextMenu.width = w
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
