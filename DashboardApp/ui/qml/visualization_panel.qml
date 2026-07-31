import QtQuick 2.15
import QtQuick.Layouts 1.15

// Live ROS camera + YOLO boxes (Task_34) + drag tracking box (Task_18 / Task_19)
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
        text: !videoFeed.hasFrame
              ? "waiting for /ah/video/compressed…"
              : (videoFeed.frameLive
                 ? (videoFeed.frameWidth + "×" + videoFeed.frameHeight + "  #" + videoFeed.frameId)
                 : "STALE (no new frames — is ah_core running?)")
        color: !videoFeed.hasFrame ? "#8a96a5"
             : (videoFeed.frameLive ? "#6bcf7f" : "#e6c35c")
        font.pixelSize: 12
        Layout.fillWidth: true
        elide: Text.ElideRight
      }

      Text {
        text: detectionsModel ? detectionsModel.summary : ""
        color: detectionsModel && detectionsModel.live ? "#6bcf7f" : "#5a6572"
        font.pixelSize: 11
        visible: detectionsModel !== null
      }

      Text {
        text: "drag box · controls below"
        color: "#5a6572"
        font.pixelSize: 11
        visible: videoFeed.hasFrame
      }
    }

    Rectangle {
      id: stage
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
        opacity: videoFeed.frameLive || !videoFeed.hasFrame ? 1.0 : 0.55
        source: videoFeed.hasFrame ? ("image://ahvideo/frame/" + videoFeed.frameId) : ""
      }

      // Painted area of the image (letterboxed inside stage)
      Item {
        id: imageArea
        readonly property real imgAspect: videoFeed.hasFrame && videoFeed.frameHeight > 0
                                          ? videoFeed.frameWidth / videoFeed.frameHeight
                                          : 16 / 9
        readonly property real areaAspect: stage.width / Math.max(stage.height, 1)
        width: areaAspect > imgAspect ? stage.height * imgAspect : stage.width
        height: areaAspect > imgAspect ? stage.height : stage.width / imgAspect
        anchors.centerIn: parent

        // YOLO detections (normalized [0,1] → imageArea pixels) — Task_34
        Repeater {
          model: detectionsModel
          delegate: Rectangle {
            required property real nx
            required property real ny
            required property real nw
            required property real nh
            required property string label
            required property real confidence

            x: nx * imageArea.width
            y: ny * imageArea.height
            width: Math.max(2, nw * imageArea.width)
            height: Math.max(2, nh * imageArea.height)
            color: "transparent"
            border.color: "#3d9eff"
            border.width: 2
            visible: videoFeed.hasFrame && detectionsModel.live
            z: 1

            Rectangle {
              anchors.left: parent.left
              anchors.bottom: parent.top
              anchors.bottomMargin: 1
              height: lab.implicitHeight + 2
              width: Math.min(imageArea.width - parent.x, lab.implicitWidth + 8)
              color: "#cc0a1520"
              visible: lab.text.length > 0
              Text {
                id: lab
                anchors.centerIn: parent
                text: label
                color: "#9ecbff"
                font.pixelSize: 11
                font.bold: true
                elide: Text.ElideRight
                width: parent.width - 6
              }
            }
          }
        }

        // Selection rectangle (normalized tracking bounding box → pixels in imageArea)
        Rectangle {
          id: trackingBoundingBoxRect
          x: trackController.trackingBoundingBoxX * imageArea.width
          y: trackController.trackingBoundingBoxY * imageArea.height
          width: trackController.trackingBoundingBoxWidth * imageArea.width
          height: trackController.trackingBoundingBoxHeight * imageArea.height
          color: systemStatus.trackingStarted ? "#336bcf7f" : "#33e6c35c"
          border.color: systemStatus.trackingStarted ? "#6bcf7f" : "#e6c35c"
          border.width: 2
          visible: videoFeed.hasFrame
          z: 2
        }

        MouseArea {
          id: dragArea
          anchors.fill: parent
          enabled: videoFeed.hasFrame && !trackController.busy
          property real startX: 0
          property real startY: 0

          onPressed: function (mouse) {
            startX = mouse.x
            startY = mouse.y
            trackController.trackingBoundingBoxX = mouse.x / width
            trackController.trackingBoundingBoxY = mouse.y / height
            trackController.trackingBoundingBoxWidth = 0.01
            trackController.trackingBoundingBoxHeight = 0.01
          }
          onPositionChanged: function (mouse) {
            if (!pressed)
              return
            const x0 = Math.min(startX, mouse.x)
            const y0 = Math.min(startY, mouse.y)
            const x1 = Math.max(startX, mouse.x)
            const y1 = Math.max(startY, mouse.y)
            trackController.trackingBoundingBoxX = Math.max(0, Math.min(1, x0 / width))
            trackController.trackingBoundingBoxY = Math.max(0, Math.min(1, y0 / height))
            trackController.trackingBoundingBoxWidth = Math.max(0.01, Math.min(1 - trackController.trackingBoundingBoxX, (x1 - x0) / width))
            trackController.trackingBoundingBoxHeight = Math.max(0.01, Math.min(1 - trackController.trackingBoundingBoxY, (y1 - y0) / height))
          }
        }
      }

      Text {
        anchors.centerIn: parent
        visible: !videoFeed.hasFrame
        text: "No video frame yet\n(run ah_core on domain 42)"
        color: "#5a6572"
        font.pixelSize: 13
        horizontalAlignment: Text.AlignHCenter
        lineHeight: 1.3
      }
    }
  }
}
