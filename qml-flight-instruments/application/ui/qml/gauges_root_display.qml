import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Layouts 1.0

Window {
  id: window
  visible: true
  minimumWidth: 800
  minimumHeight: 600
  visibility: Window.Maximized
  title: "Miscellaneous Gauges Example"
  color: "#ffffff"

  FontLoader {
    source: "qrc:/fonts/CenturyGothic.ttf"
  }
  property double scaleRatio: Math.min(height / 1080, width / 1920)
  property double radius: 250 * scaleRatio

  Grid {
    columns: 3
    columnSpacing: 48
    rowSpacing: 8

    anchors {
      centerIn: parent
      margins: 16
    }

    TankGauge {
      radius: window.radius
      leftTankFuel: flight_telemetry.leftTankFuel
      rightTankFuel: flight_telemetry.rightTankFuel
    }

    EgtFuelFlowGauge {
      radius: window.radius
      egt: flight_telemetry.egt
      fuelFlow: flight_telemetry.fuelFlow
    }

    PropellerGauge {
      radius: window.radius
      rpm: flight_telemetry.rpm
    }

    VacAmpGauge {
      radius: window.radius
      vac: flight_telemetry.vac
      amp: flight_telemetry.amp
    }

    TemperaturePressureGauge {
      radius: window.radius
      engineTemperature: flight_telemetry.engineTemperature
      enginePressure: flight_telemetry.enginePressure
    }
  }
}
