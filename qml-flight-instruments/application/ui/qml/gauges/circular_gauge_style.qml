import QtQuick

QtObject {
  // Exposed from CircularGauge
  property real outerRadius: !parentGauge ? 0.0 : Math.min(parentGauge.width, parentGauge.height) / 2
  property CircularGauge parentGauge

  // Angles
  property real minimumValueAngle: -135
  property real maximumValueAngle: 135

  // Background
  property Component background: null
  property color backgroundColor: "#333333"
  property real backgroundThickness: 14

  // Progress Arc (Set Thickness > 0 for it to appear)1
  property color progressColor: "#4CAF50"
  property real progressArcThickness: 0

  // Tick Marks
  property real tickmarkStepSize: 10
  property Component tickmark: null
  property real majorTickmarkLength: 18
  property real majorTickmarkWidth: 3
  property color tickmarkColor: "#AAAAAA"
  property double tickmarkHeight: 20 / 90 * outerRadius

  // Minor Tick Marks
  property int minorTickmarkCount: 4
  property Component minorTickmark: null
  property real minorTickmarkLength: 10
  property real minorTickmarkWidth: 2
  property color minorTickmarkColor: "#777777"
  property double minorTickmarkHeight: 14 / 90 * outerRadius

  property real tickmarkInset: 12
  property real minorTickmarkInset: 16

  // Labels
  property real labelStepSize: 10
  property Component tickmarkLabel: null
  property color labelColor: "#CCCCCC"
  property int tickmarkLabelPixelSize: Math.max(6, Math.round(0.1 * outerRadius))
  property real labelInset: 38

  // Needle
  property Component needle: null
  property color needleColor: "#E74C3C"
  property real needleLength: 0.78
  property real needleWidth: 6
  property real needleYOffset: -10

  // Foreground
  property Component foreground: null

  // Arc
  property bool positiveDirectionIsClockwise: false

  // ==================== Helper Function ====================
  function valueToAngle(val) {
    if (!parentGauge || parentGauge.maximumValue === parentGauge.minimumValue)
      return 0;
    var normalized = (val - parentGauge.minimumValue) /
        (parentGauge.maximumValue - parentGauge.minimumValue);
    return minimumValueAngle + normalized * (maximumValueAngle - minimumValueAngle);
  }
}