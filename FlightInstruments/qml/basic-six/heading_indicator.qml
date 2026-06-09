import QtQuick 2.0

Item {
  width: 2 * radius
  height: 2 * radius

  required property double radius
  property double heading: 0

  BasicSixImage {
    anchors.fill: parent
    source: "qrc:/images/hi_face.svg"
    rotation: -heading
  }

  BasicSixImage {
    anchors.fill: parent
    source: "qrc:/images/hi_case.svg"
  }
}
