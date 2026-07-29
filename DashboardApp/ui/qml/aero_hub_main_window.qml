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

  // Shared chrome (matches StatusBar footer)
  readonly property color chromeBg: "#0d1620"
  readonly property color chromeBorder: "#243140"
  readonly property color chromeText: "#8a96a5"
  readonly property color chromeTextActive: "#e8eef5"
  readonly property color chromeAccent: "#3d9eff"

  FontLoader {
    source: "qrc:/fonts/CenturyGothic.ttf"
  }

  // Custom tab button: muted idle, bright + accent underline when selected
  component ModeTabButton: TabButton {
    id: tabBtn
    font.pixelSize: 14
    font.bold: checked
    leftPadding: 22
    rightPadding: 22
    topPadding: 12
    bottomPadding: 12

    contentItem: Text {
      text: tabBtn.text
      font: tabBtn.font
      color: tabBtn.checked ? window.chromeTextActive : window.chromeText
      horizontalAlignment: Text.AlignHCenter
      verticalAlignment: Text.AlignVCenter
      opacity: tabBtn.hovered && !tabBtn.checked ? 0.9 : 1.0
    }

    background: Item {
      implicitHeight: 44

      Rectangle {
        anchors.fill: parent
        color: tabBtn.checked ? "#152030" : (tabBtn.hovered ? "#111b28" : "transparent")
      }

      // Active indicator (console “selected channel” underline)
      Rectangle {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: 2
        color: window.chromeAccent
        visible: tabBtn.checked
      }
    }
  }

  header: Rectangle {
    width: parent.width
    height: 48
    color: window.chromeBg

    Rectangle {
      anchors.left: parent.left
      anchors.right: parent.right
      anchors.bottom: parent.bottom
      height: 1
      color: window.chromeBorder
    }

    RowLayout {
      anchors.fill: parent
      anchors.leftMargin: 16
      anchors.rightMargin: 16
      spacing: 20

      // Wordmark — ties header to product without cluttering tabs
      Text {
        text: "AeroHub"
        color: window.chromeTextActive
        font.pixelSize: 15
        font.bold: true
        font.letterSpacing: 1.2
        Layout.alignment: Qt.AlignVCenter
      }

      Rectangle {
        width: 1
        height: 20
        color: window.chromeBorder
        Layout.alignment: Qt.AlignVCenter
      }

      TabBar {
        id: modeBar
        Layout.fillHeight: true
        background: Item {}
        spacing: 2

        ModeTabButton {
          text: qsTr("Ops")
        }
        ModeTabButton {
          text: qsTr("Control")
        }
        ModeTabButton {
          text: qsTr("Settings")
        }
      }

      Item {
        Layout.fillWidth: true
      }

      // Subtle mode hint (which surface is active)
      Text {
        text: modeBar.currentIndex === 0 ? qsTr("Operations wall")
            : modeBar.currentIndex === 1 ? qsTr("Control & status")
            : qsTr("Settings")
        color: window.chromeText
        font.pixelSize: 12
        Layout.alignment: Qt.AlignVCenter
      }
    }
  }

  footer: StatusBar {
    width: parent.width
  }

  StackLayout {
    anchors.fill: parent
    currentIndex: modeBar.currentIndex

    OpsDashboard {
      // index 0 — multi-panel ops wall
    }

    ControlPage {
      // index 1 — control surface + detailed status
    }

    SettingsPage {
      // index 2 — stub settings
    }
  }
}
