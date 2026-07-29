#include "ah_settings.h"

#include <QDir>
#include <QFile>
#include <QHash>
#include <QStringList>
#include <QVariant>
#include <QtGlobal>

// Keep in sync with aerohub_settings_template.ini
const AhSettings::SettingDefault AhSettings::Defaults[] = {
    // [ROS]
    {"ROS/domain_id", "42"},

    // [JSBSim/command_line]
    {"JSBSim/command_line/executable", ""},
    {"JSBSim/command_line/realtime", "--realtime"},
    {"JSBSim/command_line/aircraft", "--aircraft=737"},
    {"JSBSim/command_line/initfile", "--initfile=reset00"},
    {"JSBSim/command_line/arg1", "--property=propulsion/engine[0]/set-running=1"},
    {"JSBSim/command_line/arg2", "--property=propulsion/engine[1]/set-running=1"},
    {"JSBSim/command_line/arg3", "--suspend"},
    {"JSBSim/command_line/arg4", ""},
    {"JSBSim/command_line/arg5", ""},
    {"JSBSim/command_line/arg6", ""},

    // [JSBSim/ports]
    {"JSBSim/ports/input", "5138"},
    {"JSBSim/ports/output", "5139"},
    {"JSBSim/ports/telnet", "5137"},
    {"JSBSim/ports/flightgear", "5508"},

    // [JSBSim/rates]
    {"JSBSim/rates/UI", "100"},
    {"JSBSim/rates/output", "10"},
    {"JSBSim/rates/FlightGear", "30"},

    // [JSBSim/aircraft]
    {"JSBSim/aircraft/pitch-trim", "-0.32"},
    {"JSBSim/aircraft/pitch-trim-rate", "0.2"},

    // [JSBSim/joystick]
    {"JSBSim/joystick/elevator-axis", "1"},
    {"JSBSim/joystick/aileron-axis", "0"},
    {"JSBSim/joystick/rudder-axis", "2"},
    {"JSBSim/joystick/throttle-axis", "3"},
    {"JSBSim/joystick/aileron-trim-axis", "4"},
    {"JSBSim/joystick/elevator-trim-axis", "5"},
    {"JSBSim/joystick/axis-0-deadband", "0.04"},
    {"JSBSim/joystick/axis-1-deadband", "0.04"},
    {"JSBSim/joystick/axis-2-deadband", "0.07"},

    // [JSBSim/airport]
    {"JSBSim/airport/magvar", "12.0"},
    {"JSBSim/airport/runway-length-ft", "11095"},
    {"JSBSim/airport/ILS-runway-near-latitude", "33.937363033"},
    {"JSBSim/airport/ILS-runway-near-longitude", "-118.382713917"},
    {"JSBSim/airport/ILS-runway-far-latitude", "33.933649383"},
    {"JSBSim/airport/ILS-runway-far-longitude", "-118.419018333"},
    {"JSBSim/airport/ILS-frequency", "109.9"},
    {"JSBSim/airport/ILS-course-mag", "251.0"},
    {"JSBSim/airport/ILS-GS", "3.0"},
    {"JSBSim/airport/ILS-TDZE", "97.8"},
};

const std::size_t AhSettings::DefaultsCount = sizeof(AhSettings::Defaults) / sizeof(AhSettings::Defaults[0]);

AhSettings::AhSettings()
    : path_(QDir::current().absoluteFilePath(QString::fromUtf8(SettingsFileName))),
      was_settings_file_loaded_(QFile::exists(path_)),
      settings_(path_, QSettings::IniFormat) {
  // Snapshot INI keys before we rebuild from defaults (file may be a subset).
  QHash<QString, QVariant> file_overrides;
  if (was_settings_file_loaded_) {
    const QStringList Keys = settings_.allKeys();
    for (const QString& key : Keys) {
      file_overrides.insert(key, settings_.value(key));
    }
  }

  settings_.clear();
  PopulateDefaults();

  for (auto it = file_overrides.constBegin(); it != file_overrides.constEnd(); ++it) {
    settings_.setValue(it.key(), it.value());
  }

  if (!was_settings_file_loaded_) {
    qWarning(R"(AhSettings: "%s" not found in working directory "%s"; writing defaults)",
             SettingsFileName, qPrintable(QDir::currentPath()));
    WriteSettingsFile();
  }
}

void AhSettings::PopulateDefaults() {
  for (std::size_t i = 0; i < DefaultsCount; ++i) {
    const SettingDefault& entry = Defaults[i];
    settings_.setValue(QString::fromUtf8(entry.key), QString::fromUtf8(entry.value));
  }
}

void AhSettings::OverlayFromFile() {
  // Overlay is applied in the constructor after PopulateDefaults().
}

void AhSettings::WriteSettingsFile() {
  settings_.sync();
  if (settings_.status() != QSettings::NoError || !QFile::exists(path_)) {
    qWarning(R"(AhSettings: failed to write defaults to "%s")", qPrintable(path_));
    return;
  }
  qInfo(R"(AhSettings: created "%s" with default values)", qPrintable(path_));
}

std::uint8_t AhSettings::RosDomainId() const {
  const int Value = settings_.value(QStringLiteral("ROS/domain_id"), DefaultRosDomainId).toInt();
  // INI can hold any integer; only values that fit in uint8_t are passed through.
  if (Value < 0 || Value > 255) {
    return DefaultRosDomainId;
  }
  return static_cast<std::uint8_t>(Value);
}
