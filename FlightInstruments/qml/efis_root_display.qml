// Example standalone display (opens its own Window).
// Component names resolve because all the instrument QML files are registered
// under the same qrc:/qml/ prefix via the module's .qrc (aliases).

import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15

Window {
  id: window
  visible: true
  minimumWidth: 800
  minimumHeight: 600
  visibility: Window.Maximized
  title: "Electronic Flight Instrument System (EFIS) Example"
  color: "#ffffff"

  Item {
    id: container
    property double scaleRatio: 3.05 * Math.min(height / 1080, width / 1920)

    anchors {
      fill: parent
      margins: 16
    }

    Row {
      anchors.centerIn: parent
      spacing: 24
      scale: container.scaleRatio

      Rectangle {
        width: 310
        height: 310
        radius: 6
        color: "#000000"

        ElectronicAttitudeDirectionIndicator {
          anchors.centerIn: parent
          scaleRatio: container.scaleRatio

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

      Rectangle {
        width: 310
        height: 310
        radius: 6
        color: "#000000"

        ElectronicHorizontalSituationIndicator {
          anchors.centerIn: parent

          heading: flight_telemetry.heading
          course: flight_telemetry.course
          bearing: flight_telemetry.bearing
          deviation: flight_telemetry.vorDeviation
          headingBug: flight_telemetry.headingBug
          distance: flight_telemetry.dmeDistance
          cdiMode: flight_telemetry.courseDeviationIndicatorMode
        }
      }
    }
  }
}
