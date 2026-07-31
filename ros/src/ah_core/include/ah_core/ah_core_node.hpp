#pragma once

// AhCoreNode — AeroHub core ROS node (status, video, track, camera list/select).

#include "ah_core/camera_devices.hpp"

#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "ah_msgs/srv/list_cameras.hpp"
#include "ah_msgs/srv/select_camera.hpp"
#include "ah_msgs/srv/ai_tracking_click.hpp"
#include "ah_msgs/srv/start_tracking.hpp"
#include "rclcpp/rclcpp.hpp"
#include "sensor_msgs/msg/compressed_image.hpp"
#include "std_msgs/msg/string.hpp"
#include "std_srvs/srv/set_bool.hpp"
#include "std_srvs/srv/trigger.hpp"

namespace ah_core
{

class AhCoreNode : public rclcpp::Node
{
public:
  // Relative topic/service names (no leading /) so node namespace prefixes them.
  // Empty namespace → /ah/... ; namespace "uav1" → /uav1/ah/...
  explicit AhCoreNode(
    const std::string & ros_namespace,
    CameraSelection initial_camera,
    std::string settings_path);

  ~AhCoreNode() override;

private:
  void ApplySelectionToParams();
  void PersistSelectionToSettingsFile();
  /// Open or close long-lived capture to match camera_ (holds camera_mutex_).
  bool SyncCaptureToSelection(std::string * error_out = nullptr);
  void OnTimer();
  void OnStartTracking(
    const std::shared_ptr<ah_msgs::srv::StartTracking::Request> request,
    std::shared_ptr<ah_msgs::srv::StartTracking::Response> response);
  void OnStopTracking(
    const std::shared_ptr<std_srvs::srv::Trigger::Request> request,
    std::shared_ptr<std_srvs::srv::Trigger::Response> response);
  void OnCancelTracking(
    const std::shared_ptr<std_srvs::srv::Trigger::Request> request,
    std::shared_ptr<std_srvs::srv::Trigger::Response> response);
  void OnListCameras(
    const std::shared_ptr<ah_msgs::srv::ListCameras::Request> request,
    std::shared_ptr<ah_msgs::srv::ListCameras::Response> response);
  void OnSelectCamera(
    const std::shared_ptr<ah_msgs::srv::SelectCamera::Request> request,
    std::shared_ptr<ah_msgs::srv::SelectCamera::Response> response);
  void FillSelectResponse(
    std::shared_ptr<ah_msgs::srv::SelectCamera::Response> response) const;
  void OnAiTrackingToggle(
    const std::shared_ptr<std_srvs::srv::SetBool::Request> request,
    std::shared_ptr<std_srvs::srv::SetBool::Response> response);
  void OnAiTrackingClick(
    const std::shared_ptr<ah_msgs::srv::AiTrackingClick::Request> request,
    std::shared_ptr<ah_msgs::srv::AiTrackingClick::Response> response);
  void OnDetections(const std_msgs::msg::String::SharedPtr msg);

  struct DetectionBox
  {
    int track_id{-1};  // Ultralytics multi-object track id (-1 if unknown)
    float x{0.f};
    float y{0.f};
    float w{0.f};
    float h{0.f};
    float confidence{0.f};
    std::string class_name;
  };

  /// Pick lock target from click + last detections (must hit a box).
  bool ResolveAiTrackingClickLock(
    float click_x, float click_y,
    float * out_x, float * out_y, float * out_w, float * out_h,
    int * out_track_id,
    std::string * out_label) const;

  /// If locked_track_id_ is set, refresh lock bbox from latest detections.
  void UpdateLockFromTrackedId();

  /// Publish /ah/system/status immediately (lock changes should not wait for the video timer).
  void PublishStatusSnapshot(const char * video_status);

  rclcpp::Publisher<std_msgs::msg::String>::SharedPtr status_pub_;
  rclcpp::Publisher<sensor_msgs::msg::CompressedImage>::SharedPtr video_pub_;
  rclcpp::Service<ah_msgs::srv::StartTracking>::SharedPtr start_tracking_srv_;
  rclcpp::Service<std_srvs::srv::Trigger>::SharedPtr stop_tracking_srv_;
  rclcpp::Service<std_srvs::srv::Trigger>::SharedPtr cancel_tracking_srv_;
  rclcpp::Service<ah_msgs::srv::ListCameras>::SharedPtr list_cameras_srv_;
  rclcpp::Service<ah_msgs::srv::SelectCamera>::SharedPtr select_camera_srv_;
  rclcpp::Service<std_srvs::srv::SetBool>::SharedPtr ai_tracking_toggle_srv_;
  rclcpp::Service<ah_msgs::srv::AiTrackingClick>::SharedPtr ai_tracking_click_srv_;
  rclcpp::Subscription<std_msgs::msg::String>::SharedPtr detections_sub_;
  rclcpp::CallbackGroup::SharedPtr camera_cb_group_;
  rclcpp::TimerBase::SharedPtr timer_;
  rclcpp::TimerBase::SharedPtr probe_timer_;
  int frame_id_{0};

  bool tracking_started_{false};
  bool segmentation_active_{false};
  bool ai_tracking_active_{false};
  bool following_active_{false};
  float tracking_bounding_box_x_{0.0f};
  float tracking_bounding_box_y_{0.0f};
  float tracking_bounding_box_width_{0.0f};
  float tracking_bounding_box_height_{0.0f};
  std::string tracker_type_{"stub"};

  CameraSelection camera_;
  std::string settings_path_;
  std::mutex camera_mutex_;
  std::vector<CameraDevice> camera_cache_;
  bool camera_probe_done_{false};
  CameraCapture capture_;
  /// Last video_status string for status JSON (connected / degraded / unavailable).
  std::string last_video_status_{"connected"};
  int capture_open_fail_log_count_{0};

  mutable std::mutex detections_mutex_;
  std::vector<DetectionBox> last_detections_;
  /// When tracking under AI mode: follow this Ultralytics track_id frame-to-frame.
  int locked_track_id_{-1};
  int locked_track_miss_frames_{0};
};

}  // namespace ah_core
