#include "ah_settings.h"

#include <QDebug>
#include <QDir>

AhSettings::AhSettings() : settings_(ah::Settings::Load()) {
  if (!settings_.WasFileLoaded()) {
    qWarning().noquote() << QStringLiteral(
                                "AhSettings: \"%1\" not found (or empty path); defaults written if "
                                "save succeeded. cwd=%2")
                                .arg(QString::fromUtf8(ah::Settings::SettingsFileName), QDir::currentPath());
  }
  qInfo().noquote() << QStringLiteral("AhSettings: using \"%1\" (loaded_from_disk=%2)")
                           .arg(Path())
                           .arg(settings_.WasFileLoaded() ? QStringLiteral("true") : QStringLiteral("false"));
}
