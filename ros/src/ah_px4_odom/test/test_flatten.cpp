// Checks the slam_toolbox heading rule: east is yaw 0, north is +90 degrees.
// Also checks that a level FRD->NED quaternion, converted NED->ENU and FRD->FLU,
// lands on that same yaw. A missing body-frame flip mirrors yaw.

#include "ah_px4_odom/flatten.hpp"

#include <cmath>
#include <cstdlib>
#include <iostream>

namespace {

int g_fails = 0;

void Expect(bool ok, const char* text, int line) {
  if (!ok) {
    std::cerr << "FAIL " << line << ": " << text << '\n';
    ++g_fails;
  }
}

bool Near(double a, double b, double tol = 1e-6) { return std::fabs(a - b) < tol; }

bool AngleNear(double a, double b, double tol = 1e-6) {
  return Near(ah_px4_odom::WrapPi(a - b), 0.0, tol);
}

struct Q {
  double w, x, y, z;
};

Q Mul(Q a, Q b) {
  return {a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z, a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y,
          a.w * b.y - a.x * b.z + a.y * b.w + a.z * b.x, a.w * b.z + a.x * b.y - a.y * b.x + a.z * b.w};
}

double YawOf(Q q) {
  const double sin_yaw = 2.0 * (q.w * q.z + q.x * q.y);
  const double cos_yaw = 1.0 - 2.0 * (q.y * q.y + q.z * q.z);
  return std::atan2(sin_yaw, cos_yaw);
}

Q YawNedQuat(double yaw_ned) {
  return {std::cos(yaw_ned * 0.5), 0.0, 0.0, std::sin(yaw_ned * 0.5)};
}

void FillYaw(ah_px4_odom::Px4OdomSample& sample, double yaw_ned) {
  const Q q = YawNedQuat(yaw_ned);
  sample.qw = static_cast<float>(q.w);
  sample.qx = static_cast<float>(q.x);
  sample.qy = static_cast<float>(q.y);
  sample.qz = static_cast<float>(q.z);
}

}  // namespace

