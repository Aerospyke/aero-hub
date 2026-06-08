import QtQuick 2.0
import QtQuick.Effects

Rectangle {
  id: root
  width: 2 * radius
  height: 2 * radius
  radius: 10

  gradient: Gradient {
    GradientStop {
      position: 0.0
      color: "#333333"
    }
    GradientStop {
      position: 1.0
      color: "#101010"
    }
  }

  layer.enabled: true

  layer.effect: MultiEffect {
    id: shadowEffect
    anchors.fill: root
    source: root

    shadowEnabled: true
    shadowColor: "#000000"
    shadowHorizontalOffset: 0.2 * root.radius
    shadowVerticalOffset: 0.2 * root.radius
    shadowBlur: 0.45 * root.radius       // slightly higher for better softness
    shadowOpacity: 0.85
    shadowScale: 1.05                    // makes shadow a bit larger (helps replace spread)
  }
}
