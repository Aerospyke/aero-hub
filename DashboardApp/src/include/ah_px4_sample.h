#pragma once

// Plain samples copied out of PX4 callbacks before the ROS thread returns.
// Attitude is NED, quaternion order w, x, y, z (Hamilton), body FRD -> world.

#include <cmath>
#include <cstdint>

struct AhPx4Attitude {
  double roll_deg = 0.0;
  double pitch_deg = 0.0;
  double heading_deg = 0.0;
};

/// Yaw/pitch/roll from a NED quaternion. Heading is degrees clockwise from north, in [0, 360).
/// Returns false when the quaternion is non-finite or zero (PX4 uses NaN for an unknown attitude).
inline bool AhPx4AttitudeFromQuaternion(float w, float x, float y, float z, AhPx4Attitude& out) {
  if (!std::isfinite(w) || !std::isfinite(x) || !std::isfinite(y) || !std::isfinite(z)) {
    return false;
  }
  const double n2 = static_cast<double>(w) * w + static_cast<double>(x) * x + static_cast<double>(y) * y +
                    static_cast<double>(z) * z;
  if (!(n2 > 1e-8)) {
    return false;
  }
  const double n = std::sqrt(n2);
  const double qw = static_cast<double>(w) / n;
  const double qx = static_cast<double>(x) / n;
  const double qy = static_cast<double>(y) / n;
  const double qz = static_cast<double>(z) / n;

  constexpr double kRadToDeg = 180.0 / 3.14159265358979323846;

  // Aerospace 3-2-1 (yaw, pitch, roll), same mapping PX4 uses for FRD -> NED.
  const double sinr_cosp = 2.0 * (qw * qx + qy * qz);
  const double cosr_cosp = 1.0 - 2.0 * (qx * qx + qy * qy);
  out.roll_deg = std::atan2(sinr_cosp, cosr_cosp) * kRadToDeg;

  double sinp = 2.0 * (qw * qy - qz * qx);
  if (sinp > 1.0) {
    sinp = 1.0;
  } else if (sinp < -1.0) {
    sinp = -1.0;
  }
  out.pitch_deg = std::asin(sinp) * kRadToDeg;

  const double siny_cosp = 2.0 * (qw * qz + qx * qy);
  const double cosy_cosp = 1.0 - 2.0 * (qy * qy + qz * qz);
  double heading = std::atan2(siny_cosp, cosy_cosp) * kRadToDeg;
  if (heading < 0.0) {
    heading += 360.0;
  }
  if (heading >= 360.0) {
    heading = 0.0;
  }
  out.heading_deg = heading;
  return true;
}

struct AhPx4OdomSample {
  float north_m = 0.f;
  float east_m = 0.f;
  float down_m = 0.f;
  bool position_valid = false;
  double roll_deg = 0.0;
  double pitch_deg = 0.0;
  double heading_deg = 0.0;
  bool attitude_valid = false;
};

struct AhPx4StatusSample {
  std::uint8_t arming_state = 0;
  std::uint8_t nav_state = 0;
  std::uint8_t system_id = 0;
  std::uint8_t vehicle_type = 0;
  bool is_vtol = false;
};

struct AhPx4GpsSample {
  std::uint8_t fix_type = 0;
  int satellites_used = 0;
};
