#pragma once

// PX4 vehicle_odometry (NED, body FRD) to a flat ENU pose for slam_toolbox.
// Position: x = east, y = north, z = 0.
// Yaw: yaw_enu = pi/2 - yaw_ned, wrapped to (-pi, pi]. East is 0, north is +pi/2.
// Roll and pitch are dropped on purpose; the rover twin's down estimate drifts.
// Twist is body FLU: vx = forward, vy = left, vz = 0, yaw rate = -(body down rate).

#include <cmath>
#include <cstdint>

namespace ah_px4_odom {

inline constexpr std::uint8_t kVelocityFrameNed = 1;
inline constexpr std::uint8_t kVelocityFrameBodyFrd = 3;

struct Px4OdomSample {
  float north_m = 0.f;
  float east_m = 0.f;
  float down_m = 0.f;
  float qw = 1.f;
  float qx = 0.f;
  float qy = 0.f;
  float qz = 0.f;
  std::uint8_t velocity_frame = kVelocityFrameNed;
  float velocity[3] = {0.f, 0.f, 0.f};
  float angular_velocity_frd[3] = {0.f, 0.f, 0.f};
};

struct FlatOdom {
  double x_m = 0.0;
  double y_m = 0.0;
  double z_m = 0.0;
  double yaw_enu_rad = 0.0;
  double qw = 1.0;
  double qx = 0.0;
  double qy = 0.0;
  double qz = 0.0;
  double vx_m_s = 0.0;
  double vy_m_s = 0.0;
  double vz_m_s = 0.0;
  double yaw_rate_rad_s = 0.0;
};

inline constexpr double kPi = 3.14159265358979323846;

inline double WrapPi(double angle) {
  double wrapped = std::fmod(angle + kPi, 2.0 * kPi);
  if (wrapped < 0.0) {
    wrapped += 2.0 * kPi;
  }
  return wrapped - kPi;
}

inline bool Finite3(float a, float b, float c) {
  return std::isfinite(a) && std::isfinite(b) && std::isfinite(c);
}

/// Aerospace yaw of a body-FRD to NED quaternion. Positive is clockwise from north.
/// q is Hamiltonian w, x, y, z. Returns false when the quaternion is NaN or zero.
inline bool YawNedRad(float w, float x, float y, float z, double& yaw_ned) {
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
  const double sin_yaw = 2.0 * (qw * qz + qx * qy);
  const double cos_yaw = 1.0 - 2.0 * (qy * qy + qz * qz);
  yaw_ned = std::atan2(sin_yaw, cos_yaw);
  return true;
}

/// False when position or attitude is NaN. Invalid velocity becomes a zero twist.
inline bool FlattenOdometry(const Px4OdomSample& in, FlatOdom& out) {
  if (!Finite3(in.north_m, in.east_m, in.down_m)) {
    return false;
  }
  double yaw_ned = 0.0;
  if (!YawNedRad(in.qw, in.qx, in.qy, in.qz, yaw_ned)) {
    return false;
  }

  out = FlatOdom{};
  out.x_m = in.east_m;
  out.y_m = in.north_m;
  out.z_m = 0.0;
  out.yaw_enu_rad = WrapPi((kPi / 2.0) - yaw_ned);
  const double half = out.yaw_enu_rad * 0.5;
  out.qw = std::cos(half);
  out.qx = 0.0;
  out.qy = 0.0;
  out.qz = std::sin(half);

  if (Finite3(in.velocity[0], in.velocity[1], in.velocity[2])) {
    if (in.velocity_frame == kVelocityFrameBodyFrd) {
      out.vx_m_s = in.velocity[0];
      out.vy_m_s = -static_cast<double>(in.velocity[1]);
    } else {
      const double c = std::cos(yaw_ned);
      const double s = std::sin(yaw_ned);
      const double forward = static_cast<double>(in.velocity[0]) * c + static_cast<double>(in.velocity[1]) * s;
      const double right = -static_cast<double>(in.velocity[0]) * s + static_cast<double>(in.velocity[1]) * c;
      out.vx_m_s = forward;
      out.vy_m_s = -right;
    }
  }
  if (std::isfinite(in.angular_velocity_frd[2])) {
    out.yaw_rate_rad_s = -static_cast<double>(in.angular_velocity_frd[2]);
  }
  return true;
}

}  // namespace ah_px4_odom
