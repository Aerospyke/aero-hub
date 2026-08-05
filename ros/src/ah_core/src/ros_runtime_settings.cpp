#include "ah_core/ros_runtime_settings.hpp"

namespace ah_core {

RosRuntimeSettings LoadRosRuntimeSettings() {
  const ah::Settings settings = ah::Settings::Load();
  RosRuntimeSettings out;
  out.settings_path = settings.Path();
  out.ros_namespace = settings.Ros().NamespaceName();

  const ah::CameraSelection sel = settings.Camera().Selection();
  out.camera.video_source = sel.video_source;
  out.camera.device_id = sel.device_id;
  out.camera.device_path = sel.device_path;
  out.camera.backend = sel.backend;
  return out;
}

bool PersistCameraSelectionToSettingsFile(const std::string& settings_path, const CameraSelection& camera,
                                          std::string& error_out) {
  ah::Settings settings = settings_path.empty() ? ah::Settings::Load() : ah::Settings::LoadFromPath(settings_path);

  ah::CameraSelection sel;
  sel.video_source = camera.video_source;
  sel.device_id = camera.device_id;
  sel.device_path = camera.device_path;
  sel.backend = camera.backend;
  return settings.PersistCamera(sel, error_out);
}

}  // namespace ah_core
