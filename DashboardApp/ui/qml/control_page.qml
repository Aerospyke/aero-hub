import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15

// Stub: tracker / follower / offboard controls (Milestone_1 Task_16+)
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
        text: "Control"
        color: "#c8d1dc"
        font.pixelSize: 28
        font.bold: true
      }

      Text {
        Layout.alignment: Qt.AlignHCenter
        Layout.maximumWidth: parent.parent.width * 0.6
        horizontalAlignment: Text.AlignHCenter
        wrapMode: Text.WordWrap
        text: "Stub page for track start/stop, follower/offboard controls, and status actions.\nWire to ROS services in later Milestone_1 tasks."
        color: "#8a96a5"
        font.pixelSize: 16
      }
    }
  }
}
