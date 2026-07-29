import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15

// Control: status + track start/stop/cancel (Task_16 / 19 / 20)
Item {
  id: root

  readonly property color chromeBg: "#0d1620"
  readonly property color chromePanel: "#152030"
  readonly property color chromeBorder: "#243140"
  readonly property color chromeText: "#8a96a5"
  readonly property color chromeTextBright: "#e8eef5"
  readonly property color chromeInput: "#0a1018"

  // Dark SpinBox used for normalized tracking bounding box fields (0.00–1.00 shown, stored as 0–100).
  component DarkSpinBox: SpinBox {
    id: control
    from: 0
    to: 100
    stepSize: 1
    editable: true
    font.pixelSize: 13
    implicitWidth: 100
    implicitHeight: 32

    textFromValue: function (v) { return (v / 100).toFixed(2) }
    valueFromText: function (t) {
      const n = parseFloat(t)
      return isNaN(n) ? 0 : Math.round(Math.min(1, Math.max(0, n)) * 100)
    }

    contentItem: TextInput {
      z: 2
      text: control.displayText
      font: control.font
      color: root.chromeTextBright
      selectionColor: "#3d9eff"
      selectedTextColor: "#0d1620"
      horizontalAlignment: Qt.AlignHCenter
      verticalAlignment: Qt.AlignVCenter
      readOnly: !control.editable
      validator: control.validator
      inputMethodHints: Qt.ImhFormattedNumbersOnly
    }

    up.indicator: Rectangle {
      x: control.mirrored ? 0 : parent.width - width
      height: parent.height
      implicitWidth: 28
      implicitHeight: 32
      color: control.up.pressed ? "#243140" : root.chromePanel
      border.color: root.chromeBorder
      Text {
        anchors.centerIn: parent
        text: "+"
        color: control.up.enabled ? root.chromeTextBright : "#5a6572"
        font.pixelSize: 14
      }
    }

    down.indicator: Rectangle {
      x: control.mirrored ? parent.width - width : 0
      height: parent.height
      implicitWidth: 28
      implicitHeight: 32
      color: control.down.pressed ? "#243140" : root.chromePanel
      border.color: root.chromeBorder
      Text {
        anchors.centerIn: parent
        text: "−"
        color: control.down.enabled ? root.chromeTextBright : "#5a6572"
        font.pixelSize: 14
      }
    }

    background: Rectangle {
      implicitWidth: 100
      implicitHeight: 32
      radius: 4
      color: root.chromeInput
      border.color: root.chromeBorder
      border.width: 1
    }
  }

  component DarkButton: Button {
    id: btn
    property color face: "#1e5c32"
    property color faceDown: "#2a6b3a"
    property color edge: "#3d8f55"
    font.bold: true
    font.pixelSize: 13
    implicitHeight: 34
    implicitWidth: 96
    contentItem: Text {
      text: btn.text
      font: btn.font
      color: btn.enabled ? root.chromeTextBright : "#5a6572"
      horizontalAlignment: Text.AlignHCenter
      verticalAlignment: Text.AlignVCenter
    }
    background: Rectangle {
      radius: 4
      color: !btn.enabled ? "#1a2430" : (btn.down ? btn.faceDown : btn.face)
      border.color: btn.enabled ? btn.edge : root.chromeBorder
      border.width: 1
    }
  }

  Rectangle {
    anchors.fill: parent
    anchors.margins: 16
    radius: 8
    color: root.chromeBg
    border.color: root.chromeBorder
    border.width: 1

    ColumnLayout {
      anchors.fill: parent
      anchors.margins: 24
      spacing: 16

      Text {
        Layout.alignment: Qt.AlignHCenter
        text: "Control"
        color: root.chromeTextBright
        font.pixelSize: 28
        font.bold: true
      }

      Text {
        Layout.alignment: Qt.AlignHCenter
        text: systemStatus.connected ? "ROS status: live" : "ROS status: waiting for /ah/system/status…"
        color: systemStatus.connected ? "#6bcf7f" : root.chromeText
        font.pixelSize: 14
      }

      // --- Tracking commands ---
      Rectangle {
        Layout.alignment: Qt.AlignHCenter
        Layout.preferredWidth: Math.min(parent.width - 48, 580)
        Layout.preferredHeight: trackCol.implicitHeight + 28
        radius: 6
        color: root.chromePanel
        border.color: root.chromeBorder
        border.width: 1

        ColumnLayout {
          id: trackCol
          anchors.left: parent.left
          anchors.right: parent.right
          anchors.top: parent.top
          anchors.margins: 14
          spacing: 12

          Text {
            text: "Tracking"
            color: root.chromeTextBright
            font.pixelSize: 16
            font.bold: true
          }

          Text {
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            text: "Tracking bounding box is normalized [0,1]. Drag on Ops → Video, or edit fields (shown as 0.00–1.00)."
            color: root.chromeText
            font.pixelSize: 12
          }

          GridLayout {
            Layout.fillWidth: true
            columns: 4
            columnSpacing: 12
            rowSpacing: 4

            Text { text: "x"; color: root.chromeText; font.pixelSize: 11 }
            Text { text: "y"; color: root.chromeText; font.pixelSize: 11 }
            Text { text: "width"; color: root.chromeText; font.pixelSize: 11 }
            Text { text: "height"; color: root.chromeText; font.pixelSize: 11 }

            // Values are 0–100 internally; display maps to 0.00–1.00.
            // Only push controller → spin from drag (Connections). User edits use onValueModified.
            DarkSpinBox {
              id: spinX
              value: 35
              onValueModified: trackController.trackingBoundingBoxX = value / 100.0
            }
            DarkSpinBox {
              id: spinY
              value: 35
              onValueModified: trackController.trackingBoundingBoxY = value / 100.0
            }
            DarkSpinBox {
              id: spinW
              from: 1
              value: 30
              onValueModified: trackController.trackingBoundingBoxWidth = value / 100.0
            }
            DarkSpinBox {
              id: spinH
              from: 1
              value: 30
              onValueModified: trackController.trackingBoundingBoxHeight = value / 100.0
            }
          }

          // Keep spins aligned when tracking bounding box changes from video drag / ResetTrackingBoundingBox.
          // (No property binding on spin.value — avoids binding loops with onValueModified.)
          Connections {
            target: trackController
            function onTrackingBoundingBoxChanged() {
              spinX.value = Math.round(trackController.trackingBoundingBoxX * 100)
              spinY.value = Math.round(trackController.trackingBoundingBoxY * 100)
              spinW.value = Math.round(trackController.trackingBoundingBoxWidth * 100)
              spinH.value = Math.round(trackController.trackingBoundingBoxHeight * 100)
            }
          }

          Component.onCompleted: {
            spinX.value = Math.round(trackController.trackingBoundingBoxX * 100)
            spinY.value = Math.round(trackController.trackingBoundingBoxY * 100)
            spinW.value = Math.round(trackController.trackingBoundingBoxWidth * 100)
            spinH.value = Math.round(trackController.trackingBoundingBoxHeight * 100)
          }

          RowLayout {
            Layout.fillWidth: true
            spacing: 10

            DarkButton {
              text: "Start"
              enabled: !trackController.busy
              face: "#1e5c32"
              faceDown: "#2a6b3a"
              edge: "#3d8f55"
              onClicked: trackController.StartTracking()
            }
            DarkButton {
              text: "Stop"
              enabled: !trackController.busy
              face: "#5c4a1e"
              faceDown: "#6b5a20"
              edge: "#e6c35c"
              onClicked: trackController.StopTracking()
            }
            DarkButton {
              text: "Cancel"
              enabled: !trackController.busy
              face: "#5c1e1e"
              faceDown: "#6b2a2a"
              edge: "#e07a7a"
              onClicked: trackController.CancelTracking()
            }
            DarkButton {
              text: "Reset"
              enabled: !trackController.busy
              face: "#1a2430"
              faceDown: "#243140"
              edge: root.chromeBorder
              font.bold: false
              onClicked: trackController.ResetTrackingBoundingBox()
            }

            Item { Layout.fillWidth: true }
          }

          Text {
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            text: (trackController.busy ? "Working… " : "") + trackController.lastMessage
            color: trackController.lastSuccess ? "#6bcf7f" : "#e6c35c"
            font.pixelSize: 12
          }
        }
      }

      // --- Status detail ---
      GridLayout {
        Layout.alignment: Qt.AlignHCenter
        Layout.preferredWidth: Math.min(parent.width - 48, 580)
        columns: 2
        columnSpacing: 24
        rowSpacing: 10

        component StatusLabel: Text {
          color: root.chromeText
          font.pixelSize: 15
        }
        component StatusValue: Text {
          color: root.chromeTextBright
          font.pixelSize: 15
          font.bold: true
          Layout.fillWidth: true
        }

        StatusLabel { text: "tracking_started" }
        StatusValue {
          text: systemStatus.trackingStarted ? "true" : "false"
          color: systemStatus.trackingStarted ? "#6bcf7f" : root.chromeTextBright
        }

        StatusLabel { text: "video_status" }
        StatusValue {
          text: systemStatus.videoStatus
          color: {
            if (systemStatus.videoStatus === "connected" || systemStatus.videoStatus === "ok")
              return "#6bcf7f"
            if (systemStatus.videoStatus === "degraded")
              return "#e6c35c"
            if (systemStatus.videoStatus === "unavailable" || systemStatus.videoStatus === "unknown")
              return "#e07a7a"
            return root.chromeTextBright
          }
        }

        StatusLabel { text: "smart_mode_active" }
        StatusValue { text: systemStatus.smartModeActive ? "true" : "false" }

        StatusLabel { text: "following_active" }
        StatusValue { text: systemStatus.followingActive ? "true" : "false" }

        StatusLabel { text: "segmentation_active" }
        StatusValue { text: systemStatus.segmentationActive ? "true" : "false" }

        StatusLabel { text: "tracker_type" }
        StatusValue { text: systemStatus.trackerType }

        StatusLabel { text: "follower_mode" }
        StatusValue { text: systemStatus.followerMode }

        StatusLabel { text: "stamp" }
        StatusValue { text: Number(systemStatus.stamp).toFixed(3) }
      }

      Item { Layout.fillHeight: true }
    }
  }
}
