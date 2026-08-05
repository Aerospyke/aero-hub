#pragma once

// Thin adapters: runtime settings live in AhCommon (ah::Settings).

#include "ah_common/settings.hpp"
#include "ah_common/string_util.hpp"
#include "ah_core/camera_devices.hpp"

#include <string>
#include <string_view>

namespace ah_core
{

// Re-export shared helpers under ah_core for existing call sites.
using ah::Trim;
using ah::SanitizeNamespace;
inline constexpr std::string_view TrimIniChars = ah::kTrimIniChars;

struct RosRuntimeSettings
{
  std::string ros_namespace;  // empty = root
  std::string settings_path;
  CameraSelection camera;
};

/// Load aerohub_settings.ini via AhCommon; map into CameraSelection for the node.
RosRuntimeSettings LoadRosRuntimeSettings();

/// Persist camera selection through AhCommon Settings.
bool PersistCameraSelectionToSettingsFile(
  const std::string & settings_path,
  const CameraSelection & camera,
  std::string * error_out);

}  // namespace ah_core
