import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15

// Control: status + track start/stop/cancel (Task_16 / 19 / 20)
Item {
  id: root

  Rectangle {
    anchors.fill: parent
    anchors.margins: 16
    radius: 8
    color: "#0d1620"
    border.color: "#243140"
    border.width: 1

    ColumnLayout {
      anchors.fill: parent
      anchors.margins: 24
      spacing: 16

      Text {
        Layout.alignment: Qt.AlignHCenter
        text: "Control"
        color: "#c8d1dc"
        font.pixelSize: 28
        font.bold: true
      }

      Text {
        Layout.alignment: Qt.AlignHCenter
        text: systemStatus.connected ? "ROS status: live" : "ROS status: waiting for /ah/system/status…"
        color: systemStatus.connected ? "#6bcf7f" : "#8a96a5"
        font.pixelSize: 14
      }

      // --- Tracking commands ---
      Rectangle {
        Layout.alignment: Qt.AlignHCenter
        Layout.preferredWidth: Math.min(parent.width - 48, 560)
        Layout.preferredHeight: trackCol.implicitHeight + 28
        radius: 6
        color: "#152030"
        border.color: "#243140"
        border.width: 1

        ColumnLayout {
          id: trackCol
          anchors.left: parent.left
          anchors.right: parent.right
          anchors.top: parent.top
          anchors.margins: 14
          spacing: 10

          Text {
            text: "Tracking"
            color: "#c8d1dc"
            font.pixelSize: 16
            font.bold: true
          }

          Text {
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            text: "BBox is normalized [0,1] (interface map). Drag on Ops → Video, or edit fields below."
            color: "#8a96a5"
            font.pixelSize: 12
          }

          GridLayout {
            Layout.fillWidth: true
            columns: 4
            columnSpacing: 10
            rowSpacing: 6

            component BboxSpin: ColumnLayout {
              property string labelText: ""
              property alias spin: spin
              spacing: 2
              Text {
                text: labelText
                color: "#8a96a5"
                font.pixelSize: 11
              }
              SpinBox {
                id: spin
                from: 0
                to: 100
                stepSize: 1
                editable: true
                Layout.preferredWidth: 96
                // display as 0.00–1.00
                textFromValue: function (v) { return (v / 100).toFixed(2) }
                valueFromText: function (t) {
                  const n = parseFloat(t)
                  return isNaN(n) ? 0 : Math.round(n * 100)
                }
              }
            }

            BboxSpin {
              id: fieldX
              labelText: "x"
              spin.value: Math.round(trackController.bboxX * 100)
              spin.onValueModified: trackController.bboxX = spin.value / 100
            }
            BboxSpin {
              id: fieldY
              labelText: "y"
              spin.value: Math.round(trackController.bboxY * 100)
              spin.onValueModified: trackController.bboxY = spin.value / 100
            }
            BboxSpin {
              id: fieldW
              labelText: "width"
              spin.from: 1
              spin.value: Math.round(trackController.bboxWidth * 100)
              spin.onValueModified: trackController.bboxWidth = spin.value / 100
            }
            BboxSpin {
              id: fieldH
              labelText: "height"
              spin.from: 1
              spin.value: Math.round(trackController.bboxHeight * 100)
              spin.onValueModified: trackController.bboxHeight = spin.value / 100
            }
          }

          Connections {
            target: trackController
            function onBboxChanged() {
              fieldX.spin.value = Math.round(trackController.bboxX * 100)
              fieldY.spin.value = Math.round(trackController.bboxY * 100)
              fieldW.spin.value = Math.round(trackController.bboxWidth * 100)
              fieldH.spin.value = Math.round(trackController.bboxHeight * 100)
            }
          }

          RowLayout {
            Layout.fillWidth: true
            spacing: 10

            Button {
              text: "Start"
              enabled: !trackController.busy
              onClicked: trackController.StartTracking()
              background: Rectangle {
                implicitHeight: 34
                implicitWidth: 88
                radius: 4
                color: parent.down ? "#2a6b3a" : (parent.enabled ? "#1e5c32" : "#1a2430")
                border.color: "#3d8f55"
                border.width: 1
              }
              contentItem: Text {
                text: parent.text
                color: parent.enabled ? "#e8eef5" : "#5a6572"
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                font.bold: true
              }
            }

            Button {
              text: "Stop"
              enabled: !trackController.busy
              onClicked: trackController.StopTracking()
              background: Rectangle {
                implicitHeight: 34
                implicitWidth: 88
                radius: 4
                color: parent.down ? "#6b5a20" : (parent.enabled ? "#5c4a1e" : "#1a2430")
                border.color: "#e6c35c"
                border.width: 1
              }
              contentItem: Text {
                text: parent.text
                color: parent.enabled ? "#e8eef5" : "#5a6572"
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                font.bold: true
              }
            }

            Button {
              text: "Cancel"
              enabled: !trackController.busy
              onClicked: trackController.CancelTracking()
              background: Rectangle {
                implicitHeight: 34
                implicitWidth: 88
                radius: 4
                color: parent.down ? "#6b2a2a" : (parent.enabled ? "#5c1e1e" : "#1a2430")
                border.color: "#e07a7a"
                border.width: 1
              }
              contentItem: Text {
                text: parent.text
                color: parent.enabled ? "#e8eef5" : "#5a6572"
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                font.bold: true
              }
            }

            Button {
              text: "Reset box"
              enabled: !trackController.busy
              onClicked: trackController.ResetBbox()
              flat: true
              contentItem: Text {
                text: parent.text
                color: "#8a96a5"
                font.pixelSize: 12
              }
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
        Layout.preferredWidth: Math.min(parent.width - 48, 560)
        columns: 2
        columnSpacing: 24
        rowSpacing: 10

        component StatusLabel: Text {
          color: "#8a96a5"
          font.pixelSize: 15
        }
        component StatusValue: Text {
          color: "#e8eef5"
          font.pixelSize: 15
          font.bold: true
          Layout.fillWidth: true
        }

        StatusLabel { text: "tracking_started" }
        StatusValue {
          text: systemStatus.trackingStarted ? "true" : "false"
          color: systemStatus.trackingStarted ? "#6bcf7f" : "#e8eef5"
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
            return "#e8eef5"
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
