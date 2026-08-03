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
  readonly property color chromeAccent: "#3d9eff"

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

  Flickable {
    id: trackingFlick
    anchors.fill: parent
    anchors.margins: 12
    clip: true
    contentWidth: width
    contentHeight: trackingColumn.implicitHeight
    boundsBehavior: Flickable.StopAtBounds
    ScrollBar.vertical: ScrollBar {
      policy: trackingFlick.contentHeight > trackingFlick.height
              ? ScrollBar.AsNeeded : ScrollBar.AlwaysOff
    }

    ColumnLayout {
      id: trackingColumn
      width: trackingFlick.width
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

    RowLayout {
      Layout.fillWidth: true
      spacing: 8

      Item { Layout.fillWidth: true }

      DarkButton {
        text: systemStatus.aiTrackingActive ? "AI ON" : "AI OFF"
        enabled: !trackController.busy
        face: systemStatus.aiTrackingActive ? "#1a4a7a" : "#1a2430"
        faceDown: systemStatus.aiTrackingActive ? "#2a7acc" : "#243140"
        edge: systemStatus.aiTrackingActive ? root.chromeAccent : root.chromeBorder
        onClicked: trackController.SetAiTrackingMode(!systemStatus.aiTrackingActive)
      }
      DarkButton {
        text: "Start"
        enabled: !trackController.busy && !systemStatus.aiTrackingActive
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
      text: systemStatus.aiTrackingActive
            ? "AI tracking: click a cyan detection to lock (drag disabled)"
            : "Classic: drag box on video, then Start"
      color: systemStatus.aiTrackingActive ? "#9ecbff" : root.chromeText
      font.pixelSize: 13
    }

    Text {
      Layout.fillWidth: true
      horizontalAlignment: Text.AlignHCenter
      wrapMode: Text.WordWrap
      text: (trackController.busy ? "Working… " : "") + trackController.lastMessage
      color: systemStatus.trackingStarted ? "#6bcf7f" : "#e6c35c"
      font.pixelSize: 19
    }

    // --- Camera / video source (Task_32; list/select via ah_core) ---
    Rectangle {
      Layout.fillWidth: true
      Layout.preferredHeight: cameraColumn.implicitHeight + 20
      radius: 4
      color: root.chromePanel
      border.color: root.chromeBorder
      border.width: 1

      ColumnLayout {
        id: cameraColumn
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: 10
        spacing: 8

        Text {
          text: "Camera / video source"
          color: root.chromeTextBright
          font.pixelSize: 16
          font.bold: true
        }

        Text {
          Layout.fillWidth: true
          wrapMode: Text.WordWrap
          text: cameraController
                ? ("Current: " + cameraController.currentVideoSource
                   + (cameraController.currentDevicePath.length
                      ? " · " + cameraController.currentDevicePath : "")
                   + (cameraController.currentBackend.length
                      ? " · " + cameraController.currentBackend : ""))
                : "Current: —"
          color: root.chromeText
          font.pixelSize: 13
        }

        ComboBox {
          id: deviceCombo
          Layout.fillWidth: true
          Layout.preferredHeight: 32
          enabled: cameraController && !cameraController.busy
                   && cameraController.deviceCount > 0
          model: cameraController ? cameraController.deviceLabels : []
          currentIndex: cameraController ? cameraController.selectedListIndex : -1

          Connections {
            target: cameraController
            function onDevicesChanged() {
              deviceCombo.model = cameraController.deviceLabels
              deviceCombo.currentIndex = cameraController.selectedListIndex
            }
            function onSelectionChanged() {
              deviceCombo.currentIndex = cameraController.selectedListIndex
            }
          }

          contentItem: Text {
            leftPadding: 8
            rightPadding: deviceCombo.indicator.width + 6
            text: deviceCombo.displayText
            font.pixelSize: 13
            color: root.chromeTextBright
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
          }
          background: Rectangle {
            radius: 4
            color: root.chromeInput
            border.color: root.chromeBorder
            border.width: 1
          }
          popup: Popup {
            y: deviceCombo.height
            width: deviceCombo.width
            implicitHeight: Math.min(contentItem.implicitHeight, 220)
            padding: 1
            contentItem: ListView {
              clip: true
              implicitHeight: contentHeight
              model: deviceCombo.popup.visible ? deviceCombo.delegateModel : null
              currentIndex: deviceCombo.highlightedIndex
              ScrollIndicator.vertical: ScrollIndicator {}
            }
            background: Rectangle {
              color: root.chromePanel
              border.color: root.chromeBorder
              radius: 4
            }
          }
          delegate: ItemDelegate {
            width: deviceCombo.width
            height: 30
            contentItem: Text {
              text: modelData
              color: root.chromeTextBright
              font.pixelSize: 12
              elide: Text.ElideRight
              verticalAlignment: Text.AlignVCenter
              leftPadding: 8
            }
            background: Rectangle {
              color: parent.highlighted ? "#243140" : "transparent"
            }
          }
        }

        RowLayout {
          Layout.fillWidth: true
          spacing: 8

          DarkButton {
            text: "Refresh"
            enabled: cameraController && !cameraController.busy
            face: "#1a2430"
            faceDown: "#243140"
            edge: root.chromeBorder
            font.bold: false
            onClicked: {
              if (cameraController)
                cameraController.RefreshDevices(true)
            }
          }
          DarkButton {
            text: "Apply"
            enabled: cameraController && !cameraController.busy
                     && deviceCombo.currentIndex >= 0
            face: "#1a4a7a"
            faceDown: "#2a7acc"
            edge: root.chromeAccent
            onClicked: {
              if (cameraController && deviceCombo.currentIndex >= 0)
                cameraController.SelectDeviceAt(deviceCombo.currentIndex)
            }
          }

          Item { Layout.fillWidth: true }

          Text {
            visible: cameraController && cameraController.busy
            text: "Working…"
            color: root.chromeAccent
            font.pixelSize: 12
          }
        }

        Text {
          Layout.fillWidth: true
          wrapMode: Text.WordWrap
          text: cameraController ? cameraController.lastMessage : ""
          color: cameraController && cameraController.lastSuccess
                 ? "#7dcea0" : "#c9a06a"
          font.pixelSize: 12
        }
      }
    }

    // --- YOLO detector profile (runtime switch via ah_yolo) ---
    // Panel chrome like camera block; content centered like tracking controls above.
    Rectangle {
      id: yoloPanel
      Layout.fillWidth: true
      Layout.preferredHeight: yoloColumn.implicitHeight + 20
      radius: 4
      color: root.chromePanel
      border.color: root.chromeBorder
      border.width: 1

      ColumnLayout {
        id: yoloColumn
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: 10
        spacing: 8

        Text {
          Layout.fillWidth: true
          horizontalAlignment: Text.AlignHCenter
          text: "YOLO model"
          color: root.chromeTextBright
          font.pixelSize: 19
          font.bold: true
        }

        Text {
          Layout.fillWidth: true
          horizontalAlignment: Text.AlignHCenter
          wrapMode: Text.WordWrap
          text: {
            if (!yoloController)
              return "Active: —"
            const p = yoloController.activeProfile.length
                      ? yoloController.activeProfile
                      : (detectionsModel ? detectionsModel.profile : "—")
            const w = yoloController.activeWeights.length
                      ? yoloController.activeWeights.split("/").pop()
                      : ""
            return w.length ? ("Active: " + p + " · " + w) : ("Active: " + p)
          }
          color: root.chromeText
          font.pixelSize: 15
        }

        ComboBox {
          id: yoloProfileCombo
          Layout.alignment: Qt.AlignHCenter
          Layout.fillWidth: false
          // Fitted to longest option (see recomputeFittedWidth); never exceed panel.
          Layout.preferredWidth: Math.min(yoloColumn.width, fittedWidth)
          Layout.preferredHeight: 32
          enabled: yoloController && !yoloController.busy
          model: yoloController ? yoloController.profileLabels : []
          currentIndex: yoloController ? yoloController.selectedListIndex : 0
          font.pixelSize: 16
          leftPadding: 10
          rightPadding: 8

          // Qt Quick text measurement: TextMetrics with the *same* font as this control.
          // (Assigning metrics.text inside a property binding is unreliable; recompute explicitly.)
          TextMetrics {
            id: yoloLabelMetrics
            font: yoloProfileCombo.font
          }

          // Width that fits the longest option + chrome (indicator / padding).
          property real fittedWidth: 160

          function recomputeFittedWidth() {
            const labels = yoloController ? yoloController.profileLabels : []
            let maxText = 0
            for (let i = 0; i < labels.length; ++i) {
              yoloLabelMetrics.font = yoloProfileCombo.font
              yoloLabelMetrics.text = String(labels[i])
              // Prefer advance width; fall back to bounding rect (covers overhanging glyphs).
              const advance = yoloLabelMetrics.advanceWidth
                              ? yoloLabelMetrics.advanceWidth
                              : yoloLabelMetrics.width
              const bound = yoloLabelMetrics.boundingRect
                            ? yoloLabelMetrics.boundingRect.width
                            : advance
              maxText = Math.max(maxText, advance, bound)
            }
            const indicatorW = indicator && indicator.width > 0 ? indicator.width : 24
            // leftPadding + text + gap + indicator + rightPadding + border
            const need = Math.ceil(
                leftPadding + maxText + 8 + indicatorW + rightPadding + 2)
            fittedWidth = Math.max(120, need)
          }

          Component.onCompleted: Qt.callLater(recomputeFittedWidth)
          onModelChanged: recomputeFittedWidth()
          onFontChanged: recomputeFittedWidth()
          // Indicator width often settles after first layout pass.
          onImplicitIndicatorWidthChanged: recomputeFittedWidth()

          Connections {
            target: yoloController
            function onProfileChanged() {
              yoloProfileCombo.currentIndex = yoloController.selectedListIndex
              yoloProfileCombo.recomputeFittedWidth()
            }
          }
          Connections {
            target: detectionsModel
            function onMetaChanged() {
              if (yoloController && detectionsModel && detectionsModel.profile.length)
                yoloController.SyncFromProfileName(detectionsModel.profile)
            }
          }

          contentItem: Text {
            leftPadding: yoloProfileCombo.leftPadding
            rightPadding: yoloProfileCombo.indicator.width + yoloProfileCombo.rightPadding
            text: yoloProfileCombo.displayText
            font: yoloProfileCombo.font
            color: root.chromeTextBright
            verticalAlignment: Text.AlignVCenter
            // Only elide if panel forces us narrower than the fitted width.
            elide: yoloProfileCombo.width + 0.5 < yoloProfileCombo.fittedWidth
                   ? Text.ElideRight : Text.ElideNone
          }
          background: Rectangle {
            radius: 4
            color: root.chromeInput
            border.color: root.chromeBorder
            border.width: 1
          }
          popup: Popup {
            y: yoloProfileCombo.height
            width: Math.max(yoloProfileCombo.width, yoloProfileCombo.fittedWidth)
            implicitHeight: Math.min(contentItem.implicitHeight, 160)
            padding: 1
            contentItem: ListView {
              clip: true
              implicitHeight: contentHeight
              model: yoloProfileCombo.popup.visible ? yoloProfileCombo.delegateModel : null
              currentIndex: yoloProfileCombo.highlightedIndex
              ScrollIndicator.vertical: ScrollIndicator {}
            }
            background: Rectangle {
              color: root.chromePanel
              border.color: root.chromeBorder
              radius: 4
            }
          }
          delegate: ItemDelegate {
            width: Math.max(yoloProfileCombo.width, yoloProfileCombo.fittedWidth)
            height: 32
            contentItem: Text {
              text: modelData
              color: root.chromeTextBright
              font: yoloProfileCombo.font
              elide: Text.ElideNone
              verticalAlignment: Text.AlignVCenter
              leftPadding: yoloProfileCombo.leftPadding
            }
            background: Rectangle {
              color: parent.highlighted ? "#243140" : "transparent"
            }
          }
        }

        RowLayout {
          Layout.fillWidth: true
          spacing: 8

          Item { Layout.fillWidth: true }

          DarkButton {
            id: applyYoloButton
            text: "Apply Model"
            enabled: yoloController && !yoloController.busy
                     && yoloProfileCombo.currentIndex >= 0
            face: "#1a4a7a"
            faceDown: "#2a7acc"
            edge: root.chromeAccent
            // Default DarkButton width (80) is too narrow for this label.
            implicitWidth: Math.max(120, Math.ceil(applyYoloLabelMetrics.width) + 28)
            onClicked: {
              if (yoloController && yoloProfileCombo.currentIndex >= 0)
                yoloController.SetProfileAt(yoloProfileCombo.currentIndex)
            }
            TextMetrics {
              id: applyYoloLabelMetrics
              font: applyYoloButton.font
              text: applyYoloButton.text
            }
          }

          Text {
            visible: yoloController && yoloController.busy
            text: "Loading…"
            color: root.chromeAccent
            font.pixelSize: 15
            Layout.alignment: Qt.AlignVCenter
          }

          Item { Layout.fillWidth: true }
        }

        Text {
          Layout.fillWidth: true
          horizontalAlignment: Text.AlignHCenter
          wrapMode: Text.WordWrap
          text: yoloController
                ? ((yoloController.busy ? "Working… " : "") + yoloController.lastMessage)
                : ""
          color: yoloController && yoloController.lastSuccess ? "#6bcf7f" : "#e6c35c"
          font.pixelSize: 15
        }

        Text {
          Layout.fillWidth: true
          horizontalAlignment: Text.AlignHCenter
          wrapMode: Text.WordWrap
          text: "Requires ah_yolo running. Switch resets track IDs — re-lock after change."
          color: root.chromeText
          font.pixelSize: 13
        }
      }
    }

  }
  }

  Component.onCompleted: {
    spinBoxX.value = Math.round(trackController.trackingBoundingBoxX * 100)
    spinBoxY.value = Math.round(trackController.trackingBoundingBoxY * 100)
    spinBoxWidth.value = Math.round(trackController.trackingBoundingBoxWidth * 100)
    spinBoxHeight.value = Math.round(trackController.trackingBoundingBoxHeight * 100)
    if (cameraController)
      cameraController.RefreshDevices(false)
  }
}
