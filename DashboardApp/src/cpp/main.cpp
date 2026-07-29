#include <QByteArray>
#include <QCoreApplication>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQuickStyle>
#include <QString>
#include <QtGlobal>

#include <string>
#include <utility>
#include <vector>

#include "ah_ros_bridge.h"
#include "ah_settings.h"
#include "ah_system_status.h"
#include "ah_track_controller.h"
#include "ah_video_feed.h"
#include "animation.h"
#include "jsb_settings_tree_model.h"
#include "primary_flight_data.h"

int main(int argc, char* argv[]) {
  // macOS "native" style forbids customizing SpinBox/Button backgrounds &
  // indicators. Use a style that allows full QML chrome (Basic/Fusion/Material).
  QQuickStyle::setStyle(QStringLiteral("Basic"));

  Q_INIT_RESOURCE(QmlFlightInstruments);

  AhSettings app_settings;

  // QGuiApplication must exist before QTimer-based watchdogs (status link + video
  // stale detection) or the timers never fire and the UI stays "live" forever.
  const QGuiApplication Application(argc, argv);

  auto* system_status = new AhSystemStatus(QCoreApplication::instance());
  auto* video_feed = new AhVideoFeed(QCoreApplication::instance());

  AhRosBridge::Hooks hooks;
  hooks.on_executor_stopped = []() {
    // TODO: option to restart the ROS bridge node without quitting the app
    qInfo("AeroHub ROS Node executor stopped; quitting application");
    if (QCoreApplication::instance() != nullptr) {
      QCoreApplication::quit();
    }
  };
  hooks.on_status_json = [system_status](const std::string& json) {
    const QString payload = QString::fromStdString(json);
    QMetaObject::invokeMethod(
        system_status,
        [system_status, payload]() { system_status->ApplyJson(payload); },
        Qt::QueuedConnection);
  };
  hooks.on_video_jpeg = [video_feed](std::vector<uint8_t> jpeg) {
    const QByteArray bytes(reinterpret_cast<const char*>(jpeg.data()),
                           static_cast<int>(jpeg.size()));
    QMetaObject::invokeMethod(
        video_feed,
        [video_feed, bytes]() { video_feed->ApplyJpeg(bytes); },
        Qt::QueuedConnection);
  };

  const AhRosBridge RosBridge(app_settings.RosDomainId(), std::move(hooks));
  auto* track_controller = new AhTrackController(RosBridge.Node(), QCoreApplication::instance());

  QQmlApplicationEngine engine;
  engine.addImageProvider(QStringLiteral("ahvideo"), new AhVideoImageProvider(video_feed));

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
  engine.rootContext()->setContextProperty("systemStatus", system_status);
  engine.rootContext()->setContextProperty("videoFeed", video_feed);
  engine.rootContext()->setContextProperty("trackController", track_controller);
  engine.load(RootUrl);

  animation->init();

  return QGuiApplication::exec();
}