int main() {
  using ah_px4_odom::FlatOdom;
  using ah_px4_odom::FlattenOdometry;
  using ah_px4_odom::Px4OdomSample;
  using ah_px4_odom::kPi;
  using ah_px4_odom::kVelocityFrameBodyFrd;

  {
    Px4OdomSample sample;
    sample.north_m = 5.f;
    sample.east_m = 2.f;
    sample.down_m = 4.f;
    FillYaw(sample, 0.0);
    FlatOdom flat;
    Expect(FlattenOdometry(sample, flat), "north-facing sample", __LINE__);
    Expect(Near(flat.x_m, 2.0) && Near(flat.y_m, 5.0) && Near(flat.z_m, 0.0), "east, north, flat z", __LINE__);
    Expect(AngleNear(flat.yaw_enu_rad, kPi / 2.0), "facing north is +90 deg", __LINE__);
  }

  {
    Px4OdomSample sample;
    FillYaw(sample, kPi / 2.0);
    FlatOdom flat;
    Expect(FlattenOdometry(sample, flat), "east-facing sample", __LINE__);
    Expect(AngleNear(flat.yaw_enu_rad, 0.0), "facing east is 0 deg", __LINE__);
  }

  {
    Px4OdomSample sample;
    FillYaw(sample, kPi);
    FlatOdom flat;
    Expect(FlattenOdometry(sample, flat), "south-facing sample", __LINE__);
    Expect(AngleNear(flat.yaw_enu_rad, -kPi / 2.0), "facing south is -90 deg", __LINE__);
  }

  {
    Px4OdomSample left;
    Px4OdomSample right;
    FillYaw(left, -0.2);
    FillYaw(right, 0.2);
    FlatOdom flat_left;
    FlatOdom flat_right;
    Expect(FlattenOdometry(left, flat_left) && FlattenOdometry(right, flat_right), "turn samples", __LINE__);
    Expect(ah_px4_odom::WrapPi(flat_left.yaw_enu_rad - flat_right.yaw_enu_rad) > 0.0, "left turn increases yaw",
           __LINE__);
  }

  {
    const Q kNedToEnu{0.0, std::sqrt(0.5), std::sqrt(0.5), 0.0};
    const Q kFrdToFlu{0.0, 1.0, 0.0, 0.0};
    const double yaws[] = {0.0, kPi / 2.0, -kPi / 2.0, 1.0};
    for (const double yaw_ned : yaws) {
      const Q body = YawNedQuat(yaw_ned);
      const Q enu_flu = Mul(Mul(kNedToEnu, body), kFrdToFlu);
      Px4OdomSample sample;
      FillYaw(sample, yaw_ned);
      FlatOdom flat;
      Expect(FlattenOdometry(sample, flat), "chain sample", __LINE__);
      Expect(AngleNear(flat.yaw_enu_rad, YawOf(enu_flu)), "chain matches pi/2 - yaw_ned", __LINE__);
    }
  }

  {
    Px4OdomSample sample;
    FillYaw(sample, 0.0);
    sample.velocity[0] = 1.f;
    sample.velocity[1] = 0.f;
    FlatOdom flat;
    Expect(FlattenOdometry(sample, flat), "north velocity", __LINE__);
    Expect(Near(flat.vx_m_s, 1.0) && Near(flat.vy_m_s, 0.0) && Near(flat.vz_m_s, 0.0), "north speed is forward",
           __LINE__);
  }

  {
    Px4OdomSample sample;
    FillYaw(sample, 0.0);
    sample.velocity[0] = 0.f;
    sample.velocity[1] = 1.f;
    FlatOdom flat;
    Expect(FlattenOdometry(sample, flat), "east velocity", __LINE__);
    Expect(Near(flat.vx_m_s, 0.0) && Near(flat.vy_m_s, -1.0), "east speed while facing north is right", __LINE__);
  }

  {
    Px4OdomSample sample;
    FillYaw(sample, kPi / 2.0);
    sample.velocity[0] = 0.f;
    sample.velocity[1] = 1.5f;
    FlatOdom flat;
    Expect(FlattenOdometry(sample, flat), "east velocity facing east", __LINE__);
    Expect(Near(flat.vx_m_s, 1.5) && Near(flat.vy_m_s, 0.0), "east speed while facing east is forward", __LINE__);
  }

  {
    Px4OdomSample sample;
    FillYaw(sample, 0.4);
    sample.velocity_frame = kVelocityFrameBodyFrd;
    sample.velocity[0] = 1.f;
    sample.velocity[1] = 2.f;
    sample.velocity[2] = 9.f;
    sample.angular_velocity_frd[2] = 0.3f;
    FlatOdom flat;
    Expect(FlattenOdometry(sample, flat), "body velocity", __LINE__);
    Expect(Near(flat.vx_m_s, 1.0) && Near(flat.vy_m_s, -2.0) && Near(flat.vz_m_s, 0.0), "FRD to FLU", __LINE__);
    Expect(Near(flat.yaw_rate_rad_s, -0.3), "yaw rate flips about down", __LINE__);
  }

  {
    Px4OdomSample sample;
    sample.north_m = std::nanf("");
    FillYaw(sample, 0.0);
    FlatOdom flat;
    Expect(!FlattenOdometry(sample, flat), "NaN position is dropped", __LINE__);
  }

  {
    Px4OdomSample sample;
    sample.qw = std::nanf("");
    FlatOdom flat;
    Expect(!FlattenOdometry(sample, flat), "NaN attitude is dropped", __LINE__);
  }

  {
    Px4OdomSample sample;
    sample.qw = 0.f;
    sample.qx = 0.f;
    sample.qy = 0.f;
    sample.qz = 0.f;
    FlatOdom flat;
    Expect(!FlattenOdometry(sample, flat), "zero quaternion is dropped", __LINE__);
  }

  {
    Px4OdomSample sample;
    FillYaw(sample, 0.0);
    sample.velocity[0] = std::nanf("");
    FlatOdom flat;
    Expect(FlattenOdometry(sample, flat), "NaN velocity still publishes pose", __LINE__);
    Expect(Near(flat.vx_m_s, 0.0) && Near(flat.vy_m_s, 0.0), "invalid velocity is zero twist", __LINE__);
    Expect(AngleNear(flat.yaw_enu_rad, kPi / 2.0), "pose yaw survives bad velocity", __LINE__);
  }

  if (g_fails != 0) {
    std::cerr << g_fails << " check(s) failed\n";
    return EXIT_FAILURE;
  }
  std::cout << "ah_px4_odom flatten checks passed\n";
  return EXIT_SUCCESS;
}
