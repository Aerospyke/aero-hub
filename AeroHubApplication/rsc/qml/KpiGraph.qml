import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtGraphs

Rectangle {
    id: kpi_graph_root

    property string graphTitle: "KPI"
    property var seriesAData: []
    property var seriesBData: []
    property var seriesCData: []
    property color siteAColor: "#4285F4"
    property color siteBColor: "#34A853"
    property color siteCColor: "purple"
    property bool xAxisVisible: false
    property double yAxisMax: 100
    property double yAxisMin: 0
    property double yAxisTickInterval: 10

    Layout.fillWidth: true
    Layout.leftMargin: 20
    Layout.preferredHeight: 260
    Layout.rightMargin: 20
    border.color: "#e0e0e0"
    color: "white"
    radius: 12

    RowLayout {
        anchors.fill: parent
        anchors.margins: 24
        spacing: 8

        Text {
            Layout.fillHeight: true
            Layout.preferredWidth: 40
            font.bold: true
            font.pixelSize: 20
            horizontalAlignment: Text.AlignHCenter
            rotation: -90
            text: kpi_graph_root.graphTitle
            verticalAlignment: Text.AlignVCenter
        }
        GraphsView {
            Layout.fillHeight: true
            Layout.fillWidth: true

            axisX: BarCategoryAxis {
                categories: ["1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12", "13", "14", "15", "16", "17", "18", "19", "20", "21", "22", "23", "24", "25", "26", "27", "28", "29", "30"]
                titleFont.bold: true
                titleFont.pixelSize: 20
                titleText: kpi_graph_root.xAxisVisible ? "Days" : ""
                visible: kpi_graph_root.xAxisVisible
            }
            axisY: ValueAxis {
                labelDecimals: 0
                max: kpi_graph_root.yAxisMax
                min: kpi_graph_root.yAxisMin
                tickInterval: kpi_graph_root.yAxisTickInterval
            }
            theme: GraphsTheme {
                backgroundColor: "white"
                colorScheme: GraphsTheme.ColorScheme.Light
                grid.mainColor: "#e0e0e0"
                plotAreaBackgroundColor: "white"
            }

            Component.onCompleted: {
                seriesA.clear();
                for (var i = 0; i < kpi_graph_root.seriesAData.length; i++) {
                    seriesA.append(kpi_graph_root.seriesAData[i].x, kpi_graph_root.seriesAData[i].y);
                }
                seriesB.clear();
                for (var i = 0; i < kpi_graph_root.seriesBData.length; i++) {
                    seriesB.append(kpi_graph_root.seriesBData[i].x, kpi_graph_root.seriesBData[i].y);
                }
                seriesC.clear();
                for (var i = 0; i < kpi_graph_root.seriesCData.length; i++) {
                    seriesC.append(kpi_graph_root.seriesCData[i].x, kpi_graph_root.seriesCData[i].y);
                }
            }

            LineSeries {
                id: seriesA

                color: kpi_graph_root.siteAColor
                width: 4
            }
            LineSeries {
                id: seriesB

                color: kpi_graph_root.siteBColor
                width: 4
            }
            LineSeries {
                id: seriesC

                color: kpi_graph_root.siteCColor
                width: 4
            }
        }
    }
}
