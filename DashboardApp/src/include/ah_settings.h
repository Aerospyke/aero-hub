#pragma once

#include <cstddef>
#include <cstdint>

#include <QSettings>
#include <QString>

/// Loads aerohub_settings.ini from the process working directory and exposes
/// accessors for the rest of the app.
///
/// Always starts from built-in defaults, then overlays only keys present in the
/// INI (a partial file still yields a complete, valid settings set). If the file
/// is missing, defaults are written to a new aerohub_settings.ini.
class AhSettings final {
 public:
  static constexpr std::uint8_t DefaultRosDomainId = 42;

  AhSettings();

  AhSettings(const AhSettings&) = delete;
  AhSettings& operator=(const AhSettings&) = delete;

  [[nodiscard]] const QString& Path() const { return path_; }
  [[nodiscard]] bool WasSettingsFileLoaded() const { return was_settings_file_loaded_; }
  [[nodiscard]] QSettings& Settings() { return settings_; }
  [[nodiscard]] const QSettings& Settings() const { return settings_; }

  /// Value of [ROS]/domain_id (default DefaultRosDomainId if not representable
  /// as uint8_t). Range check vs MaxRosDomainId is done by AhRosBridge when the
  /// value is applied.
  [[nodiscard]] std::uint8_t RosDomainId() const;

 private:
  static constexpr char SettingsFileName[] = "aerohub_settings.ini";

  /// Key/value default entry (value stored as string; QSettings converts as needed).
  struct SettingDefault {
    const char* key;
    const char* value;
  };

  static const SettingDefault Defaults[];
  static const std::size_t DefaultsCount;

  void PopulateDefaults();
  void WriteSettingsFile();

  QString path_;
  bool was_settings_file_loaded_ = false;
  QSettings settings_;
};
