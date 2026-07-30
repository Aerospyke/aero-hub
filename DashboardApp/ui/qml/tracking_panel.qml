import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15

// Tracking controls (start/stop/cancel + normalized box fields) — sits under Video on Ops.
Rectangle {
  id: root
  color: "#0d1620"
  border.color: "#243140"
  border.width: 1
  radius: 6
  clip: true

  readonly property color chromePanel: "#152030"
  readonly property color chromeBorder: "#243140"
  readonly property color chromeText: "#8a96a5"
  readonly property color chromeTextBright: "#e8eef5"
  readonly property color chromeInput: "#0a1018"

  component DarkSpinBox: SpinBox {
    id: control
    from: 0
    to: 100
    stepSize: 1
    editable: true
    font.pixelSize: 16
    implicitWidth: 88
    implicitHeight: 30

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
      implicitWidth: 26
      implicitHeight: 30
      color: control.up.pressed ? "#243140" : root.chromePanel
      border.color: root.chromeBorder
      Text {
        anchors.centerIn: parent
        text: "+"
        color: control.up.enabled ? root.chromeTextBright : "#5a6572"
        font.pixelSize: 13
      }
    }

    down.indicator: Rectangle {
      x: control.mirrored ? parent.width - width : 0
      height: parent.height
      implicitWidth: 26
      implicitHeight: 30
      color: control.down.pressed ? "#243140" : root.chromePanel
      border.color: root.chromeBorder
      Text {
        anchors.centerIn: parent
        text: "−"
        color: control.down.enabled ? root.chromeTextBright : "#5a6572"
        font.pixelSize: 13
      }
    }

    background: Rectangle {
      implicitWidth: 88
      implicitHeight: 30
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
    font.pixelSize: 16
    implicitHeight: 32
    implicitWidth: 80
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

  ColumnLayout {
    anchors.fill: parent
    anchors.margins: 12
    spacing: 10

    Text {
      Layout.fillWidth: true
      horizontalAlignment: Text.AlignHCenter
      text: "Tracking"
      color: root.chromeTextBright
      font.pixelSize: 24
      font.bold: true
    }

    Text {
      Layout.fillWidth: true
      horizontalAlignment: Text.AlignHCenter
      wrapMode: Text.WordWrap
      text: "Normalized box [0,1]. Drag on Video above, or edit fields."
      color: root.chromeText
      font.pixelSize: 15
    }

    GridLayout {
      Layout.fillWidth: true
      Layout.leftMargin: 12
      Layout.rightMargin: 12
      Layout.topMargin: 6
      Layout.bottomMargin: 6
      columns: 4
      columnSpacing: 14
      rowSpacing: 8

      Text {
        text: "x"
        color: root.chromeText
        font.pixelSize: 19
        Layout.fillWidth: true
        horizontalAlignment: Text.AlignHCenter
      }
      Text {
        text: "y"
        color: root.chromeText
        font.pixelSize: 19
        Layout.fillWidth: true
        horizontalAlignment: Text.AlignHCenter
      }
      Text {
        text: "width"
        color: root.chromeText
        font.pixelSize: 19
        Layout.fillWidth: true
        horizontalAlignment: Text.AlignHCenter
      }
      Text {
        text: "height"
        color: root.chromeText
        font.pixelSize: 19
        Layout.fillWidth: true
        horizontalAlignment: Text.AlignHCenter
      }

      DarkSpinBox {
        id: spinBoxX
        Layout.fillWidth: true
        Layout.leftMargin: 4
        Layout.rightMargin: 4
        Layout.topMargin: 4
        Layout.bottomMargin: 4
        value: 35
        onValueModified: trackController.trackingBoundingBoxX = value / 100.0
      }
      DarkSpinBox {
        id: spinBoxY
        Layout.fillWidth: true
        Layout.leftMargin: 4
        Layout.rightMargin: 4
        Layout.topMargin: 4
        Layout.bottomMargin: 4
        value: 35
        onValueModified: trackController.trackingBoundingBoxY = value / 100.0
      }
      DarkSpinBox {
        id: spinBoxWidth
        Layout.fillWidth: true
        Layout.leftMargin: 4
        Layout.rightMargin: 4
        Layout.topMargin: 4
        Layout.bottomMargin: 4
        from: 1
        value: 30
        onValueModified: trackController.trackingBoundingBoxWidth = value / 100.0
      }
      DarkSpinBox {
        id: spinBoxHeight
        Layout.fillWidth: true
        Layout.leftMargin: 4
        Layout.rightMargin: 4
        Layout.topMargin: 4
        Layout.bottomMargin: 4
        from: 1
        value: 30
        onValueModified: trackController.trackingBoundingBoxHeight = value / 100.0
      }
    }

    Connections {
      target: trackController
      function onTrackingBoundingBoxChanged() {
        spinBoxX.value = Math.round(trackController.trackingBoundingBoxX * 100)
        spinBoxY.value = Math.round(trackController.trackingBoundingBoxY * 100)
        spinBoxWidth.value = Math.round(trackController.trackingBoundingBoxWidth * 100)
        spinBoxHeight.value = Math.round(trackController.trackingBoundingBoxHeight * 100)
      }
    }

    Component.onCompleted: {
      spinBoxX.value = Math.round(trackController.trackingBoundingBoxX * 100)
      spinBoxY.value = Math.round(trackController.trackingBoundingBoxY * 100)
      spinBoxWidth.value = Math.round(trackController.trackingBoundingBoxWidth * 100)
      spinBoxHeight.value = Math.round(trackController.trackingBoundingBoxHeight * 100)
    }

    RowLayout {
      Layout.fillWidth: true
      spacing: 8

      Item { Layout.fillWidth: true }

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
      horizontalAlignment: Text.AlignHCenter
      wrapMode: Text.WordWrap
      text: (trackController.busy ? "Working… " : "") + trackController.lastMessage
      color: systemStatus.trackingStarted ? "#6bcf7f" : "#e6c35c"
      font.pixelSize: 19
    }

    Item { Layout.fillHeight: true }
  }
}
