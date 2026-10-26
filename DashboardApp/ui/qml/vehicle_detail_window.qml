import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Layouts 1.15

// Live detail for one PX4 namespace. The vehicle object is an AhPx4Telemetry.
Window {
  id: detail
  property var vehicle: null

  width: 440
  height: 620
  minimumWidth: 360
  minimumHeight: 480
  visible: false
  color: "#0d1620"
  title: vehicle && vehicle.vehicleNamespace.length > 0
         ? (vehicle.vehicleNamespace + " — telemetry")
         : "Telemetry"

  readonly property color liveColor: "#6bcf7f"
  readonly property color staleColor: "#e6c35c"
  readonly property color waitColor: "#8a96a5"
  readonly property color valueColor: "#e8eef5"
  readonly property string state: vehicle ? vehicle.linkState : "waiting"

  function valueColorFor(known, fresh) {
    if (!known)
      return "#5a6572"
    return fresh ? valueColor : staleColor
  }

  function meters(valid, value) {
    if (!vehicle || !vehicle.hasOdometry || !valid)
      return "—"
    return Number(value).toFixed(2) + " m"
  }

  function headingText() {
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

  function signedDeg(valid, value) {
    if (!vehicle || !valid)
      return "—"
    return Number(value).toFixed(1) + "°"
  }

  function linkLabel(linkState) {
    if (linkState === "live")
      return "LIVE"
    if (linkState === "stale")
      return "STALE"
    return "WAITING"
  }

  function linkColor(linkState) {
    if (linkState === "live")
      return liveColor
    if (linkState === "stale")
      return staleColor
    return waitColor
  }

  component Readout: RowLayout {
    property string label: ""
    property string value: ""
    property color valueColor: "#e8eef5"
    Layout.fillWidth: true
    spacing: 8

    Text {
      text: label
      color: "#8a96a5"
      font.pixelSize: 14
      Layout.preferredWidth: 140
    }
    Text {
      text: value
      color: parent.valueColor
      font.pixelSize: 16
      font.bold: true
      Layout.fillWidth: true
      elide: Text.ElideRight
    }
  }

  component SectionTitle: Text {
    property string title: ""
    property bool known: false
    property bool fresh: false
    text: title + (known && !fresh ? "  · stale" : "")
    color: fresh ? "#8a96a5" : (known ? "#e6c35c" : "#5a6572")
    font.pixelSize: 12
    font.bold: true
    font.capitalization: Font.AllUppercase
    Layout.fillWidth: true
    Layout.topMargin: 8
  }

  ColumnLayout {
    anchors.fill: parent
    anchors.margins: 20
    spacing: 6

    RowLayout {
      Layout.fillWidth: true
      spacing: 8

      Text {
        text: detail.vehicle && detail.vehicle.vehicleNamespace.length > 0
              ? detail.vehicle.vehicleNamespace : "—"
        color: "#e8eef5"
        font.pixelSize: 22
        font.bold: true
        Layout.fillWidth: true
        elide: Text.ElideRight
      }
      Text {
        text: detail.linkLabel(detail.state)
        color: detail.linkColor(detail.state)
        font.pixelSize: 13
        font.bold: true
      }
    }

    Text {
      Layout.fillWidth: true
      text: {
        if (!detail.vehicle || !detail.vehicle.hasStatus)
          return "—  ·  ID —"
        return detail.vehicle.vehicleType + "  ·  ID " + detail.vehicle.systemId
      }
      color: detail.valueColorFor(detail.vehicle && detail.vehicle.hasStatus,
                                   detail.vehicle && detail.vehicle.statusFresh)
      font.pixelSize: 14
      elide: Text.ElideRight
    }

    SectionTitle {
      title: "Position"
      known: detail.vehicle && detail.vehicle.hasOdometry
      fresh: detail.vehicle && detail.vehicle.odometryFresh
    }
    Readout {
      label: "North"
      value: detail.meters(detail.vehicle && detail.vehicle.positionValid,
                           detail.vehicle ? detail.vehicle.northM : 0)
      valueColor: detail.valueColorFor(detail.vehicle && detail.vehicle.positionValid,
                                        detail.vehicle && detail.vehicle.odometryFresh)
    }
    Readout {
      label: "East"
      value: detail.meters(detail.vehicle && detail.vehicle.positionValid,
                           detail.vehicle ? detail.vehicle.eastM : 0)
      valueColor: detail.valueColorFor(detail.vehicle && detail.vehicle.positionValid,
                                        detail.vehicle && detail.vehicle.odometryFresh)
    }
    Readout {
      label: "Down"
      value: detail.meters(detail.vehicle && detail.vehicle.positionValid,
                           detail.vehicle ? detail.vehicle.downM : 0)
      valueColor: detail.valueColorFor(detail.vehicle && detail.vehicle.positionValid,
                                        detail.vehicle && detail.vehicle.odometryFresh)
    }

    SectionTitle {
      title: "Attitude"
      known: detail.vehicle && detail.vehicle.attitudeValid
      fresh: detail.vehicle && detail.vehicle.odometryFresh
    }
    Readout {
      label: "Heading"
      value: detail.headingText()
      valueColor: detail.valueColorFor(detail.vehicle && detail.vehicle.attitudeValid,
                                        detail.vehicle && detail.vehicle.odometryFresh)
    }
    Readout {
      label: "Roll"
      value: detail.signedDeg(detail.vehicle && detail.vehicle.attitudeValid,
                              detail.vehicle ? detail.vehicle.rollDeg : 0)
      valueColor: detail.valueColorFor(detail.vehicle && detail.vehicle.attitudeValid,
                                        detail.vehicle && detail.vehicle.odometryFresh)
    }
    Readout {
      label: "Pitch"
      value: detail.signedDeg(detail.vehicle && detail.vehicle.attitudeValid,
                              detail.vehicle ? detail.vehicle.pitchDeg : 0)
      valueColor: detail.valueColorFor(detail.vehicle && detail.vehicle.attitudeValid,
                                        detail.vehicle && detail.vehicle.odometryFresh)
    }

    SectionTitle {
      title: "State"
      known: detail.vehicle && detail.vehicle.hasStatus
      fresh: detail.vehicle && detail.vehicle.statusFresh
    }
    Readout {
      label: "Arming"
      value: detail.vehicle && detail.vehicle.hasStatus ? detail.vehicle.armingState : "—"
      valueColor: detail.valueColorFor(detail.vehicle && detail.vehicle.hasStatus,
                                        detail.vehicle && detail.vehicle.statusFresh)
    }
    Readout {
      label: "Navigation"
      value: detail.vehicle && detail.vehicle.hasStatus ? detail.vehicle.navState : "—"
      valueColor: detail.valueColorFor(detail.vehicle && detail.vehicle.hasStatus,
                                        detail.vehicle && detail.vehicle.statusFresh)
    }

    SectionTitle {
      title: "GPS"
      known: detail.vehicle && detail.vehicle.hasGps
      fresh: detail.vehicle && detail.vehicle.gpsFresh
    }
    Readout {
      label: "Fix"
      value: detail.vehicle && detail.vehicle.hasGps ? detail.vehicle.gpsFixType : "—"
      valueColor: detail.valueColorFor(detail.vehicle && detail.vehicle.hasGps,
                                        detail.vehicle && detail.vehicle.gpsFresh)
    }
    Readout {
      label: "Satellites"
      value: detail.vehicle && detail.vehicle.hasGps ? String(detail.vehicle.satellitesUsed) : "—"
      valueColor: detail.valueColorFor(detail.vehicle && detail.vehicle.hasGps,
                                        detail.vehicle && detail.vehicle.gpsFresh)
    }

    Item {
      Layout.fillHeight: true
    }
  }
}
