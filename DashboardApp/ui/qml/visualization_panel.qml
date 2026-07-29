import QtQuick 2.15
import QtQuick.Layouts 1.15

// Live ROS camera: /ah/video/compressed (Task_18)
Rectangle {
  id: root
  color: "#0d1620"
  border.color: "#243140"
  border.width: 1
  radius: 6
  clip: true

  ColumnLayout {
    anchors.fill: parent
    anchors.margins: 10
    spacing: 8

    RowLayout {
      Layout.fillWidth: true
      spacing: 8

      Text {
        text: "Video"
        color: "#c8d1dc"
        font.pixelSize: 16
        font.bold: true
      }

      Text {
        text: videoFeed.hasFrame
              ? (videoFeed.frameWidth + "×" + videoFeed.frameHeight + "  #" + videoFeed.frameId)
              : "waiting for /ah/video/compressed…"
        color: videoFeed.hasFrame ? "#6bcf7f" : "#8a96a5"
        font.pixelSize: 12
        Layout.fillWidth: true
        elide: Text.ElideRight
      }
    }

    Rectangle {
      Layout.fillWidth: true
      Layout.fillHeight: true
      color: "#000000"
      radius: 4
      clip: true

      Image {
        id: videoImage
        anchors.fill: parent
        anchors.margins: 2
        fillMode: Image.PreserveAspectFit
        asynchronous: false
        cache: false
        // Bust cache each frame via frameId
        source: videoFeed.hasFrame ? ("image://ahvideo/frame/" + videoFeed.frameId) : ""
      }

      Text {
        anchors.centerIn: parent
        visible: !videoFeed.hasFrame
        text: "No video frame yet\n(run ah_core or publish CompressedImage)"
        color: "#5a6572"
        font.pixelSize: 13
        horizontalAlignment: Text.AlignHCenter
        lineHeight: 1.3
      }
    }
  }
}
