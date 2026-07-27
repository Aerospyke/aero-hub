import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15

ApplicationWindow {
  id: window
  width: 1920
  height: 1080
  minimumWidth: 1280
  minimumHeight: 720
  visible: true
  title: "AeroHub Dashboard"
  color: "#6171A5"

  FontLoader {
    source: "qrc:/fonts/CenturyGothic.ttf"
  }

  header: TabBar {
    id: modeBar
    width: parent.width
    background: Rectangle {
      color: "#0d1620"
    }

    TabButton {
      text: qsTr("Ops")
      width: implicitWidth + 32
    }
    TabButton {
      text: qsTr("Control")
      width: implicitWidth + 32
    }
    TabButton {
      text: qsTr("Settings")
      width: implicitWidth + 32
    }
  }

  StackLayout {
    anchors.fill: parent
    currentIndex: modeBar.currentIndex

    OpsDashboard {
      // index 0 — multi-panel ops wall
    }

    ControlPage {
      // index 1 — stub control surface
    }

    SettingsPage {
      // index 2 — stub settings
    }
  }
}
