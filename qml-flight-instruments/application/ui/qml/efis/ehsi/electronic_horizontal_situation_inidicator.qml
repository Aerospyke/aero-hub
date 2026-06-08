import QtQuick 2.15

Item {
  id: root
  width: 300
  height: 300
  clip: true

  property double heading: 0
  property double course: 0
  property double bearing: 0
  property double deviation: 0
  property double distance: 0
  property double headingBug: 0
  property int cdiMode: 0 // 0->OFF, 1->TO, 2->FROM

  readonly property double pixelPerDeviation: 52.5

  FontLoader {
    source: "qrc:/fonts/Courier Std Bold.otf"
  }

  EhsiImage {
    id: back
    source: "qrc:/images/ehsi_back.svg"
    width: 300
    height: 300
  }

  EhsiImage {
    id: devScale
    rotation: -heading + course
    source: "qrc:/images/ehsi_dev_scale.svg"
    width: 300
    height: 300
    visible: cdiMode === 1 || cdiMode === 2
  }

  EhsiImage {
    id: devBar
    rotation: -heading + course
    transform: Translate {
      x: pixelPerDeviation * deviation * Math.cos((-heading + course) * Math.PI / 180.0)
      y: pixelPerDeviation * deviation * Math.sin((-heading + course) * Math.PI / 180.0)
    }
    source: "qrc:/images/ehsi_dev_bar.svg"
    width: 300
    height: 300
    visible: cdiMode === 1 || cdiMode === 2
  }

  EhsiImage {
    id: brgArrow
    rotation: -heading + bearing
    source: "qrc:/images/ehsi_brg_arrow.svg"
    width: 300
    height: 300
  }

  EhsiImage {
    id: crsArrow
    rotation: -heading + course
    source: "qrc:/images/ehsi_crs_arrow.svg"
    width: 300
    height: 300
  }

  EhsiImage {
    id: cdiTo
    rotation: -heading + course
    transform: Translate {
      x: pixelPerDeviation * deviation * Math.cos((-heading + course) * Math.PI / 180.0)
      y: pixelPerDeviation * deviation * Math.sin((-heading + course) * Math.PI / 180.0)
    }
    source: "qrc:/images/ehsi_cdi_to.svg"
    width: 300
    height: 300
    visible: cdiMode === 1
  }

  EhsiImage {
    id: cdiFrom
    rotation: -heading + course
    transform: Translate {
      x: pixelPerDeviation * deviation * Math.cos((-heading + course) * Math.PI / 180.0)
      y: pixelPerDeviation * deviation * Math.sin((-heading + course) * Math.PI / 180.0)
    }
    source: "qrc:/images/ehsi_cdi_from.svg"
    width: 300
    height: 300
    visible: cdiMode === 2
  }

  EhsiImage {
    id: mask
    source: "qrc:/images/ehsi_mask.svg"
    width: 300
    height: 300
  }

  EhsiImage {
    id: hdgScale
    source: "qrc:/images/ehsi_hdg_scale.svg"
    rotation: -heading
    width: 300
    height: 300
  }

  EhsiImage {
    id: hdgBug
    rotation: -heading + headingBug
    source: "qrc:/images/ehsi_hdg_bug.svg"
    width: 300
    height: 300
  }

  EhsiImage {
    id: mark
    source: "qrc:/images/ehsi_mark.svg"
    width: 300
    height: 300
  }

  EhsiLabels {
    id: labels
    headingBug: root.headingBug
    course: root.course
    distance: root.distance
  }
}
