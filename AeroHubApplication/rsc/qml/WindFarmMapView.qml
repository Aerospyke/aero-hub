import QtQuick
import QtQuick.Layouts

Rectangle {
  id: mapview_root
  Layout.fillWidth: true
  Layout.preferredHeight: 700

  property color site_a_color: "#4285F4"
  property color site_b_color: "#34A853"
  property color site_c_color: "purple"

  signal siteClicked(string siteName)

  property var sites: [
    {name: "A", x: 0.2, y: 0.3, color: site_a_color},
    {name: "B", x: 0.7, y: 0.25, color: site_b_color},
    {name: "C", x: 0.35, y: 0.65, color: site_c_color},
  ]

  ColumnLayout {
    anchors.fill: parent
    anchors.margins: 20
    spacing: 12

    Text {
      text: "Wind Farm Sites Overview"
      font.pixelSize: 24
      font.bold: true
      color: "#1f2a44"
      leftPadding: 20
      Layout.alignment: Qt.AlignLeft
    }

    Canvas {
      id: mapCanvas
      Layout.fillWidth: true
      Layout.preferredHeight: 600

      MouseArea {
        anchors.fill: parent
        onClicked: function (event) {
          const x = event.x;
          const y = event.y;
          const radius = 25;
          for (let i = 0; i < mapview_root.sites.length; i++) {
            const site = mapview_root.sites[i];
            const siteX = mapCanvas.width * site.x;
            const siteY = mapCanvas.height * site.y
            const dx = x - siteX
            const dy = y - siteY
            if (dx * dx + dy * dy < radius * radius) {
              mapview_root.siteClicked(site.name)
              break
            }
          }
        }
      }

      onPaint: {
        const ctx = mapCanvas.getContext("2d");
        ctx.clearRect(0, 0, width, height)

        ctx.fillStyle = "#e8f5e9"
        ctx.fillRect(0, 0, width, height)

        ctx.fillStyle = "#a5d6a7"
        ctx.beginPath()
        ctx.moveTo(0, height * 0.2)
        ctx.lineTo(width * 0.25, height * 0.15)
        ctx.lineTo(width * 0.35, height * 0.35)
        ctx.lineTo(width * 0.2, height * 0.55)
        ctx.lineTo(0, height * 0.45)
        ctx.closePath()
        ctx.fill()
        ctx.beginPath()
        ctx.moveTo(width * 0.65, height * 0.1)
        ctx.lineTo(width * 0.85, height * 0.2)
        ctx.lineTo(width, height * 0.15)
        ctx.lineTo(width, height * 0.5)
        ctx.lineTo(width * 0.75, height * 0.45)
        ctx.lineTo(width * 0.7, height * 0.65)
        ctx.lineTo(width * 0.6, height * 0.5)
        ctx.closePath()
        ctx.fill()

        const hillGradient = ctx.createLinearGradient(width * 0.35, height * 0.75, width * 0.65, height * 0.25);
        hillGradient.addColorStop(0, "#81c784")
        hillGradient.addColorStop(0.5, "#a5d6a7")
        hillGradient.addColorStop(1, "#66bb6a")
        ctx.fillStyle = hillGradient
        ctx.beginPath()
        ctx.moveTo(width * 0.3, height * 0.85)
        ctx.quadraticCurveTo(width * 0.5, height * 0.15, width * 0.7, height * 0.85)
        ctx.closePath()
        ctx.fill()

        ctx.fillStyle = "#b3e5fc"
        ctx.beginPath()
        ctx.ellipse(width * 0.15, height * 0.4, width * 0.12, height * 0.15, 0, 0, Math.PI * 2)
        ctx.fill()
        ctx.beginPath()
        ctx.ellipse(width * 0.85, height * 0.75, width * 0.1, height * 0.12, 0, 0, Math.PI * 2)
        ctx.fill()

        ctx.strokeStyle = "#bdbdbd"
        ctx.lineWidth = 4
        ctx.beginPath()
        ctx.moveTo(0, height * 0.35)
        ctx.lineTo(width * 0.3, height * 0.35)
        ctx.stroke()
        ctx.beginPath()
        ctx.moveTo(width * 0.7, height * 0.35)
        ctx.lineTo(width, height * 0.35)
        ctx.stroke()
        ctx.beginPath()
        ctx.moveTo(width * 0.5, 0)
        ctx.quadraticCurveTo(width * 0.52, height * 0.25, width * 0.5, height * 0.4)
        ctx.stroke()

        ctx.strokeStyle = "#e0e0e0"
        ctx.lineWidth = 1
        for (let i = 0; i < 10; i++) {
          const y = (height / 10) * i;
          ctx.beginPath()
          ctx.moveTo(0, y)
          ctx.lineTo(width, y)
          ctx.stroke()
        }
        for (let i_2 = 0; i_2 < 10; i_2++) {
          const x = (width / 10) * i_2;
          ctx.beginPath()
          ctx.moveTo(x, 0)
          ctx.lineTo(x, height)
          ctx.stroke()
        }

        for (let i_3 = 0; i < mapview_root.sites.length; i_3++) {
          const site = mapview_root.sites[i_3];
          const siteX = width * site.x;
          const siteY = height * site.y;
          const radius = 25;

          ctx.fillStyle = site.color
          ctx.beginPath()
          ctx.arc(siteX, siteY, radius, 0, Math.PI * 2)
          ctx.fill()

          ctx.shadowColor = site.color + "80"
          ctx.shadowBlur = 15
          ctx.strokeStyle = "white"
          ctx.lineWidth = 3
          ctx.stroke()
          ctx.shadowBlur = 0

          ctx.fillStyle = "white"
          ctx.font = "bold 20px Arial"
          ctx.textAlign = "center"
          ctx.textBaseline = "middle"
          ctx.fillText(site.name, siteX, siteY)
        }
      }
    }
  }
}
