import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15

ApplicationWindow {
  width: 1920
  height: 1080
  visible: true
  title: "AeroHub Simulation Connector"
  color: "#314155"

  FontLoader {
    source: "qrc:/fonts/CenturyGothic.ttf"
  }

  SplitView {
    anchors.fill: parent
    orientation: Qt.Horizontal

    // LEFT: vertically stacked Basic Six + Gauges
    SplitView {
      orientation: Qt.Vertical
      SplitView.preferredWidth: 1150
      SplitView.minimumWidth: 500

      // ========== BASIC SIX SECTION ==========
      ColumnLayout {
        id: basicSixSection
        SplitView.preferredHeight: 520
        SplitView.minimumHeight: 300
        Layout.fillWidth: true
        spacing: 6

        property double scaleRatio: Math.min((height - 40) / 650, (width - 20) / 1050)
        property double instrumentRadius: 155 * scaleRatio

        Label {
          text: "Basic Six"
          font.bold: true
          font.pixelSize: 20
          color: "#000000"

          Layout.alignment: Qt.AlignHCenter
        }

        Rectangle {
          Layout.fillWidth: true
          Layout.fillHeight: true
          color: "#ffffff"
          radius: 4
          clip: true

          Grid {
            columns: 3
            columnSpacing: 18
            rowSpacing: 10
            anchors.centerIn: parent

            AirspeedIndicatorBasicSix {
              radius: basicSixSection.instrumentRadius
              airspeed: flight_telemetry.airspeed
            }
            AttitudeIndicatorBasicSix {
              radius: basicSixSection.instrumentRadius
              roll: flight_telemetry.roll
              pitch: flight_telemetry.pitch
            }
            AltimeterBasicSix {
              radius: basicSixSection.instrumentRadius
              altitude: flight_telemetry.altitude
              pressure: flight_telemetry.pressure
            }
            TurnCoordinatorBasicSix {
              radius: basicSixSection.instrumentRadius
              turnRate: flight_telemetry.turnRate
              slipSkid: flight_telemetry.slipSkid
            }
            HeadingIndicatorBasicSix {
              radius: basicSixSection.instrumentRadius
              heading: flight_telemetry.heading
            }
            VerticalSpeedIndicatorBasicSix {
              radius: basicSixSection.instrumentRadius
              climbRate: flight_telemetry.climbRate
            }
          }
        }
      }

      // ========== GAUGES SECTION ==========
      ColumnLayout {
        id: gaugesSection
        SplitView.preferredHeight: 520
        SplitView.minimumHeight: 300
        Layout.fillWidth: true
        spacing: 6

        property double scaleRatio: Math.min((height - 40) / 650, (width - 20) / 1050)
        property double instrumentRadius: 155 * scaleRatio

        Label {
          text: "Engine & System Gauges"
          font.bold: true
          font.pixelSize: 20
          color: "#000000"
          Layout.alignment: Qt.AlignHCenter
        }

        Rectangle {
          Layout.fillWidth: true
          Layout.fillHeight: true
          color: "#ffffff"
          radius: 4
          clip: true

          Grid {
            columns: 3
            columnSpacing: 24
            rowSpacing: 10
            anchors.centerIn: parent

            TankGauge {
              radius: gaugesSection.instrumentRadius
              leftTankFuel: flight_telemetry.leftTankFuel
              rightTankFuel: flight_telemetry.rightTankFuel
            }
            EgtFuelFlowGauge {
              radius: gaugesSection.instrumentRadius
              egt: flight_telemetry.egt
              fuelFlow: flight_telemetry.fuelFlow
            }
            PropellerGauge {
              radius: gaugesSection.instrumentRadius
              rpm: flight_telemetry.rpm
            }
            VacAmpGauge {
              radius: gaugesSection.instrumentRadius
              vac: flight_telemetry.vac
              amp: flight_telemetry.amp
            }
            TemperaturePressureGauge {
              radius: gaugesSection.instrumentRadius
              engineTemperature: flight_telemetry.engineTemperature
              enginePressure: flight_telemetry.enginePressure
            }
          }
        }
      }
    }

    // RIGHT: EFIS (full height)
    ColumnLayout {
      SplitView.preferredWidth: 700
      SplitView.minimumWidth: 500
      Layout.fillHeight: true

      Label {
        text: "EFIS (EADI + EHSI)"
        font.bold: true
        font.pixelSize: 20
        color: "#000000"
        Layout.alignment: Qt.AlignHCenter
      }

      Rectangle {
        Layout.fillWidth: true
        Layout.fillHeight: true
        color: "#ffffff"
        radius: 4
        clip: true

        Item {
          id: efisContainer
          anchors.fill: parent
          anchors.margins: 8

          property double scaleRatio: Math.min(height / 650, width / 330)

          Column {
            anchors.centerIn: parent
            spacing: 16
            scale: efisContainer.scaleRatio

            Rectangle {
              width: 310
              height: 310
              radius: 6
              color: "#000000"

              ElectronicAttitudeDirectionIndicator {
                anchors.centerIn: parent
                scaleRatio: efisContainer.scaleRatio

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
    }
  }


}
