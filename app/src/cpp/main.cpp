#include <QGuiApplication>
#include <QtQml/QtQml>
#include "animation.h"
#include "primary_flight_data.h"

int main(int argc, char* argv[]) {
  Q_INIT_RESOURCE(QmlFlightInstruments);
  const QGuiApplication Application(argc, argv);

  QQmlApplicationEngine engine;

  const QUrl RootUrl("qrc:/qml/AeroHubMainWindow.qml");
  QObject::connect(
      &engine, &QQmlApplicationEngine::objectCreated, &Application,
      [RootUrl](const QObject* object, const QUrl& object_url) {
        if (!object && RootUrl == object_url)
          QCoreApplication::exit(-1);
      },
      Qt::QueuedConnection);

  auto* flight_telemetry = new PrimaryFlightData;
  auto* animation = new Animation;
  animation->setPfd(flight_telemetry);

  engine.rootContext()->setContextProperty("flight_telemetry", flight_telemetry);
  engine.load(RootUrl);

  animation->init();

  return Application.exec();
  return QGuiApplication::exec();
}
