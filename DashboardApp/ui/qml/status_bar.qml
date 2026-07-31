import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15

// Footer status chips (Task_17) — tracking / video / ai tracking / following.
Rectangle {
  id: root
  implicitHeight: 40
  color: "#0d1620"
  border.color: "#243140"
  border.width: 0

  // Top edge separator
  Rectangle {
    anchors.left: parent.left
    anchors.right: parent.right
    anchors.top: parent.top
    height: 1
    color: "#243140"
  }

  RowLayout {
    anchors.fill: parent
    anchors.leftMargin: 16
    anchors.rightMargin: 16
    spacing: 10

    Text {
      // ROS: green=live, amber=waiting, red=lost contact with ah_core
      text: systemStatus.linkState === "lost" ? "ROS LOST" : "ROS"
      color: systemStatus.linkState === "live" ? "#6bcf7f"
           : (systemStatus.linkState === "lost" ? "#e07a7a" : "#e6c35c")
      font.pixelSize: 12
      font.bold: true
      Layout.alignment: Qt.AlignVCenter
    }

    Item {
      width: 8
    }

    // Shared chip chrome
    component StatusChip: Rectangle {
      id: chip
      property string label: ""
      property string value: ""
      property color accent: "#8a96a5"

      implicitHeight: 26
      implicitWidth: chipRow.implicitWidth + 20
      radius: 13
      color: "#152030"
      border.color: accent
      border.width: 1

      Row {
        id: chipRow
        anchors.centerIn: parent
        spacing: 6

        Text {
          text: chip.label
          color: "#8a96a5"
          font.pixelSize: 12
          anchors.verticalCenter: parent.verticalCenter
        }
        Text {
          text: chip.value
          color: chip.accent
          font.pixelSize: 12
          font.bold: true
          anchors.verticalCenter: parent.verticalCenter
        }
      }
    }

    StatusChip {
      label: "Tracking"
      value: systemStatus.trackingStarted ? "ON" : "OFF"
      accent: systemStatus.trackingStarted ? "#6bcf7f" : "#8a96a5"
    }

    StatusChip {
      label: "Video"
      value: !systemStatus.connected ? "—"
           : (!videoFeed.frameLive && videoFeed.hasFrame ? "stale"
           : systemStatus.videoStatus)
      accent: {
        if (!systemStatus.connected)
          return "#8a96a5"
        if (!videoFeed.frameLive && videoFeed.hasFrame)
          return "#e6c35c"
        const v = systemStatus.videoStatus
        if (v === "connected" || v === "ok")
          return "#6bcf7f"
        if (v === "degraded")
          return "#e6c35c"
        if (v === "unavailable" || v === "unknown")
          return "#e07a7a"
        return "#c8d1dc"
      }
    }

    StatusChip {
      label: "AI track"
      value: systemStatus.aiTrackingActive ? "ON" : "OFF"
      accent: systemStatus.aiTrackingActive ? "#6bcf7f" : "#8a96a5"
    }

    StatusChip {
      label: "Following"
      value: systemStatus.followingActive ? "ON" : "OFF"
      accent: systemStatus.followingActive ? "#6bcf7f" : "#8a96a5"
    }

    Item {
      Layout.fillWidth: true
    }

    Text {
      visible: systemStatus.connected
      text: "stamp " + Number(systemStatus.stamp).toFixed(1)
      color: "#5a6572"
      font.pixelSize: 11
      Layout.alignment: Qt.AlignVCenter
    }
  }
}
