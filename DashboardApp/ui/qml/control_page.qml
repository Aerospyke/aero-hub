import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15

// Control surface: live system status (Task_16); track services later.
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
      spacing: 20

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

      GridLayout {
        Layout.alignment: Qt.AlignHCenter
        Layout.preferredWidth: Math.min(parent.width - 48, 520)
        columns: 2
        columnSpacing: 24
        rowSpacing: 12

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

      Text {
        Layout.fillWidth: true
        Layout.alignment: Qt.AlignHCenter
        horizontalAlignment: Text.AlignHCenter
        wrapMode: Text.WordWrap
        text: "Track start/stop and offboard controls land in later Milestone_1 tasks.\nPublish status on domain 42, e.g. ros2 topic pub /ah/system/status …"
        color: "#8a96a5"
        font.pixelSize: 13
      }

      Item { Layout.fillHeight: true }
    }
  }
}
