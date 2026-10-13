#include "ah_px4_telemetry.h"

#include "px4_msgs/msg/sensor_gps.hpp"
#include "px4_msgs/msg/vehicle_status.hpp"

#include <QDateTime>

namespace {

QString ArmingName(std::uint8_t state) {
  using Status = px4_msgs::msg::VehicleStatus;
  switch (state) {
    case Status::ARMING_STATE_DISARMED:
      return QStringLiteral("Disarmed");
    case Status::ARMING_STATE_ARMED:
      return QStringLiteral("Armed");
    default:
      return QStringLiteral("Unknown (%1)").arg(state);
  }
}

QString NavName(std::uint8_t nav) {
  using Status = px4_msgs::msg::VehicleStatus;
  switch (nav) {
    case Status::NAVIGATION_STATE_MANUAL:
      return QStringLiteral("Manual");
    case Status::NAVIGATION_STATE_ALTCTL:
      return QStringLiteral("Altitude");
    case Status::NAVIGATION_STATE_POSCTL:
      return QStringLiteral("Position");
    case Status::NAVIGATION_STATE_AUTO_MISSION:
      return QStringLiteral("Mission");
    case Status::NAVIGATION_STATE_AUTO_LOITER:
      return QStringLiteral("Hold");
    case Status::NAVIGATION_STATE_AUTO_RTL:
      return QStringLiteral("Return");
    case Status::NAVIGATION_STATE_POSITION_SLOW:
      return QStringLiteral("Position (slow)");
    case Status::NAVIGATION_STATE_FREE5:
      return QStringLiteral("Free 5");
    case Status::NAVIGATION_STATE_ALTITUDE_CRUISE:
      return QStringLiteral("Altitude cruise");
    case Status::NAVIGATION_STATE_FREE3:
      return QStringLiteral("Free 3");
    case Status::NAVIGATION_STATE_ACRO:
      return QStringLiteral("Acro");
    case Status::NAVIGATION_STATE_FREE2:
      return QStringLiteral("Free 2");
    case Status::NAVIGATION_STATE_DESCEND:
      return QStringLiteral("Descend");
    case Status::NAVIGATION_STATE_TERMINATION:
      return QStringLiteral("Termination");
    case Status::NAVIGATION_STATE_OFFBOARD:
      return QStringLiteral("Offboard");
    case Status::NAVIGATION_STATE_STAB:
      return QStringLiteral("Stabilized");
    case Status::NAVIGATION_STATE_FREE1:
      return QStringLiteral("Free 1");
    case Status::NAVIGATION_STATE_AUTO_TAKEOFF:
      return QStringLiteral("Takeoff");
    case Status::NAVIGATION_STATE_AUTO_LAND:
      return QStringLiteral("Land");
    case Status::NAVIGATION_STATE_AUTO_FOLLOW_TARGET:
      return QStringLiteral("Follow target");
    case Status::NAVIGATION_STATE_AUTO_PRECLAND:
      return QStringLiteral("Precision land");
    case Status::NAVIGATION_STATE_ORBIT:
      return QStringLiteral("Orbit");
    case Status::NAVIGATION_STATE_AUTO_VTOL_TAKEOFF:
      return QStringLiteral("VTOL takeoff");
    case Status::NAVIGATION_STATE_EXTERNAL1:
      return QStringLiteral("External 1");
    case Status::NAVIGATION_STATE_EXTERNAL2:
      return QStringLiteral("External 2");
    case Status::NAVIGATION_STATE_EXTERNAL3:
      return QStringLiteral("External 3");
    case Status::NAVIGATION_STATE_EXTERNAL4:
      return QStringLiteral("External 4");
    case Status::NAVIGATION_STATE_EXTERNAL5:
      return QStringLiteral("External 5");
    case Status::NAVIGATION_STATE_EXTERNAL6:
      return QStringLiteral("External 6");
    case Status::NAVIGATION_STATE_EXTERNAL7:
      return QStringLiteral("External 7");
    case Status::NAVIGATION_STATE_EXTERNAL8:
      return QStringLiteral("External 8");
    default:
      return QStringLiteral("Unknown (%1)").arg(nav);
  }
}

// PX4 SensorGps fix_type 4 is code-differential (DGPS/SBAS). Do not label it as a carrier-phase fix.
QString GpsFixName(std::uint8_t fix) {
  using Gps = px4_msgs::msg::SensorGps;
  switch (fix) {
    case 0:
      return QStringLiteral("No fix");
    case Gps::FIX_TYPE_NONE:
      return QStringLiteral("None");
    case Gps::FIX_TYPE_2D:
      return QStringLiteral("2D");
    case Gps::FIX_TYPE_3D:
      return QStringLiteral("3D");
    case Gps::FIX_TYPE_RTCM_CODE_DIFFERENTIAL:
      return QStringLiteral("DGPS/SBAS");
    case Gps::FIX_TYPE_RTK_FLOAT:
      return QStringLiteral("Float");
    case Gps::FIX_TYPE_RTK_FIXED:
      return QStringLiteral("Fixed");
    case Gps::FIX_TYPE_EXTRAPOLATED:
      return QStringLiteral("Extrapolated");
    default:
      return QStringLiteral("Unknown (%1)").arg(fix);
  }
}

}  // namespace

