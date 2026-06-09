// Example standalone display (opens its own Window).
// Component names (e.g. AirspeedIndicatorBasicSix) resolve because all the
// instrument QML files are registered under the same qrc:/qml/ prefix (via aliases).

import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Layouts 1.0

Window {
  id: window
  visible: true
  minimumWidth: 800
  minimumHeight: 600
  visibility: Window.Maximized
  title: "Basic Six Example"
  color: "#ffffff"

  property double scaleRatio: Math.min(height / 1080, width / 1920)
  property double radius: 250 * scaleRatio

  Grid {
    columns: 3
    columnSpacing: 32
    rowSpacing: 16

    anchors {
      centerIn: parent
      margins: 16
    }

    AirspeedIndicatorBasicSix {
      radius: window.radius
      airspeed: flight_telemetry.airspeed
    }

    AttitudeIndicatorBasicSix {
      radius: window.radius
      roll: flight_telemetry.roll
      pitch: flight_telemetry.pitch
    }

    AltimeterBasicSix {
      radius: window.radius
      altitude: flight_telemetry.altitude
      pressure: flight_telemetry.pressure
    }

    TurnCoordinatorBasicSix {
      radius: window.radius
      turnRate: flight_telemetry.turnRate
      slipSkid: flight_telemetry.slipSkid
    }

    HeadingIndicatorBasicSix {
      radius: window.radius
      heading: flight_telemetry.heading
    }

    VerticalSpeedIndicatorBasicSix {
      radius: window.radius
      climbRate: flight_telemetry.climbRate
    }
  }
}
