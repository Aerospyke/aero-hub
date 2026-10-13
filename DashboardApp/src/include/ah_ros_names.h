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
inline constexpr char CameraListService[] = "ah/camera/list";
inline constexpr char CameraSelectService[] = "ah/camera/select";
inline constexpr char DetectionsTopic[] = "ah/detections";
inline constexpr char AiTrackingToggleService[] = "ah/ai_tracking/toggle";
inline constexpr char AiTrackingClickService[] = "ah/ai_tracking/click";
inline constexpr char YoloSetProfileService[] = "ah/yolo/set_profile";
inline constexpr char YoloReloadService[] = "ah/yolo/reload";

// PX4 uXRCE topics. Relative (no leading '/') so they resolve under the
// vehicle namespace node: namespace "rover" -> /rover/fmu/out/...
inline constexpr char Px4VehicleOdometryTopic[] = "fmu/out/vehicle_odometry";
inline constexpr char Px4VehicleStatusTopic[] = "fmu/out/vehicle_status_v1";
inline constexpr char Px4VehicleGpsTopic[] = "fmu/out/vehicle_gps_position";

}  // namespace ah_ros_names


