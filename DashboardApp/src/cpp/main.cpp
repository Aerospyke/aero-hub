#include <QByteArray>
#include <QCoreApplication>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQuickStyle>
#include <QString>
#include <QVariant>
#include <QtGlobal>

#include <cstdlib>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "ah_camera_controller.h"
#include "ah_detections_model.h"
#include "ah_px4_telemetry.h"
#include "ah_ros_bridge.h"
#include "ah_settings.h"
#include "ah_system_status.h"
#include "ah_track_controller.h"
#include "ah_video_feed.h"
#include "ah_yolo_controller.h"
#include "animation.h"
#include "jsb_settings_tree_model.h"
#include "primary_flight_data.h"

namespace {

// rosidl loads px4 typesupport by absolute path from the ament index. CLion
// launched from the Dock has no shell overlay, so prepend the prefix baked in
// at configure time. Existing entries (the run/ init script) stay in place.
void EnsurePx4MsgsOnAmentPath() {
#ifdef AH_PX4_MSGS_PREFIX
  const std::string prefix(AH_PX4_MSGS_PREFIX);
  if (prefix.empty()) {
    return;
  }
  const char* existing = std::getenv("AMENT_PREFIX_PATH");
  const std::string current = existing == nullptr ? std::string() : std::string(existing);
  const std::string wrapped = ":" + current + ":";
  if (wrapped.find(":" + prefix + ":") != std::string::npos) {
    return;
  }
  std::string merged = prefix;
  if (!current.empty()) {
    merged.push_back(':');
    merged += current;
  }
  setenv("AMENT_PREFIX_PATH", merged.c_str(), 1);
#else
  // Configured without px4_msgs. Subscriptions fail soft in the bridge.
#endif
}

}  // namespace

