import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15

// Settings — camera source from ah_core (Task_32). Thin ROS client only.
Item {
  id: root

  readonly property color chromePanel: "#0d1620"
  readonly property color chromePanelAlt: "#152030"
  readonly property color chromeBorder: "#243140"
  readonly property color chromeText: "#8a96a5"
  readonly property color chromeTextBright: "#e8eef5"
  readonly property color chromeInput: "#0a1018"
  readonly property color chromeAccent: "#3d9eff"

  Rectangle {
    anchors.fill: parent
    anchors.margins: 16
    radius: 8
    color: root.chromePanel
    border.color: root.chromeBorder
    border.width: 1

    ColumnLayout {
      anchors.fill: parent
      anchors.margins: 20
      spacing: 16

      Text {
        text: "Settings"
        color: root.chromeTextBright
        font.pixelSize: 26
        font.bold: true
      }

      Text {
        Layout.fillWidth: true
        wrapMode: Text.WordWrap
        text: "Video source is owned by ah_core. Refresh loads the device list over ROS; select applies without rebuilding."
        color: root.chromeText
        font.pixelSize: 14
      }

      // --- Camera source ---
      Rectangle {
        Layout.fillWidth: true
        Layout.preferredHeight: cameraColumn.implicitHeight + 28
        radius: 6
        color: root.chromePanelAlt
        border.color: root.chromeBorder
        border.width: 1

        ColumnLayout {
          id: cameraColumn
          anchors.left: parent.left
          anchors.right: parent.right
          anchors.top: parent.top
          anchors.margins: 14
          spacing: 12

          Text {
            text: "Camera / video source"
            color: root.chromeTextBright
            font.pixelSize: 18
            font.bold: true
          }

          GridLayout {
            Layout.fillWidth: true
            columns: 2
            columnSpacing: 12
            rowSpacing: 8

            Text {
              text: "Current source"
              color: root.chromeText
              font.pixelSize: 14
            }
            Text {
              text: cameraController ? (cameraController.currentVideoSource +
                    (cameraController.currentDevicePath.length
                     ? " · " + cameraController.currentDevicePath : ""))
                  : "—"
              color: root.chromeTextBright
              font.pixelSize: 14
              font.bold: true
            }

            Text {
              text: "Backend"
              color: root.chromeText
              font.pixelSize: 14
            }
            Text {
              text: cameraController && cameraController.currentBackend.length
                    ? cameraController.currentBackend : "—"
              color: root.chromeTextBright
              font.pixelSize: 14
            }

            Text {
              text: "Device"
              color: root.chromeText
              font.pixelSize: 14
              Layout.alignment: Qt.AlignVCenter
            }

            ComboBox {
              id: deviceCombo
              Layout.fillWidth: true
              Layout.preferredHeight: 34
              enabled: cameraController && !cameraController.busy
                       && cameraController.deviceCount > 0
              model: cameraController ? cameraController.deviceLabels : []
              currentIndex: cameraController ? cameraController.selectedListIndex : -1

              // Keep combo in sync when list/selection updates from ROS.
              Connections {
                target: cameraController
                function onDevicesChanged() {
                  deviceCombo.model = cameraController.deviceLabels
                  deviceCombo.currentIndex = cameraController.selectedListIndex
                }
                function onSelectionChanged() {
                  deviceCombo.currentIndex = cameraController.selectedListIndex
                }
              }

              contentItem: Text {
                leftPadding: 10
                rightPadding: deviceCombo.indicator.width + 8
                text: deviceCombo.displayText
                font.pixelSize: 14
                color: root.chromeTextBright
                verticalAlignment: Text.AlignVCenter
                elide: Text.ElideRight
              }
              background: Rectangle {
                radius: 4
                color: root.chromeInput
                border.color: root.chromeBorder
                border.width: 1
              }
              popup: Popup {
                y: deviceCombo.height
                width: deviceCombo.width
                implicitHeight: Math.min(contentItem.implicitHeight, 280)
                padding: 1
                contentItem: ListView {
                  clip: true
                  implicitHeight: contentHeight
                  model: deviceCombo.popup.visible ? deviceCombo.delegateModel : null
                  currentIndex: deviceCombo.highlightedIndex
                  ScrollIndicator.vertical: ScrollIndicator {}
                }
                background: Rectangle {
                  color: root.chromePanelAlt
                  border.color: root.chromeBorder
                  radius: 4
                }
              }
              delegate: ItemDelegate {
                width: deviceCombo.width
                height: 32
                contentItem: Text {
                  text: modelData
                  color: root.chromeTextBright
                  font.pixelSize: 13
                  elide: Text.ElideRight
                  verticalAlignment: Text.AlignVCenter
                  leftPadding: 8
                }
                background: Rectangle {
                  color: parent.highlighted ? "#243140" : "transparent"
                }
              }
            }
          }

          RowLayout {
            Layout.fillWidth: true
            spacing: 10

            Button {
              id: refreshBtn
              text: "Refresh list"
              enabled: cameraController && !cameraController.busy
              font.pixelSize: 14
              font.bold: true
              implicitHeight: 34
              implicitWidth: 120
              onClicked: {
                if (cameraController)
                  cameraController.RefreshDevices(true)
              }
              contentItem: Text {
                text: refreshBtn.text
                font: refreshBtn.font
                color: refreshBtn.enabled ? root.chromeTextBright : "#5a6572"
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
              }
              background: Rectangle {
                radius: 4
                color: refreshBtn.down ? "#243140" : root.chromePanel
                border.color: root.chromeBorder
              }
            }

            Button {
              id: applyBtn
              text: "Apply selection"
              enabled: cameraController && !cameraController.busy
                       && deviceCombo.currentIndex >= 0
              font.pixelSize: 14
              font.bold: true
              implicitHeight: 34
              implicitWidth: 140
              onClicked: {
                if (cameraController && deviceCombo.currentIndex >= 0)
                  cameraController.SelectDeviceAt(deviceCombo.currentIndex)
              }
              contentItem: Text {
                text: applyBtn.text
                font: applyBtn.font
                color: applyBtn.enabled ? "#0d1620" : "#5a6572"
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
              }
              background: Rectangle {
                radius: 4
                color: !applyBtn.enabled ? "#1a222c"
                     : (applyBtn.down ? "#2a7acc" : root.chromeAccent)
              }
            }

            Item { Layout.fillWidth: true }

            Text {
              visible: cameraController && cameraController.busy
              text: "Working…"
              color: root.chromeAccent
              font.pixelSize: 13
            }
          }

          Text {
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            text: cameraController ? cameraController.lastMessage : ""
            color: cameraController && cameraController.lastSuccess
                   ? "#7dcea0" : "#c9a06a"
            font.pixelSize: 13
          }
        }
      }

      Text {
        Layout.fillWidth: true
        wrapMode: Text.WordWrap
        text: "Tip: macOS Camera permission must be granted to the app that launches AeroHub (e.g. Ghostty, CLion). Live video is on Ops → Video."
        color: root.chromeText
        font.pixelSize: 12
      }

      Item { Layout.fillHeight: true }
    }
  }

  Component.onCompleted: {
    if (cameraController)
      cameraController.RefreshDevices(false)
  }
}
