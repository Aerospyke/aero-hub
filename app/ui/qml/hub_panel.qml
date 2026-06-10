import QtQuick 2.15

Rectangle {
  color: "#0d1620"
  border.color: "#243140"
  border.width: 1
  radius: 6

  Column {
    anchors.centerIn: parent
    spacing: 12

    Text {
      anchors.horizontalCenter: parent.horizontalCenter
      text: "Hub"
      color: "#c8d1dc"
      font.pixelSize: 20
      font.bold: true
    }

    Rectangle {
      width: 330
      height: 330
      radius: 6
      color: "#000000"

      ElectronicAttitudeDirectionIndicator {
        anchors.centerIn: parent
        // scaleRatio: 8.5
        adi.angleOfAttack: flight_telemetry.angleOfAttack
        adi.sideSlipAngle: flight_telemetry.angleOfSideSlip
        adi.roll: flight_telemetry.roll
        adi.pitch: flight_telemetry.pitch
        adi.slipSkid: flight_telemetry.slipSkid
        adi.turnRate: flight_telemetry.turnRate
        adi.dotH: flight_telemetry.ilsLOC
        adi.dotV: flight_telemetry.ilsGS
        adi.fdPitch: flight_telemetry.fdPitch
        adi.fdRoll: flight_telemetry.fdRoll
        adi.dotHVisible: flight_telemetry.ilsLOCVisible
        adi.dotVVisible: flight_telemetry.ilsGSVisible
        adi.fdVisible: flight_telemetry.fdVisible
        adi.stallVisible: flight_telemetry.stall

        asi.airspeed: flight_telemetry.airspeed
        asi.bugValue: flight_telemetry.airspeedBug

        alt.altitude: flight_telemetry.altitude
        alt.bugValue: flight_telemetry.altitudeBug

        hsi.heading: flight_telemetry.heading
        hsi.bugValue: flight_telemetry.headingBug

        vsi.climbRate: flight_telemetry.climbRate

        labels.airspeedBug: flight_telemetry.airspeedBug
        labels.machNumber: flight_telemetry.machNumber
        labels.altitudeBug: flight_telemetry.altitudeBug
        labels.pressure: flight_telemetry.pressure
        labels.pressureMode: flight_telemetry.pressureMode
        labels.flightMode: flight_telemetry.flightMode
        labels.speedMode: flight_telemetry.speedMode
        labels.lnav: flight_telemetry.lateralNavigationMode
        labels.vnav: flight_telemetry.verticalNavigationMode
      }
    }
  }
}
