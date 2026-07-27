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
  
  // qDebug() << "Check Setting: " << settings->value("JSBSim/command_line/aircraft").toString();

  QQmlApplicationEngine engine;

  // Load AeroHubTheme as a context property so it is available globally
  // in all QML files (like the instrument types) without per-file imports.
  QQmlComponent theme_component(&engine, QUrl(QStringLiteral("qrc:/qml/AeroHubTheme.qml")));
  QObject *theme_object = theme_component.create();
  if (theme_object) {
    theme_object->setParent(&engine);
    engine.rootContext()->setContextProperty("AeroHubTheme", theme_object);
  } else {
    qWarning() << "Failed to create AeroHubTheme:" << theme_component.errorString();
  }

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
