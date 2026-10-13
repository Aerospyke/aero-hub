import QtQuick 2.15
import QtQuick.Layouts 1.15

// Live PX4 state from /<vehicle>/fmu/out (odometry, status, GPS).
Rectangle {
  id: root
  color: "#0d1620"
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

  function meters(valid, value) {
    if (!px4Vehicle.hasOdometry || !valid)
      return "—"
    return Number(value).toFixed(2) + " m"
  }

  function signedDeg(valid, value) {
    if (!px4Vehicle.attitudeValid)
      return "—"
    return Number(value).toFixed(1) + "°"
  }

  function headingText() {
    if (!px4Vehicle.attitudeValid)
      return "—"
    var h = Number(px4Vehicle.headingDeg)
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

  component Readout: Row {
    property string label: ""
    property string value: ""
    property color valueColor: "#e8eef5"
    width: parent.width
    spacing: 8

    Text {
      width: 118
      text: label
      color: "#8a96a5"
      font.pixelSize: 13
    }
    Text {
      text: value
      color: parent.valueColor
      font.pixelSize: 14
      font.bold: true
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
  }

  ColumnLayout {
    anchors.fill: parent
    anchors.margins: 12
    spacing: 8

    RowLayout {
      Layout.fillWidth: true
      spacing: 8

      Text {
        text: "Autopilot (PX4)"
        color: "#c8d1dc"
        font.pixelSize: 18
        font.bold: true
      }

      Item {
        Layout.fillWidth: true
      }

      Text {
        text: px4Vehicle.linkState === "live" ? "LIVE"
            : (px4Vehicle.linkState === "stale" ? "STALE" : "WAITING")
        color: px4Vehicle.linkState === "live" ? root.liveColor
             : (px4Vehicle.linkState === "stale" ? root.staleColor : root.waitColor)
        font.pixelSize: 12
        font.bold: true
        Layout.alignment: Qt.AlignVCenter
      }

      Text {
        text: px4Vehicle.vehicleNamespace.length > 0 ? ("/" + px4Vehicle.vehicleNamespace) : "/"
        color: "#5a6572"
        font.pixelSize: 12
        Layout.alignment: Qt.AlignVCenter
      }
    }

    Flickable {
      Layout.fillWidth: true
      Layout.fillHeight: true
      contentHeight: body.implicitHeight
      clip: true
      boundsBehavior: Flickable.StopAtBounds

      Column {
        id: body
        width: parent.width
        spacing: 8

        SectionTitle {
          title: "Position"
          known: px4Vehicle.hasOdometry
          fresh: px4Vehicle.odometryFresh
        }
        Readout {
          label: "North"
          value: root.meters(px4Vehicle.positionValid, px4Vehicle.northM)
          valueColor: root.valueColorFor(px4Vehicle.positionValid, px4Vehicle.odometryFresh)
        }
        Readout {
          label: "East"
          value: root.meters(px4Vehicle.positionValid, px4Vehicle.eastM)
          valueColor: root.valueColorFor(px4Vehicle.positionValid, px4Vehicle.odometryFresh)
        }
        Readout {
          label: "Down"
          value: root.meters(px4Vehicle.positionValid, px4Vehicle.downM)
          valueColor: root.valueColorFor(px4Vehicle.positionValid, px4Vehicle.odometryFresh)
        }

        Item {
          width: 1
          height: 4
        }

        SectionTitle {
          title: "Attitude"
          known: px4Vehicle.attitudeValid
          fresh: px4Vehicle.odometryFresh
        }
        Readout {
          label: "Heading"
          value: root.headingText()
          valueColor: root.valueColorFor(px4Vehicle.attitudeValid, px4Vehicle.odometryFresh)
        }
        Readout {
          label: "Roll"
          value: root.signedDeg(px4Vehicle.attitudeValid, px4Vehicle.rollDeg)
          valueColor: root.valueColorFor(px4Vehicle.attitudeValid, px4Vehicle.odometryFresh)
        }
        Readout {
          label: "Pitch"
          value: root.signedDeg(px4Vehicle.attitudeValid, px4Vehicle.pitchDeg)
          valueColor: root.valueColorFor(px4Vehicle.attitudeValid, px4Vehicle.odometryFresh)
        }

        Item {
          width: 1
          height: 4
        }

        SectionTitle {
          title: "State"
          known: px4Vehicle.hasStatus
          fresh: px4Vehicle.statusFresh
        }
        Readout {
          label: "Arming"
          value: px4Vehicle.hasStatus ? px4Vehicle.armingState : "—"
          valueColor: root.valueColorFor(px4Vehicle.hasStatus, px4Vehicle.statusFresh)
        }
        Readout {
          label: "Navigation"
          value: px4Vehicle.hasStatus ? px4Vehicle.navState : "—"
          valueColor: root.valueColorFor(px4Vehicle.hasStatus, px4Vehicle.statusFresh)
        }

        Item {
          width: 1
          height: 4
        }

        SectionTitle {
          title: "GPS"
          known: px4Vehicle.hasGps
          fresh: px4Vehicle.gpsFresh
        }
        Readout {
          label: "Fix"
          value: px4Vehicle.hasGps ? px4Vehicle.gpsFixType : "—"
          valueColor: root.valueColorFor(px4Vehicle.hasGps, px4Vehicle.gpsFresh)
        }
        Readout {
          label: "Satellites"
          value: px4Vehicle.hasGps ? String(px4Vehicle.satellitesUsed) : "—"
          valueColor: root.valueColorFor(px4Vehicle.hasGps, px4Vehicle.gpsFresh)
        }
      }
    }
  }
}
