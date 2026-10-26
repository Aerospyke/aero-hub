import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Window 2.15

// One card per PX4 namespace. px4Vehicle (the first entry) still drives the hub.
// A click opens a detail window for that vehicle. A later click raises the same window.
Rectangle {
  id: root
  color: "#0d1620"

  property var detailComponent: null
  property var detailWindows: ({})

  function openVehicleDetail(vehicle) {
    if (!vehicle || !detailComponent || detailComponent.status !== Component.Ready)
      return
    var key = String(vehicle.vehicleNamespace)
    var windows = detailWindows
    var existing = windows[key]
    if (existing) {
      existing.show()
      existing.raise()
      existing.requestActivate()
      return
    }
    var win = detailComponent.createObject(root, { vehicle: vehicle })
    if (!win) {
      console.warn("Vehicle detail:", detailComponent.errorString())
      return
    }
    windows[key] = win
    detailWindows = windows
    win.show()
    win.raise()
    win.requestActivate()
  }

  Component.onCompleted: {
    detailComponent = Qt.createComponent("qrc:/qml/VehicleDetailWindow.qml", Component.PreferSynchronous, root)
    if (detailComponent.status === Component.Error)
      console.warn("Vehicle detail:", detailComponent.errorString())
  }
  border.color: "#243140"
  border.width: 1
  radius: 6

  readonly property color liveColor: "#6bcf7f"
  readonly property color staleColor: "#e6c35c"
  readonly property color waitColor: "#8a96a5"
  readonly property color valueColor: "#e8eef5"

  function valueColorFor(known, fresh) {
    if (!known)
      return "#5a6572"
    return fresh ? valueColor : staleColor
  }

  function meters(vehicle, valid, value) {
    if (!vehicle || !vehicle.hasOdometry || !valid)
      return "—"
    return Number(value).toFixed(2) + " m"
  }

  function headingText(vehicle) {
    if (!vehicle || !vehicle.attitudeValid)
      return "—"
    var h = Number(vehicle.headingDeg)
    if (h < 0)
      h = 0
    if (h >= 360)
      h = 0
    var body = h.toFixed(1)
    if (h < 10)
      return "00" + body + "°"
    if (h < 100)
      return "0" + body + "°"
    return body + "°"
  }

  function linkLabel(state) {
    if (state === "live")
      return "LIVE"
    if (state === "stale")
      return "STALE"
    return "WAITING"
  }

  function linkColor(state) {
    if (state === "live")
      return liveColor
    if (state === "stale")
      return staleColor
    return waitColor
  }

  component VehicleCard: Rectangle {
    id: card
    required property var modelData
    required property int index

    readonly property var vehicle: modelData
    readonly property string state: vehicle ? vehicle.linkState : "waiting"

    color: clickArea.containsMouse ? "#172433" : "#101820"
    radius: 4
    border.width: 1
    border.color: clickArea.containsMouse ? "#3d9eff"
                 : (state === "live" ? "#1f6b45" : (state === "stale" ? "#8a6a2f" : "#243140"))
    height: cardBody.implicitHeight + 16

    Column {
      id: cardBody
      anchors.left: parent.left
      anchors.right: parent.right
      anchors.top: parent.top
      anchors.margins: 8
      spacing: 3

      Row {
        width: parent.width
        spacing: 8

        Text {
          width: parent.width - linkText.implicitWidth - 8
          text: card.vehicle && card.vehicle.vehicleNamespace.length > 0
                ? card.vehicle.vehicleNamespace : "—"
          color: "#e8eef5"
          font.pixelSize: 15
          font.bold: true
          elide: Text.ElideRight
        }
        Text {
          id: linkText
          text: root.linkLabel(card.state)
          color: root.linkColor(card.state)
          font.pixelSize: 11
          font.bold: true
        }
      }

      Text {
        width: parent.width
        text: {
          if (!card.vehicle || !card.vehicle.hasStatus)
            return "—  ·  ID —"
          return card.vehicle.vehicleType + "  ·  ID " + card.vehicle.systemId
        }
        color: root.valueColorFor(card.vehicle && card.vehicle.hasStatus, card.vehicle && card.vehicle.statusFresh)
        font.pixelSize: 12
        elide: Text.ElideRight
      }

      Text {
        width: parent.width
        text: {
          if (!card.vehicle || !card.vehicle.hasStatus)
            return "Arming —    Nav —"
          return card.vehicle.armingState + "    " + card.vehicle.navState
        }
        color: root.valueColorFor(card.vehicle && card.vehicle.hasStatus, card.vehicle && card.vehicle.statusFresh)
        font.pixelSize: 12
        elide: Text.ElideRight
      }

      Text {
        width: parent.width
        text: {
          if (!card.vehicle)
            return "N —   E —   D —"
          return "N " + root.meters(card.vehicle, card.vehicle.positionValid, card.vehicle.northM)
              + "   E " + root.meters(card.vehicle, card.vehicle.positionValid, card.vehicle.eastM)
              + "   D " + root.meters(card.vehicle, card.vehicle.positionValid, card.vehicle.downM)
        }
        color: root.valueColorFor(card.vehicle && card.vehicle.positionValid, card.vehicle && card.vehicle.odometryFresh)
        font.pixelSize: 12
        elide: Text.ElideRight
      }

      Text {
        width: parent.width
        text: {
          if (!card.vehicle)
            return "Hdg —    GPS —"
          var fix = card.vehicle.hasGps ? card.vehicle.gpsFixType : "—"
          var sats = card.vehicle.hasGps ? String(card.vehicle.satellitesUsed) : "—"
          return "Hdg " + root.headingText(card.vehicle) + "    GPS " + fix + "    " + sats + " sats"
        }
        color: card.state === "stale" ? root.staleColor : (card.state === "waiting" ? "#5a6572" : "#c8d1dc")
        font.pixelSize: 12
        elide: Text.ElideRight
      }
    }

    MouseArea {
      id: clickArea
      anchors.fill: parent
      hoverEnabled: true
      cursorShape: Qt.PointingHandCursor
      onClicked: root.openVehicleDetail(card.vehicle)
    }
  }

  ColumnLayout {
    anchors.fill: parent
    anchors.margins: 12
    spacing: 8

    Text {
      text: "Autopilot (PX4)"
      color: "#c8d1dc"
      font.pixelSize: 18
      font.bold: true
      Layout.fillWidth: true
    }

    Flickable {
      id: flick
      Layout.fillWidth: true
      Layout.fillHeight: true
      contentHeight: cards.implicitHeight
      clip: true
      boundsBehavior: Flickable.StopAtBounds

      Column {
        id: cards
        width: flick.width
        spacing: 8

        Repeater {
          model: px4Vehicles
          delegate: VehicleCard {
            width: cards.width
          }
        }
      }
    }
  }
}
