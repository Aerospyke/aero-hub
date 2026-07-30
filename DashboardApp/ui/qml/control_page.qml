import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15

// Control: system status detail (tracking commands live on Ops under Video).
Item {
  id: root

  readonly property color chromeBg: "#0d1620"
  readonly property color chromeBorder: "#243140"
  readonly property color chromeText: "#8a96a5"
  readonly property color chromeTextBright: "#e8eef5"

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
        Layout.fillWidth: true
        horizontalAlignment: Text.AlignHCenter
        wrapMode: Text.WordWrap
        text: systemStatus.connectionMessage
        color: systemStatus.linkState === "live" ? "#6bcf7f"
             : (systemStatus.linkState === "lost" ? "#e07a7a" : root.chromeText)
        font.pixelSize: 14
      }

      Text {
        Layout.alignment: Qt.AlignHCenter
        Layout.fillWidth: true
        horizontalAlignment: Text.AlignHCenter
        wrapMode: Text.WordWrap
        text: "Tracking Start / Stop / Cancel and the box fields are on Ops (below Video)."
        color: root.chromeText
        font.pixelSize: 13
      }

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
