#pragma once

// Load ROS domain / namespace / camera selection from env + aerohub_settings.ini.

#include "ah_core/camera_devices.hpp"

#include <string>

namespace ah_core
{

struct RosRuntimeSettings
{
  std::string ros_namespace;  // empty = root
  std::string settings_path;
  CameraSelection camera;
};

/// Trim whitespace and optional surrounding quotes (INI values).
std::string Trim(std::string s);

/// Sanitize ROS namespace (no leading/trailing slashes; reject "//").
std::string SanitizeNamespace(std::string ns);

/// Same Qt-style INI as aero-hub/aerohub_settings.ini:
/// [ROS] domain_id, namespace
/// [Camera] video_source, device_id, device_path, backend
/// Env ROS_DOMAIN_ID wins over file for domain.
/// Namespace: env AERO_HUB_ROS_NAMESPACE or file.
RosRuntimeSettings LoadRosRuntimeSettings();

/// Rewrite [Camera] section in settings file (path from LoadRosRuntimeSettings).
/// Returns false if path empty or write failed.
bool PersistCameraSelectionToSettingsFile(
  const std::string & settings_path,
  const CameraSelection & camera,
  std::string * error_out);

}  // namespace ah_core
