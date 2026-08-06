#pragma once

// C ABI for process bootstrap (Python ah_yolo via ctypes, or other non-C++ callers).
// Same settings engine as ah_core: ah::Settings::Load().

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct AhSettingsBootstrap {
  uint8_t domain_id;
  int loaded_from_file;  // 1 if INI existed
  char ros_namespace[256];
  char settings_path[1024];
  char yolo_models_dir[1024];  // empty if not resolved
} AhSettingsBootstrap;

/// Load ./aerohub_settings.ini (CWD only) via AhCommon; publish runtime env
/// (ROS_DOMAIN_ID, RMW_IMPLEMENTATION, AERO_HUB_YOLO_MODELS) from the INI.
/// Fills @p out. Returns 0 on success, non-zero on failure.
int ah_settings_bootstrap(AhSettingsBootstrap* out);

#ifdef __cplusplus
}
#endif
