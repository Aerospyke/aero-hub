// ah_core_node — Milestone_1 stub
// - /ah/system/status       : JSON in std_msgs/String  (ros2-interface-map §3.1)
// - /ah/video/compressed    : synthetic JPEG frames    (ros2-interface-map §5)
// - /ah/tracking/start|stop|cancel : track services    (ros2-interface-map §4)

#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

#include "ah_msgs/srv/start_tracking.hpp"
#include "rclcpp/rclcpp.hpp"
#include "sensor_msgs/msg/compressed_image.hpp"
#include "std_msgs/msg/string.hpp"
#include "std_srvs/srv/trigger.hpp"

using namespace std::chrono_literals;

namespace
{
constexpr int kFrameWidth = 640;
constexpr int kFrameHeight = 480;
constexpr int kJpegQuality = 80;
constexpr int kPublishHz = 10;

std::string CreateStatusJson(
  double stamp_sec,
  const char * video_status,
  bool tracking_started,
  bool segmentation_active,
  bool smart_mode_active,
  bool following_active)
{
  std::ostringstream oss;
  oss.setf(std::ios::fixed);
  oss.precision(3);
  oss << '{'
      << "\"smart_mode_active\":" << (smart_mode_active ? "true" : "false") << ','
      << "\"tracking_started\":" << (tracking_started ? "true" : "false") << ','
      << "\"segmentation_active\":" << (segmentation_active ? "true" : "false") << ','
      << "\"following_active\":" << (following_active ? "true" : "false") << ','
      << "\"video_status\":\"" << video_status << "\","
      << "\"tracker_type\":\"stub\","
      << "\"follower_mode\":\"none\","
      << "\"stamp\":" << stamp_sec
      << '}';
  return oss.str();
}

// Simple synthetic pattern: dark background, color bar strip, bouncing box, frame id.
cv::Mat CreateSyntheticImage(const int frame_id, bool tracking_started)
{
  cv::Mat frame(kFrameHeight, kFrameWidth, CV_8UC3, cv::Scalar(20, 24, 32));

  // Horizontal color bands
  const int band_h = 40;
  for (int i = 0; i < 6; ++i) {
    const cv::Scalar colors[] = {
      {0, 0, 200}, {0, 200, 200}, {0, 200, 0},
      {200, 200, 0}, {200, 0, 0}, {200, 0, 200},
    };
    cv::rectangle(
      frame,
      cv::Rect(0, i * band_h, kFrameWidth, band_h),
      colors[i],
      cv::FILLED);
  }

  // Bouncing box
  const int box = 48;
  const int span_x = kFrameWidth - box;
  const int span_y = kFrameHeight - box - 6 * band_h;
  const int x = (frame_id * 3) % (span_x > 0 ? span_x : 1);
  const int y = 6 * band_h + ((frame_id * 2) % (span_y > 0 ? span_y : 1));
  cv::rectangle(frame, cv::Rect(x, y, box, box), cv::Scalar(255, 255, 255), cv::FILLED);
  cv::rectangle(frame, cv::Rect(x, y, box, box), cv::Scalar(0, 140, 255), 2);

  // Overlay text
  cv::putText(
    frame,
    tracking_started ? "AeroHub ah_core TRACKING" : "AeroHub ah_core synthetic",
    cv::Point(12, kFrameHeight - 36),
    cv::FONT_HERSHEY_SIMPLEX,
    0.7,
    tracking_started ? cv::Scalar(80, 255, 120) : cv::Scalar(230, 230, 230),
    2);
  cv::putText(
    frame,
    "frame " + std::to_string(frame_id),
    cv::Point(12, kFrameHeight - 12),
    cv::FONT_HERSHEY_SIMPLEX,
    0.6,
    cv::Scalar(180, 200, 220),
    1);

  return frame;
}

bool IsNormalizedBbox(float x, float y, float width, float height)
{
  return x >= 0.0f && y >= 0.0f && width > 0.0f && height > 0.0f &&
         x <= 1.0f && y <= 1.0f && width <= 1.0f && height <= 1.0f &&
         (x + width) <= 1.0001f && (y + height) <= 1.0001f;
}
}  // namespace

