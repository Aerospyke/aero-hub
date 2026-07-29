#pragma once

/// Relative graph names (no leading '/'). With an empty ROS namespace they
/// resolve to /ah/...; with namespace "uav1" they become /uav1/ah/...
/// Absolute names (/ah/...) would ignore the node namespace.
namespace ah_ros_names {

inline constexpr char StatusTopic[] = "ah/system/status";
inline constexpr char VideoCompressedTopic[] = "ah/video/compressed";
inline constexpr char TrackingStartService[] = "ah/tracking/start";
inline constexpr char TrackingStopService[] = "ah/tracking/stop";
inline constexpr char TrackingCancelService[] = "ah/tracking/cancel";

}  // namespace ah_ros_names
