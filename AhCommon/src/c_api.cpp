#include "ah_common/c_api.h"

#include "ah_common/settings.hpp"

#include <cstdio>
#include <cstring>
#include <string>

namespace {

void CopyToBuf(char* dest, size_t dest_len, const std::string& src) {
  if (dest == nullptr || dest_len == 0) {
    return;
  }
  std::snprintf(dest, dest_len, "%s", src.c_str());
}

}  // namespace

extern "C" int ah_settings_bootstrap(AhSettingsBootstrap* out) {
  if (out == nullptr) {
    return 1;
  }
  std::memset(out, 0, sizeof(*out));

  try {
    const ah::Settings settings = ah::Settings::Load();
    out->domain_id = settings.Ros().DomainId();
    out->loaded_from_file = settings.WasFileLoaded() ? 1 : 0;
    CopyToBuf(out->ros_namespace, sizeof(out->ros_namespace), settings.Ros().NamespaceName());
    CopyToBuf(out->settings_path, sizeof(out->settings_path), settings.Path());

    CopyToBuf(out->yolo_models_dir, sizeof(out->yolo_models_dir), settings.Ros().YoloModelsDir());
    // Runtime env already published by Settings::Load() → PublishRuntimeEnvFromSettings().
    return 0;
  } catch (...) {
    return 2;
  }
}