class AhCoreNode : public rclcpp::Node
{
public:
  AhCoreNode()
  : Node("ah_core")
  {
    rclcpp::QoS status_qos(rclcpp::KeepLast(1));
    status_qos.reliable();

    // Video: latest-frame only, best effort (interface map §5).
    rclcpp::QoS video_qos(rclcpp::KeepLast(1));
    video_qos.best_effort();

    status_pub_ = create_publisher<std_msgs::msg::String>("/ah/system/status", status_qos);
    video_pub_ =
      create_publisher<sensor_msgs::msg::CompressedImage>("/ah/video/compressed", video_qos);

    start_tracking_srv_ = create_service<ah_msgs::srv::StartTracking>(
      "/ah/tracking/start",
      std::bind(&AhCoreNode::OnStartTracking, this, std::placeholders::_1, std::placeholders::_2));

    stop_tracking_srv_ = create_service<std_srvs::srv::Trigger>(
      "/ah/tracking/stop",
      std::bind(&AhCoreNode::OnStopTracking, this, std::placeholders::_1, std::placeholders::_2));

    cancel_tracking_srv_ = create_service<std_srvs::srv::Trigger>(
      "/ah/tracking/cancel",
      std::bind(&AhCoreNode::OnCancelTracking, this, std::placeholders::_1, std::placeholders::_2));

    const auto period = std::chrono::milliseconds(1000 / kPublishHz);
    timer_ = create_wall_timer(period, std::bind(&AhCoreNode::OnTimer, this));

    RCLCPP_INFO(
      get_logger(),
      "ah_core stub: status + video @ %d Hz; services /ah/tracking/{start,stop,cancel}",
      kPublishHz);
  }

private:
  void OnTimer()
  {
    const auto now = get_clock()->now();
    ++frame_id_;

    // --- status ---
    std_msgs::msg::String status_msg;
    status_msg.data = CreateStatusJson(
      now.seconds(),
      "connected",
      tracking_started_,
      segmentation_active_,
      smart_mode_active_,
      following_active_);
    status_pub_->publish(status_msg);

    // --- synthetic compressed video ---
    cv::Mat bgr = CreateSyntheticImage(frame_id_, tracking_started_);
    std::vector<int> params = {cv::IMWRITE_JPEG_QUALITY, kJpegQuality};
    std::vector<uint8_t> jpeg;
    if (!cv::imencode(".jpg", bgr, jpeg, params)) {
      RCLCPP_WARN_THROTTLE(get_logger(), *get_clock(), 2000, "JPEG encode failed");
      return;
    }

    sensor_msgs::msg::CompressedImage video_msg;
    video_msg.header.stamp = now;
    video_msg.header.frame_id = "ah_camera_stub";
    video_msg.format = "jpeg";
    video_msg.data = std::move(jpeg);
    video_pub_->publish(video_msg);
  }

  void OnStartTracking(
    const std::shared_ptr<ah_msgs::srv::StartTracking::Request> request,
    std::shared_ptr<ah_msgs::srv::StartTracking::Response> response)
  {
    if (!IsNormalizedBbox(request->x, request->y, request->width, request->height)) {
      response->success = false;
      response->message =
        "bbox must be normalized in [0,1] with positive width/height (interface map §4)";
      RCLCPP_WARN(
        get_logger(),
        "start rejected: x=%.3f y=%.3f w=%.3f h=%.3f",
        request->x, request->y, request->width, request->height);
      return;
    }

    tracking_started_ = true;
    track_x_ = request->x;
    track_y_ = request->y;
    track_w_ = request->width;
    track_h_ = request->height;

    response->success = true;
    response->message = "tracking started (stub)";
    RCLCPP_INFO(
      get_logger(),
      "tracking started (stub) bbox norm x=%.3f y=%.3f w=%.3f h=%.3f",
      track_x_, track_y_, track_w_, track_h_);
  }