AhPx4Telemetry::AhPx4Telemetry(QString vehicle_namespace, QObject* parent)
    : QObject(parent), vehicle_namespace_(std::move(vehicle_namespace)) {
  freshness_timer_.setInterval(250);
  QObject::connect(&freshness_timer_, &QTimer::timeout, this, &AhPx4Telemetry::CheckFreshness);
  freshness_timer_.start();
}

void AhPx4Telemetry::SetOdometryFresh(bool fresh) {
  if (odometry_fresh_ == fresh) {
    return;
  }
  odometry_fresh_ = fresh;
  emit odometryFreshChanged();
  UpdateLinkState();
}

void AhPx4Telemetry::SetStatusFresh(bool fresh) {
  if (status_fresh_ == fresh) {
    return;
  }
  status_fresh_ = fresh;
  emit statusFreshChanged();
  UpdateLinkState();
}

void AhPx4Telemetry::SetGpsFresh(bool fresh) {
  if (gps_fresh_ == fresh) {
    return;
  }
  gps_fresh_ = fresh;
  emit gpsFreshChanged();
  UpdateLinkState();
}

void AhPx4Telemetry::UpdateLinkState() {
  QString state = QStringLiteral("waiting");
  if (odometry_fresh_ || status_fresh_ || gps_fresh_) {
    state = QStringLiteral("live");
  } else if (ever_odom_ || ever_status_ || ever_gps_) {
    state = QStringLiteral("stale");
  }
  if (link_state_ == state) {
    return;
  }
  link_state_ = state;
  emit linkStateChanged();
}

void AhPx4Telemetry::CheckFreshness() {
  const qint64 now = QDateTime::currentMSecsSinceEpoch();
  if (ever_odom_ && now - last_odom_ms_ > StaleTimeoutMs) {
    SetOdometryFresh(false);
  }
  if (ever_status_ && now - last_status_ms_ > StaleTimeoutMs) {
    SetStatusFresh(false);
  }
  if (ever_gps_ && now - last_gps_ms_ > StaleTimeoutMs) {
    SetGpsFresh(false);
  }
}

void AhPx4Telemetry::ApplyOdometry(AhPx4OdomSample sample) {
  last_odom_ms_ = QDateTime::currentMSecsSinceEpoch();
  ever_odom_ = true;
  has_odometry_ = true;
  SetOdometryFresh(true);

  // A later NaN sample means "unknown right now". Keep the last finite fix so the
  // panel does not flicker back to empty while the stream is still arriving.
  if (sample.position_valid) {
    position_valid_ = true;
    north_m_ = sample.north_m;
    east_m_ = sample.east_m;
    down_m_ = sample.down_m;
  }
  if (sample.attitude_valid) {
    attitude_valid_ = true;
    roll_deg_ = sample.roll_deg;
    pitch_deg_ = sample.pitch_deg;
    heading_deg_ = sample.heading_deg;
  }
  emit odometryChanged();
}

void AhPx4Telemetry::ApplyStatus(AhPx4StatusSample sample) {
  last_status_ms_ = QDateTime::currentMSecsSinceEpoch();
  ever_status_ = true;
  has_status_ = true;
  SetStatusFresh(true);

  arming_state_ = ArmingName(sample.arming_state);
  nav_state_ = NavName(sample.nav_state);
  emit statusChanged();
}

void AhPx4Telemetry::ApplyGps(AhPx4GpsSample sample) {
  last_gps_ms_ = QDateTime::currentMSecsSinceEpoch();
  ever_gps_ = true;
  has_gps_ = true;
  SetGpsFresh(true);

  gps_fix_type_ = GpsFixName(sample.fix_type);
  satellites_used_ = sample.satellites_used;
  emit gpsChanged();
}
