#pragma once

// Load ROS domain / namespace / camera selection from env + aerohub_settings.ini.

#include "ah_core/camera_devices.hpp"

#include <string>
#include <string_view>

namespace ah_core
{

struct RosRuntimeSettings
{
  std::string ros_namespace;  // empty = root
  std::string settings_path;
  CameraSelection camera;
};

/// Characters stripped from both ends of INI keys/values (space, tab, quotes).
inline constexpr std::string_view TrimIniChars{" \t\""};

/// Remove any of @p chars_to_trim from both ends of @p s.
std::string Trim(const std::string& full_string, std::string_view chars_to_trim);

/// Sanitize ROS namespace (trim INI noise + slashes; reject "//" empty segments).
std::string SanitizeNamespace(const std::string& raw_namespace_setting);

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