  void OnStopTracking(
    const std::shared_ptr<std_srvs::srv::Trigger::Request> /*request*/,
    std::shared_ptr<std_srvs::srv::Trigger::Response> response)
  {
    // Stop: clear tracking only (interface map §4 stop/cancel semantics).
    const bool was = tracking_started_;
    tracking_started_ = false;
    response->success = true;
    response->message = was ? "tracking stopped" : "tracking already idle";
    RCLCPP_INFO(get_logger(), "%s", response->message.c_str());
  }

  void OnCancelTracking(
    const std::shared_ptr<std_srvs::srv::Trigger::Request> /*request*/,
    std::shared_ptr<std_srvs::srv::Trigger::Response> response)
  {
    // Cancel: broader hard reset (tracking + related selection state).
    tracking_started_ = false;
    segmentation_active_ = false;
    response->success = true;
    response->message = "tracking cancelled (stub hard reset)";
    RCLCPP_INFO(get_logger(), "%s", response->message.c_str());
  }

  rclcpp::Publisher<std_msgs::msg::String>::SharedPtr status_pub_;
  rclcpp::Publisher<sensor_msgs::msg::CompressedImage>::SharedPtr video_pub_;
  rclcpp::Service<ah_msgs::srv::StartTracking>::SharedPtr start_tracking_srv_;
  rclcpp::Service<std_srvs::srv::Trigger>::SharedPtr stop_tracking_srv_;
  rclcpp::Service<std_srvs::srv::Trigger>::SharedPtr cancel_tracking_srv_;
  rclcpp::TimerBase::SharedPtr timer_;
  int frame_id_{0};

  bool tracking_started_{false};
  bool segmentation_active_{false};
  bool smart_mode_active_{false};
  bool following_active_{false};
  float track_x_{0.0f};
  float track_y_{0.0f};
  float track_w_{0.0f};
  float track_h_{0.0f};
};

namespace
{
// Same Qt-style INI as aero-hub/aerohub_settings.ini (see aerohub_settings_template.ini):
// [ROS] domain_id=N. Env ROS_DOMAIN_ID wins if already set (Docker --env). Else read file.
void ApplyRosDomainFromSettings()
{
  if (const char * existing = std::getenv("ROS_DOMAIN_ID");
    existing != nullptr && existing[0] != '\0')
  {
    return;
  }

  const char * candidates[] = {
    std::getenv("AERO_HUB_SETTINGS"),
    "aerohub_settings.ini",
    "../aerohub_settings.ini",
    "../../aerohub_settings.ini",
    "/aero-hub/aerohub_settings.ini",  // optional full-repo mount
  };

  for (const char * path : candidates) {
    if (path == nullptr || path[0] == '\0') {
      continue;
    }
    std::ifstream in(path);
    if (!in) {
      continue;
    }
    std::string line;
    bool in_ros = false;
    while (std::getline(in, line)) {
      if (!line.empty() && line.back() == '\r') {
        line.pop_back();
      }
      if (line.empty() || line[0] == ';' || line[0] == '#') {
        continue;
      }
      if (line.front() == '[') {
        in_ros = (line == "[ROS]");
        continue;
      }
      if (!in_ros) {
        continue;
      }
      const auto eq = line.find('=');
      if (eq == std::string::npos) {
        continue;
      }
      const std::string key = line.substr(0, eq);
      std::string val = line.substr(eq + 1);
      if (key == "domain_id" && !val.empty()) {
        setenv("ROS_DOMAIN_ID", val.c_str(), 1);
        return;
      }
    }
  }
}
}  // namespace

int main(int argc, char ** argv)
{
  ApplyRosDomainFromSettings();
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<AhCoreNode>());
  rclcpp::shutdown();
  return 0;
}
