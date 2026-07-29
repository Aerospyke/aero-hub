#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlComponent>
#include <QQmlContext>

#include "ah_ros_bridge.h"
#include "ah_settings.h"
#include "animation.h"
#include "jsb_settings_tree_model.h"
#include "primary_flight_data.h"

int main(int argc, char* argv[]) {
  Q_INIT_RESOURCE(QmlFlightInstruments);

  // Shared project-root INI; domain id is passed into the ROS bridge (not via env).
  AhSettings app_settings;

  // ROS joins the graph on a background thread; Qt keeps the main loop.
  const AhRosBridge RosBridge(app_settings.RosDomainId());

  const QGuiApplication Application(argc, argv);

  QQmlApplicationEngine engine;

  // Load AeroHubTheme as a context property so it is available globally
  // in all QML files (like the instrument types) without per-file imports.
  QQmlComponent theme_component(&engine, QUrl(QStringLiteral("qrc:/qml/AeroHubTheme.qml")));
  QObject* theme_object = theme_component.create();
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
        if (!object && RootUrl == object_url) {
          QCoreApplication::exit(-1);
        }
      },
      Qt::QueuedConnection);

  auto* flight_telemetry = new PrimaryFlightData;
  auto* animation = new Animation;
  animation->setPfd(flight_telemetry);

  auto* jsb_settings_model = new JsbSettingsTreeModel(&app_settings.Settings(), &engine);
  engine.rootContext()->setContextProperty("flight_telemetry", flight_telemetry);
  engine.rootContext()->setContextProperty("jsbSettingsModel", jsb_settings_model);
  engine.load(RootUrl);

  animation->init();

  return QGuiApplication::exec();
}
