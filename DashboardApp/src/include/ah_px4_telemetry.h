#pragma once

#include "ah_px4_sample.h"

#include <QObject>
#include <QString>
#include <QTimer>

/// Live PX4 state for one vehicle namespace. ROS callbacks copy a sample, then
/// Apply* runs on the Qt thread. Each stream goes stale after ~2 s of silence.
class AhPx4Telemetry final : public QObject {
  Q_OBJECT

  Q_PROPERTY(QString vehicleNamespace READ VehicleNamespace CONSTANT)
  /// "waiting" (no PX4 message yet) | "live" | "stale" (had data, then silence)
  Q_PROPERTY(QString linkState READ LinkState NOTIFY linkStateChanged)
  Q_PROPERTY(bool odometryFresh READ OdometryFresh NOTIFY odometryFreshChanged)
  Q_PROPERTY(bool statusFresh READ StatusFresh NOTIFY statusFreshChanged)
  Q_PROPERTY(bool gpsFresh READ GpsFresh NOTIFY gpsFreshChanged)
  Q_PROPERTY(bool hasOdometry READ HasOdometry NOTIFY odometryChanged)
  Q_PROPERTY(bool positionValid READ PositionValid NOTIFY odometryChanged)
  Q_PROPERTY(bool attitudeValid READ AttitudeValid NOTIFY odometryChanged)
  Q_PROPERTY(double northM READ NorthM NOTIFY odometryChanged)
  Q_PROPERTY(double eastM READ EastM NOTIFY odometryChanged)
  Q_PROPERTY(double downM READ DownM NOTIFY odometryChanged)
  Q_PROPERTY(double headingDeg READ HeadingDeg NOTIFY odometryChanged)
  Q_PROPERTY(double rollDeg READ RollDeg NOTIFY odometryChanged)
  Q_PROPERTY(double pitchDeg READ PitchDeg NOTIFY odometryChanged)
  Q_PROPERTY(bool hasStatus READ HasStatus NOTIFY statusChanged)
  Q_PROPERTY(QString armingState READ ArmingState NOTIFY statusChanged)
  Q_PROPERTY(QString navState READ NavState NOTIFY statusChanged)
  /// MAVLink system id from vehicle_status_v1. Display only; the namespace is the identity.
  Q_PROPERTY(int systemId READ SystemId NOTIFY statusChanged)
  /// Rover, Multicopter, Fixed wing, or VTOL. Display only.
  Q_PROPERTY(QString vehicleType READ VehicleType NOTIFY statusChanged)
  Q_PROPERTY(bool hasGps READ HasGps NOTIFY gpsChanged)
  /// Fix type 4 is "DGPS/SBAS". The label never uses a carrier-phase name for that code.
  Q_PROPERTY(QString gpsFixType READ GpsFixType NOTIFY gpsChanged)
  Q_PROPERTY(int satellitesUsed READ SatellitesUsed NOTIFY gpsChanged)

 public:
  static constexpr int StaleTimeoutMs = 2000;

  explicit AhPx4Telemetry(QString vehicle_namespace, QObject* parent = nullptr);

  [[nodiscard]] QString VehicleNamespace() const { return vehicle_namespace_; }
  [[nodiscard]] QString LinkState() const { return link_state_; }
  [[nodiscard]] bool OdometryFresh() const { return odometry_fresh_; }
  [[nodiscard]] bool StatusFresh() const { return status_fresh_; }
  [[nodiscard]] bool GpsFresh() const { return gps_fresh_; }
  [[nodiscard]] bool HasOdometry() const { return has_odometry_; }
  [[nodiscard]] bool PositionValid() const { return position_valid_; }
  [[nodiscard]] bool AttitudeValid() const { return attitude_valid_; }
  [[nodiscard]] double NorthM() const { return north_m_; }
  [[nodiscard]] double EastM() const { return east_m_; }
  [[nodiscard]] double DownM() const { return down_m_; }
  [[nodiscard]] double HeadingDeg() const { return heading_deg_; }
  [[nodiscard]] double RollDeg() const { return roll_deg_; }
  [[nodiscard]] double PitchDeg() const { return pitch_deg_; }
  [[nodiscard]] bool HasStatus() const { return has_status_; }
  [[nodiscard]] QString ArmingState() const { return arming_state_; }
  [[nodiscard]] QString NavState() const { return nav_state_; }
  [[nodiscard]] int SystemId() const { return system_id_; }
  [[nodiscard]] QString VehicleType() const { return vehicle_type_; }
  [[nodiscard]] bool HasGps() const { return has_gps_; }
  [[nodiscard]] QString GpsFixType() const { return gps_fix_type_; }
  [[nodiscard]] int SatellitesUsed() const { return satellites_used_; }

  void ApplyOdometry(AhPx4OdomSample sample);
  void ApplyStatus(AhPx4StatusSample sample);
  void ApplyGps(AhPx4GpsSample sample);

 signals:
  void linkStateChanged();
  void odometryFreshChanged();
  void statusFreshChanged();
  void gpsFreshChanged();
  void odometryChanged();
  void statusChanged();
  void gpsChanged();

 private slots:
  void CheckFreshness();

 private:
  void SetOdometryFresh(bool fresh);
  void SetStatusFresh(bool fresh);
  void SetGpsFresh(bool fresh);
  void UpdateLinkState();

  QString vehicle_namespace_;
  QTimer freshness_timer_;
  QString link_state_ = QStringLiteral("waiting");

  bool odometry_fresh_ = false;
  bool status_fresh_ = false;
  bool gps_fresh_ = false;
  bool ever_odom_ = false;
  bool ever_status_ = false;
  bool ever_gps_ = false;
  qint64 last_odom_ms_ = 0;
  qint64 last_status_ms_ = 0;
  qint64 last_gps_ms_ = 0;

  bool has_odometry_ = false;
  bool position_valid_ = false;
  bool attitude_valid_ = false;
  double north_m_ = 0.0;
  double east_m_ = 0.0;
  double down_m_ = 0.0;
  double heading_deg_ = 0.0;
  double roll_deg_ = 0.0;
  double pitch_deg_ = 0.0;

  bool has_status_ = false;
  QString arming_state_ = QStringLiteral("—");
  QString nav_state_ = QStringLiteral("—");
  int system_id_ = 0;
  QString vehicle_type_ = QStringLiteral("—");

  bool has_gps_ = false;
  QString gps_fix_type_ = QStringLiteral("—");
  int satellites_used_ = 0;
};