// Lab: settings + relative paths use process CWD. Launch from /run directory with two steps
// 1. `source ./init_ah_ros_in_terminal.sh`
// 2. './AeroHub.app/Contents/MacOS/AeroHub'
// Do not use bare `open AeroHub.app` — it does not inherit the shell ROS env.
int main(int argument_count, char* argument_values[]) {
  // macOS "native" style forbids customizing SpinBox/Button backgrounds &
  // indicators. Use a style that allows full QML chrome (Basic/Fusion/Material).
  QQuickStyle::setStyle(QStringLiteral("Basic"));

  Q_INIT_RESOURCE(QmlFlightInstruments);


  AhSettings app_settings;
  EnsurePx4MsgsOnAmentPath();

  // QGuiApplication must exist before QTimer-based watchdogs (status link + video
  // stale detection) or the timers never fire and the UI stays "live" forever.
  const QGuiApplication Application(argument_count, argument_values);

  auto* system_status = new AhSystemStatus(QCoreApplication::instance());
  auto* video_feed = new AhVideoFeed(QCoreApplication::instance());
  auto* detections_model = new AhDetectionsModel(QCoreApplication::instance());
  const QStringList vehicle_names = app_settings.VehicleNamespaces();
  std::vector<AhPx4Telemetry*> px4_vehicles;
  px4_vehicles.reserve(static_cast<size_t>(vehicle_names.size()));
  std::unordered_map<std::string, AhPx4Telemetry*> px4_by_namespace;
  for (const QString& name : vehicle_names) {
    auto* vehicle = new AhPx4Telemetry(name, QCoreApplication::instance());
    px4_vehicles.push_back(vehicle);
    px4_by_namespace.emplace(name.toStdString(), vehicle);
  }
  AhPx4Telemetry* px4_vehicle = px4_vehicles.front();

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
        system_status, [system_status, payload]() { system_status->ApplyJson(payload); }, Qt::QueuedConnection);
  };
  hooks.on_video_jpeg = [video_feed](std::vector<uint8_t> jpeg) {
    const QByteArray bytes(reinterpret_cast<const char*>(jpeg.data()), static_cast<int>(jpeg.size()));
    QMetaObject::invokeMethod(
        video_feed, [video_feed, bytes]() { video_feed->ApplyJpeg(bytes); }, Qt::QueuedConnection);
  };
  hooks.on_detections_json = [detections_model](const std::string& json) {
    const QString payload = QString::fromStdString(json);
    QMetaObject::invokeMethod(
        detections_model, [detections_model, payload]() { detections_model->ApplyJson(payload); },
        Qt::QueuedConnection);
  };
  hooks.on_px4_odometry = [px4_by_namespace](const std::string& ns, AhPx4OdomSample sample) {
    const auto it = px4_by_namespace.find(ns);
    if (it == px4_by_namespace.end()) {
      return;
    }
    AhPx4Telemetry* vehicle = it->second;
    QMetaObject::invokeMethod(
        vehicle, [vehicle, sample]() { vehicle->ApplyOdometry(sample); }, Qt::QueuedConnection);
  };
  hooks.on_px4_status = [px4_by_namespace](const std::string& ns, AhPx4StatusSample sample) {
    const auto it = px4_by_namespace.find(ns);
    if (it == px4_by_namespace.end()) {
      return;
    }
    AhPx4Telemetry* vehicle = it->second;
    QMetaObject::invokeMethod(
        vehicle, [vehicle, sample]() { vehicle->ApplyStatus(sample); }, Qt::QueuedConnection);
  };
  hooks.on_px4_gps = [px4_by_namespace](const std::string& ns, AhPx4GpsSample sample) {
    const auto it = px4_by_namespace.find(ns);
    if (it == px4_by_namespace.end()) {
      return;
    }
    AhPx4Telemetry* vehicle = it->second;
    QMetaObject::invokeMethod(
        vehicle, [vehicle, sample]() { vehicle->ApplyGps(sample); }, Qt::QueuedConnection);
  };

  std::vector<std::string> vehicle_namespaces;
  vehicle_namespaces.reserve(px4_vehicles.size());
  for (const QString& name : vehicle_names) {
    vehicle_namespaces.push_back(name.toStdString());
  }
  const AhRosBridge RosBridge(app_settings.RosDomainId(), app_settings.RosNamespace().toStdString(),
                              std::move(vehicle_namespaces), std::move(hooks));
  auto* track_controller = new AhTrackController(RosBridge.Node(), QCoreApplication::instance());
  auto* camera_controller = new AhCameraController(RosBridge.Node(), QCoreApplication::instance());
  auto* yolo_controller = new AhYoloController(RosBridge.Node(), QCoreApplication::instance());

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

  auto* jsb_settings_model = new JsbSettingsTreeModel(&app_settings.Core(), &engine);
  engine.rootContext()->setContextProperty("flight_telemetry", flight_telemetry);
  engine.rootContext()->setContextProperty("jsbSettingsModel", jsb_settings_model);
  engine.rootContext()->setContextProperty("systemStatus", system_status);
  engine.rootContext()->setContextProperty("videoFeed", video_feed);
  engine.rootContext()->setContextProperty("trackController", track_controller);
  engine.rootContext()->setContextProperty("cameraController", camera_controller);
  engine.rootContext()->setContextProperty("yoloController", yolo_controller);
  engine.rootContext()->setContextProperty("detectionsModel", detections_model);
  QVariantList px4_vehicle_list;
  for (AhPx4Telemetry* vehicle : px4_vehicles) {
    px4_vehicle_list.append(QVariant::fromValue(static_cast<QObject*>(vehicle)));
  }
  engine.rootContext()->setContextProperty("px4Vehicles", px4_vehicle_list);
  // First ini entry. Hub and any panel that still binds one vehicle keep working.
  engine.rootContext()->setContextProperty("px4Vehicle", px4_vehicle);
  engine.load(RootUrl);

  animation->init();

  return QGuiApplication::exec();
}
