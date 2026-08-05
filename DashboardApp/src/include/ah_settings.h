#pragma once

#include "ah_common/settings.hpp"

#include <cstdint>

#include <QString>

/// Qt-facing thin wrapper around ah::Settings (std-only AhCommon).
/// Prefer settings.Ros().DomainId() via Core() when writing new code.
class AhSettings final {
 public:
  AhSettings();

  AhSettings(const AhSettings&) = delete;
  AhSettings& operator=(const AhSettings&) = delete;

  [[nodiscard]] ah::Settings& Core() { return settings_; }

  [[nodiscard]] const ah::Settings& Core() const { return settings_; }

  [[nodiscard]] QString Path() const { return QString::fromStdString(settings_.Path()); }

  [[nodiscard]] bool WasSettingsFileLoaded() const { return settings_.WasFileLoaded(); }

  [[nodiscard]] std::uint8_t RosDomainId() const { return settings_.Ros().DomainId(); }

  [[nodiscard]] QString RosNamespace() const { return QString::fromStdString(settings_.Ros().NamespaceName()); }

 private:
  ah::Settings settings_;
};
