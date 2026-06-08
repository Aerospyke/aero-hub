#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>

#include "animation.h"
#include "primary_flight_data.h"

int main(int argc, char* argv[]) {
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
  QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif
  const QGuiApplication Application(argc, argv);

  QQmlApplicationEngine engine;

  const QUrl BasicSixRoot("qrc:/qml/BasicSixRootDisplay.qml");
  QObject::connect(
      &engine, &QQmlApplicationEngine::objectCreated, &Application,
      [BasicSixRoot](const QObject* object, const QUrl& object_url) {
        if (!object && BasicSixRoot == object_url)
          QCoreApplication::exit(-1);
      },
      Qt::QueuedConnection);

  const QUrl EfisRoot("qrc:/qml/EfisRootDisplay.qml");
  QObject::connect(
      &engine, &QQmlApplicationEngine::objectCreated, &Application,
      [EfisRoot](const QObject* obj, const QUrl& object_url) {
        if (!obj && EfisRoot == object_url)
          QCoreApplication::exit(-1);
      },
      Qt::QueuedConnection);

  const QUrl GaugesRoot("qrc:/qml/GaugesRootDisplay");
  QObject::connect(
      &engine, &QQmlApplicationEngine::objectCreated, &Application,
      [GaugesRoot](const QObject* object, const QUrl& object_url) {
        if (!object && GaugesRoot == object_url)
          QCoreApplication::exit(-1);
      },
      Qt::QueuedConnection);

  auto* flight_telemetry = new PrimaryFlightData;
  auto* animation = new Animation;
  animation->setPfd(flight_telemetry);

  engine.rootContext()->setContextProperty("flight_telemetry", flight_telemetry);
  engine.load(BasicSixRoot);
  engine.load(EfisRoot);
  engine.load(GaugesRoot);

  animation->init();

  return Application.exec();
}
