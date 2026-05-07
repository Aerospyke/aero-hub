pragma Singleton

import QtQuick

Item {

  FontLoader {
    id: monoFontLoader
    Component.onCompleted: source = Qt.url("fonts/ShareTechMono-Regular.ttf")
  }
  FontLoader {
    id: barlowFontLoader
    Component.onCompleted: source = Qt.url("fonts/BarlowCondensed-Bold.ttf")
  }

  // ─── Fonts ────────────────────────────────────────────────────────────────
  readonly property string monoFamily: "Share Tech Mono"
  readonly property string labelFamily: "Barlow Condensed"

  // ─── Core Palette ─────────────────────────────────────────────────────────
  readonly property color cGreen: Qt.color("#00e676")
  readonly property color cGreenDim: Qt.color("#00954c")
  readonly property color cAmber: Qt.color("#ffb300")
  readonly property color cRed: Qt.color("#ff3d3d")
  readonly property color cCyan: Qt.color("#00e5ff")
  readonly property color cWhite: Qt.color("#e8eaed")
  readonly property color cGrey: Qt.color("#607d8b")
  readonly property color cGreyDim: Qt.color("#37474f")
  readonly property color cBorder: Qt.color("#0d1c28")
  readonly property color cBg: Qt.color("#020810")      // overall window / bezel bg
  readonly property color cScreen: Qt.color("#030810")       // main content area bg
  readonly property color cPanel: Qt.color("#020810")      // individual panel backgrounds

  readonly property color cBackgroundRed: Qt.color("#1a0000")
  readonly property color cBackgroundAmber: Qt.color("#1a0e00")
  readonly property color cBackgroundGreen: Qt.color("#001a0a")

  // ─── Status helpers (used for fault/warning/caution states) ───────────────
  function getTextColorForStatus(status) {
    if (status === "WARNING") {
      return cRed
    }
    if (status === "CAUTION") {
      return cAmber
    }
    // noinspection UnnecessaryReturnStatementJS
    return cGreen
  }

  function getBackgroundColorForStatus(status) {
    if (status === "WARNING") return cBackgroundRed
    if (status === "CAUTION") return cBackgroundAmber
    // noinspection UnnecessaryReturnStatementJS
    return cBackgroundGreen
  }

  function getIconForStatus(status) {
    if (status === "WARNING") return "▼"
    if (status === "CAUTION") return "▲"
    return "●"
  }

  // ─── Reusable gradients ───────────────────────────────────────────────────
  readonly property Gradient bezelGradient: Gradient
  {
    GradientStop {
      position: 0.0; color: "#1a1e22"
    }
    GradientStop {
      position: 0.999; color: "#0e1215"
    }
  }

  readonly property Gradient knobGradient: Gradient
  {
    GradientStop {
      position: 0.0; color: "#2a3540"
    }
    GradientStop {
      position: 1.0; color: "#0a1015"
    }
  }

  // You can add more helpers later (pulse opacity, shadow effects, etc.)
}