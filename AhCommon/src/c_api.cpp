#include "ah_common/c_api.h"

#include "ah_common/settings.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
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

    namespace fs = std::filesystem;
    const fs::path settings_path(settings.Path());
    const fs::path models = settings_path.parent_path() / "models";
    if (fs::is_directory(models)) {
      CopyToBuf(out->models_dir, sizeof(out->models_dir), models.string());
      if (std::getenv("AERO_HUB_MODELS") == nullptr || std::string(std::getenv("AERO_HUB_MODELS")).empty()) {
        setenv("AERO_HUB_MODELS", models.string().c_str(), 1);
      }
    }
    return 0;
  } catch (...) {
    return 2;
  }
}
