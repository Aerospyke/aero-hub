import QtQuick 2.0

Item {
  width: 2 * radius
  height: 2 * radius

  required property double radius
  property double climbRate: 0

  BasicSixImage {
    anchors.fill: parent
    source: "qrc:/images/vsi_face.svg"
  }

  BasicSixImage {
    anchors.fill: parent
    source: "qrc:/images/vsi_case.svg"
  }

  BasicSixImage {
    anchors.fill: parent
    source: "qrc:/images/vsi_hand.svg"
    rotation: 8.6 * climbRate
  }
}
