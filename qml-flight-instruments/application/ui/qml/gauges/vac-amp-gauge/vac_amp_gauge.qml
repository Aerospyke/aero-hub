import QtQuick 2.0

Item {
  property double radius: 300
  property double vac: 0
  property double amp: 0

  width: 2 * radius
  height: 2 * radius

  Rectangle {
    id: rectangle
    width: 2 * radius
    height: 2 * radius
    radius: parent.radius
    clip: true
    color: "#181818"
    scale: 0.775

    VacGauge {
      x: -0.525 * width
      y: 0
      width: parent.width
      height: parent.height
      value: vac
    }

    AmpGauge {
      x: 0.525 * width
      y: 0
      width: parent.width
      height: parent.height
      value: amp
    }

    GaugeMask {
    }

    GaugeText {
      x: 30 / 300 * parent.radius
      width: 40 / 300 * parent.radius
      height: 195 / 300 * parent.radius
      color: "#ffffff"
      text: "VAC"
      anchors.verticalCenter: parent.verticalCenter
      font.pixelSize: Math.max(6, 0.075 * parent.width)
      horizontalAlignment: Text.AlignHCenter
      verticalAlignment: Text.AlignVCenter
      lineHeight: 1
      wrapMode: Text.Wrap
    }

    GaugeText {
      x: 530 / 300 * parent.radius
      width: 40 / 300 * parent.radius
      height: 230 / 300 * parent.radius
      color: "#ffffff"
      text: "AMP"
      anchors.verticalCenter: parent.verticalCenter
      font.pixelSize: Math.max(6, 0.075 * parent.width)
      horizontalAlignment: Text.AlignHCenter
      verticalAlignment: Text.AlignVCenter
      lineHeight: 1
      wrapMode: Text.Wrap
    }
  }

  GaugeImage {
    anchors.fill: parent
    source: "qrc:/images/case.svg"
  }
}
