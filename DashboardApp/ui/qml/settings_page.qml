import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15

// Stub: configuration / schema UI (later milestones).
// Camera / video source lives on Ops → Tracking (Task_32).
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
      anchors.centerIn: parent
      spacing: 12

      Text {
        Layout.alignment: Qt.AlignHCenter
        text: "Settings"
        color: "#c8d1dc"
        font.pixelSize: 28
        font.bold: true
      }

      Text {
        Layout.alignment: Qt.AlignHCenter
        Layout.maximumWidth: parent.parent.width * 0.65
        horizontalAlignment: Text.AlignHCenter
        wrapMode: Text.WordWrap
        text: "Camera / video source is on Ops → Tracking (below the track controls). This page remains for ROS domain, tracker preferences, and future schema-driven config."
        color: "#8a96a5"
        font.pixelSize: 16
      }
    }
  }
}
