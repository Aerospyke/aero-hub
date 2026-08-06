#include <QByteArray>
#include <QCoreApplication>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQuickStyle>
#include <QString>
#include <QtGlobal>

#include <cstdlib>
#include <filesystem>
#include <string>
#include <utility>
#include <vector>

#include "ah_camera_controller.h"
#include "ah_detections_model.h"
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

// Lab model: ROS lives in the machine install (RoboStack /opt/ros), not inside the
// app bundle. Finder / bare open has no shell env. AhCommon sets domain/RMW from
// the INI; ament still needs prefixes for RMW + typesupport plugins.
void EnsureAmentPrefixPathForLab() {
  namespace fs = std::filesystem;
  std::string conda;
  if (const char* c = std::getenv("CONDA_PREFIX"); c != nullptr && c[0] != '\0') {
    conda = c;
  } else if (const char* home = std::getenv("HOME"); home != nullptr) {
    const fs::path Candidate = fs::path(home) / "miniconda3" / "envs" / "ros_env";
    if (fs::is_directory(Candidate / "share" / "rmw_fastrtps_cpp")) {
      conda = Candidate.string();
      setenv("CONDA_PREFIX", conda.c_str(), /*overwrite=*/0);
    }
  }
  if (conda.empty()) {
    return;
  }

  std::string ament = conda;
  // Colcon overlay next to operational CWD (run/ → ../ros/install/<pkg>).
  const fs::path RosInstall = fs::current_path() / ".." / "ros" / "install";
  if (fs::is_directory(RosInstall)) {
    for (const auto& entry : fs::directory_iterator(RosInstall)) {
      if (entry.is_directory() && fs::is_directory(entry.path() / "share")) {
        ament += ':';
        ament += entry.path().lexically_normal().string();
      }
    }
  }
  if (const char* existing = std::getenv("AMENT_PREFIX_PATH");
      existing != nullptr && existing[0] != '\0') {
    ament = std::string(existing) + ':' + ament;
  }
  setenv("AMENT_PREFIX_PATH", ament.c_str(), /*overwrite=*/1);
}

}  // namespace

int main(int argument_count, char* argument_values[]) {
  // macOS "native" style forbids customizing SpinBox/Button backgrounds &
  // indicators. Use a style that allows full QML chrome (Basic/Fusion/Material).
  QQuickStyle::setStyle(QStringLiteral("Basic"));

  Q_INIT_RESOURCE(QmlFlightInstruments);

  AhSettings app_settings;
  EnsureAmentPrefixPathForLab();


  // QGuiApplication must exist before QTimer-based watchdogs (status link + video
  // stale detection) or the timers never fire and the UI stays "live" forever.
  const QGuiApplication Application(argument_count, argument_values);

  auto* system_status = new AhSystemStatus(QCoreApplication::instance());
  auto* video_feed = new AhVideoFeed(QCoreApplication::instance());
  auto* detections_model = new AhDetectionsModel(QCoreApplication::instance());

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

  const AhRosBridge RosBridge(app_settings.RosDomainId(), app_settings.RosNamespace().toStdString(), std::move(hooks));
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
  engine.load(RootUrl);

  animation->init();

  return QGuiApplication::exec();
}
