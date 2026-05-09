#include <qpalette.h>
#include <qquickstyle.h>
#include <QGuiApplication>
#include <QQmlApplicationEngine>

int main(int argc, char* argv[]) {
  // QQuickStyle::setStyle("Fusion");

  QGuiApplication app(argc, argv);

  // Create light palette
  QPalette lightPalette;
  lightPalette.setColor(QPalette::Window, Qt::white);
  lightPalette.setColor(QPalette::WindowText, Qt::black);
  lightPalette.setColor(QPalette::Base, Qt::white);
  lightPalette.setColor(QPalette::AlternateBase, Qt::lightGray);
  lightPalette.setColor(QPalette::Text, Qt::black);
  lightPalette.setColor(QPalette::Button, Qt::white);
  lightPalette.setColor(QPalette::ButtonText, Qt::black);
  app.setPalette(lightPalette);  // Force light mode

  QQmlApplicationEngine application_engine;
  // QmlAvailableResources qml_available_resources;

  // application_engine.load(QUrl(QStringLiteral("qrc:/qml/windfarm_phms_pitch_main.qml")));
  application_engine.load(QUrl(QStringLiteral("qrc:/qml/tesla_main.qml")));

  if (application_engine.rootObjects().isEmpty()) {
    return -1;
  }
  return QGuiApplication::exec();
}
