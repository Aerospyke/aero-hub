#pragma once

#include "ah_common/settings.hpp"

#include <cstdint>

#include <QString>
#include <QStringList>

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

  /// First PX4 vehicle namespace. Default "rover". Hub and the legacy panel use this.
  [[nodiscard]] QString VehicleNamespace() const {
    return QString::fromStdString(settings_.Vehicle().NamespaceName());
  }

  /// [Vehicle] namespaces, in display order. One entry when the ini only has namespace=.
  [[nodiscard]] QStringList VehicleNamespaces() const {
    QStringList names;
    for (const std::string& name : settings_.Vehicle().NamespaceNames()) {
      names.push_back(QString::fromStdString(name));
    }
    return names;
  }

 private:
  ah::Settings settings_;
};
