#pragma once

#include "ah_common/settings.hpp"

#include <cstdint>

#include <QString>

/// Qt-facing thin wrapper around ah::Settings (std-only AhCommon).
/// Prefer settings.ros().domainId() via core() when writing new code.
class AhSettings final {
 public:
  AhSettings();

  AhSettings(const AhSettings&) = delete;
  AhSettings& operator=(const AhSettings&) = delete;

  [[nodiscard]] ah::Settings& core() { return settings_; }
  [[nodiscard]] const ah::Settings& core() const { return settings_; }

  [[nodiscard]] QString Path() const { return QString::fromStdString(settings_.path()); }
  [[nodiscard]] bool WasSettingsFileLoaded() const { return settings_.wasFileLoaded(); }

  [[nodiscard]] std::uint8_t RosDomainId() const { return settings_.ros().domainId(); }
  [[nodiscard]] QString RosNamespace() const {
    return QString::fromStdString(settings_.ros().namespaceName());
  }

 private:
  ah::Settings settings_;
};
