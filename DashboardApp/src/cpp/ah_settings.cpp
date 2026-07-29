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
    {.key = "ROS/domain_id", .value = "42"},
    // Empty = root graph (/ah/...). e.g. "uav1" → /uav1/ah/...
    {.key = "ROS/namespace", .value = ""},

    // [JSBSim/command_line]
    {.key = "JSBSim/command_line/executable", .value = ""},
    {.key="JSBSim/command_line/realtime", .value="--realtime"},
    {.key="JSBSim/command_line/aircraft", .value="--aircraft=737"},
    {.key="JSBSim/command_line/initfile", .value="--initfile=reset00"},
    {.key="JSBSim/command_line/arg1", .value="--property=propulsion/engine[0]/set-running=1"},
    {.key="JSBSim/command_line/arg2", .value="--property=propulsion/engine[1]/set-running=1"},
    {.key="JSBSim/command_line/arg3", .value="--suspend"},
    {.key="JSBSim/command_line/arg4", .value=""},
    {.key="JSBSim/command_line/arg5", .value=""},
    {.key="JSBSim/command_line/arg6", .value=""},

    // [JSBSim/ports]
    {.key="JSBSim/ports/input", .value="5138"},
    {.key="JSBSim/ports/output", .value="5139"},
    {.key="JSBSim/ports/telnet", .value="5137"},
    {.key="JSBSim/ports/flightgear", .value="5508"},

    // [JSBSim/rates]
    {.key="JSBSim/rates/UI", .value="100"},
    {.key="JSBSim/rates/output", .value="10"},
    {.key="JSBSim/rates/FlightGear", .value="30"},

    // [JSBSim/aircraft]
    {.key="JSBSim/aircraft/pitch-trim", .value="-0.32"},
    {.key="JSBSim/aircraft/pitch-trim-rate", .value="0.2"},

    // [JSBSim/joystick]
    {.key="JSBSim/joystick/elevator-axis", .value="1"},
    {.key="JSBSim/joystick/aileron-axis", .value="0"},
    {.key="JSBSim/joystick/rudder-axis", .value="2"},
    {.key="JSBSim/joystick/throttle-axis", .value="3"},
    {.key="JSBSim/joystick/aileron-trim-axis", .value="4"},
    {.key="JSBSim/joystick/elevator-trim-axis", .value="5"},
    {.key="JSBSim/joystick/axis-0-deadband", .value="0.04"},
    {.key="JSBSim/joystick/axis-1-deadband", .value="0.04"},
    {.key="JSBSim/joystick/axis-2-deadband", .value="0.07"},

    // [JSBSim/airport]
    {.key="JSBSim/airport/magvar", .value="12.0"},
    {.key="JSBSim/airport/runway-length-ft", .value="11095"},
    {.key="JSBSim/airport/ILS-runway-near-latitude", .value="33.937363033"},
    {.key="JSBSim/airport/ILS-runway-near-longitude", .value="-118.382713917"},
    {.key="JSBSim/airport/ILS-runway-far-latitude", .value="33.933649383"},
    {.key="JSBSim/airport/ILS-runway-far-longitude", .value="-118.419018333"},
    {.key="JSBSim/airport/ILS-frequency", .value="109.9"},
    {.key="JSBSim/airport/ILS-course-mag", .value="251.0"},
    {.key="JSBSim/airport/ILS-GS", .value="3.0"},
    {.key="JSBSim/airport/ILS-TDZE", .value="97.8"},
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
  for (auto [key, value] : Defaults) {
    settings_.setValue(QString::fromUtf8(key), QString::fromUtf8(value));
  }
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

QString AhSettings::RosNamespace() const {
  QString ns = settings_.value(QStringLiteral("ROS/namespace"), QString()).toString().trimmed();
  while (ns.startsWith(QLatin1Char('/'))) {
    ns.remove(0, 1);
  }
  while (ns.endsWith(QLatin1Char('/'))) {
    ns.chop(1);
  }
  // Disallow empty path segments like "uav//1"
  if (ns.contains(QLatin1String("//"))) {
    qWarning("AhSettings: ROS/namespace contains empty segments; using root namespace");
    return {};
  }
  return ns;
}
