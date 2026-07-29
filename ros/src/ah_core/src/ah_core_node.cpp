// ah_core_node — Milestone_1 stub
// - /ah/system/status  : JSON in std_msgs/String  (ros2-interface-map §3.1)
// - /ah/video/compressed : synthetic JPEG frames   (ros2-interface-map §5)

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

#include "rclcpp/rclcpp.hpp"
#include "sensor_msgs/msg/compressed_image.hpp"
#include "std_msgs/msg/string.hpp"

using namespace std::chrono_literals;

namespace
{
constexpr int kFrameWidth = 640;
constexpr int kFrameHeight = 480;
constexpr int kJpegQuality = 80;
constexpr int kPublishHz = 10;

std::string CreateStatusJson(double stamp_sec, const char * video_status)
{
  std::ostringstream oss;
  oss.setf(std::ios::fixed);
  oss.precision(3);
  oss << '{'
      << "\"smart_mode_active\":false,"
      << "\"tracking_started\":false,"
      << "\"segmentation_active\":false,"
      << "\"following_active\":false,"
      << "\"video_status\":\"" << video_status << "\","
      << "\"tracker_type\":\"stub\","
      << "\"follower_mode\":\"none\","
      << "\"stamp\":" << stamp_sec
      << '}';
  return oss.str();
}

// Simple synthetic pattern: dark background, color bar strip, bouncing box, frame id.
cv::Mat CreateSyntheticImage(const int frame_id)
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
    "AeroHub ah_core synthetic",
    cv::Point(12, kFrameHeight - 36),
    cv::FONT_HERSHEY_SIMPLEX,
    0.7,
    cv::Scalar(230, 230, 230),
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

    const auto period = std::chrono::milliseconds(1000 / kPublishHz);
    timer_ = create_wall_timer(period, std::bind(&AhCoreNode::on_timer, this));

    RCLCPP_INFO(
      get_logger(),
      "ah_core stub: /ah/system/status + /ah/video/compressed (synthetic JPEG) at %d Hz",
      kPublishHz);
  }

private:
  void on_timer()
  {
    const auto now = get_clock()->now();
    ++frame_id_;

    // --- status ---
    std_msgs::msg::String status_msg;
    status_msg.data = CreateStatusJson(now.seconds(), "connected");
    status_pub_->publish(status_msg);

    // --- synthetic compressed video ---
    cv::Mat bgr = CreateSyntheticImage(frame_id_);
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

  rclcpp::Publisher<std_msgs::msg::String>::SharedPtr status_pub_;
  rclcpp::Publisher<sensor_msgs::msg::CompressedImage>::SharedPtr video_pub_;
  rclcpp::TimerBase::SharedPtr timer_;
  int frame_id_{0};
};

namespace
{
// Same Qt-style INI as aero-hub/aerohub_settings.ini (see aerohub_settings_template.ini):
// [ROS] domain_id=N. Env ROS_DOMAIN_ID wins if already set (Docker --env). Else read file.
void apply_ros_domain_from_settings()
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
  apply_ros_domain_from_settings();
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<AhCoreNode>());
  rclcpp::shutdown();
  return 0;
}
