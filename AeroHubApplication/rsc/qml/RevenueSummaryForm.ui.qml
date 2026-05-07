import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import CyberTheme 1.0

Item {
  id: revenue_summary_root

  Rectangle {
    anchors.fill: parent
    color: CyberTheme.cBackgroundAmber
  }

  ColumnLayout {
    id: summary_chart_area
    anchors.fill: parent
    anchors.margins: 20
    spacing: 20
    // margins: 20

    // ==================== Top Area (Chart) ====================
    Rectangle {
      id: area_one
      Layout.fillWidth: true
      Layout.fillHeight: true
      color: CyberTheme.cGreenDim

      Text {
        id: text_one
        anchors.centerIn: parent
        color: CyberTheme.cCyan
        font.family: CyberTheme.labelFamily
        font.letterSpacing: 1.2
        font.pointSize: 24
        font.weight: Font.Bold
        style: Text.Outline
        styleColor: Qt.rgba(1, 1, 1, 0.2)
        text: "Chart Will Go Here"
      }
    }

    // ==================== Bottom Area ====================
    Rectangle {
      id: area_two
      Layout.fillWidth: true
      Layout.preferredHeight: 80
      color: CyberTheme.cGreyDim

      Text {
        id: text_two
        anchors.centerIn: parent
        color: CyberTheme.cCyan
        font.family: CyberTheme.labelFamily
        font.letterSpacing: 1.2
        font.pointSize: 24
        font.weight: Font.Bold
        style: Text.Outline
        styleColor: Qt.rgba(1, 1, 1, 0.2)
        text: "Maybe Something Here?"
      }
    }
  }
}