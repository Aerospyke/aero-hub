#include <QGuiApplication>
#include <QtQml/QtQml>
#include <QSettings>
#include <iostream>
#include "animation.h"
#include "jsb_settings_tree_model.h"
#include "primary_flight_data.h"

static const char* DebugSettingsFilePath = "./aerohub_settings.ini";

int main(int argc, char* argv[]) {
  Q_INIT_RESOURCE(QmlFlightInstruments);
  const QGuiApplication Application(argc, argv);
  auto* settings = new QSettings(DebugSettingsFilePath, QSettings::IniFormat);
  // QTextStream coutStream(stdout);
  // coutStream << "Check Setting: " << settings->value("JSBSim/command_line/aircraft").toString() << Qt::endl;
  // std::cout << "Check Setting: " << qPrintable(settings->value("JSBSim/airport/ILS-runway-near-latitude").toString()) << std::endl;
  std::cout << "Check Setting: " << settings->value("JSBSim/command_line/aircraft").toString().toStdString() << std::endl;

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

  auto* jsbSettingsModel = new JsbSettingsTreeModel(settings, &engine);
  engine.rootContext()->setContextProperty("flight_telemetry", flight_telemetry);
  engine.rootContext()->setContextProperty("jsbSettingsModel", jsbSettingsModel);
  engine.load(RootUrl);

  animation->init();

  return Application.exec();
  return QGuiApplication::exec();
}
