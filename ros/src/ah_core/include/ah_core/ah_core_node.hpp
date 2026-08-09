#pragma once

// AhCoreNode — AeroHub core ROS node orchestration
// (status, video, track, camera list/select, AI lock).
// JSON → messages.hpp; frames → video_pipeline.hpp; tracking → tracking_controller.hpp.

#include "ah_core/camera_devices.hpp"
#include "ah_core/tracking_controller.hpp"

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
  explicit AhCoreNode(
    const std::string & ros_namespace,
    CameraSelection initial_camera,
    std::string settings_path);

  ~AhCoreNode() override;

private:
  void ApplySelectionToParams();
  void PersistSelectionToSettingsFile() const;
  /// Open or close long-lived capture to match camera_ (holds camera_mutex_).
  bool SyncCaptureToSelection(std::string * error_out = nullptr);
  void OnTimer();
  void OnStartTracking(
    std::shared_ptr<ah_msgs::srv::StartTracking::Request> request,
    std::shared_ptr<ah_msgs::srv::StartTracking::Response> response);
  void OnStopTracking(
    std::shared_ptr<std_srvs::srv::Trigger::Request> request,
    std::shared_ptr<std_srvs::srv::Trigger::Response> response);
  void OnCancelTracking(
    std::shared_ptr<std_srvs::srv::Trigger::Request> request,
    std::shared_ptr<std_srvs::srv::Trigger::Response> response);
  void OnListCameras(
    std::shared_ptr<ah_msgs::srv::ListCameras::Request> request,
    std::shared_ptr<ah_msgs::srv::ListCameras::Response> response);
  void OnSelectCamera(
    std::shared_ptr<ah_msgs::srv::SelectCamera::Request> request,
    std::shared_ptr<ah_msgs::srv::SelectCamera::Response> response);
  void FillSelectResponse(
    std::shared_ptr<ah_msgs::srv::SelectCamera::Response> response) const;
  void OnAiTrackingToggle(
    std::shared_ptr<std_srvs::srv::SetBool::Request> request,
    std::shared_ptr<std_srvs::srv::SetBool::Response> response);
  void OnAiTrackingClick(
    std::shared_ptr<ah_msgs::srv::AiTrackingClick::Request> request,
    std::shared_ptr<ah_msgs::srv::AiTrackingClick::Response> response);
  void OnDetections(std_msgs::msg::String::SharedPtr msg);

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

  TrackingController tracking_;

  CameraSelection camera_;
  std::string settings_path_;
  std::mutex camera_mutex_;
  std::vector<CameraDevice> camera_cache_;
  bool camera_probe_done_{false};
  CameraCapture camera_capture_;
  /// Last video_status string for status JSON (connected / degraded / unavailable).
  std::string last_video_status_{"connected"};
  int capture_open_fail_log_count_{0};
};

}  // namespace ah_core
