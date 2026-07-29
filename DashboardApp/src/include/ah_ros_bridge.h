#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "rclcpp/rclcpp.hpp"
#include "sensor_msgs/msg/compressed_image.hpp"
#include "std_msgs/msg/string.hpp"

/// Hosts the ah_dashboard rclcpp node and spins it on a background thread so
/// the UI event loop stays free. Does not depend on Qt.
class AhRosBridge {
 public:
  /// Invoked on the executor thread after spin() returns (e.g. SIGINT/SIGTERM).
  using ExecutorStoppedCallback = std::function<void()>;

  /// Invoked on the executor thread for each `/ah/system/status` message (JSON string).
  /// Marshal into the UI thread before touching Qt objects.
  using StatusJsonCallback = std::function<void(const std::string& json)>;

  /// Invoked on the executor thread for each `/ah/video/compressed` JPEG payload.
  /// Marshal into the UI thread before decoding / touching Qt objects.
  using VideoJpegCallback = std::function<void(std::vector<uint8_t> jpeg)>;

  /// Optional hooks; all may be empty.
  struct Hooks {
    ExecutorStoppedCallback on_executor_stopped;
    StatusJsonCallback on_status_json;
    VideoJpegCallback on_video_jpeg;
  };

  /// DDS domain id valid range is [0, MaxRosDomainId]. Out-of-range values log a
  /// warning and fall back to DefaultRosDomainId; the object remains usable.
  static constexpr std::uint8_t DefaultRosDomainId = 42;
  static constexpr std::uint8_t MaxRosDomainId = 232;

  explicit AhRosBridge(std::uint8_t ros_domain_id, Hooks hooks = {});
  ~AhRosBridge();

  AhRosBridge(const AhRosBridge&) = delete;
  AhRosBridge& operator=(const AhRosBridge&) = delete;

  [[nodiscard]] rclcpp::Node::SharedPtr Node() const { return node_; }
  [[nodiscard]] std::uint8_t RosDomainId() const { return ros_domain_id_; }

 private:
  [[nodiscard]] static std::uint8_t SanitizeDomainId(std::uint8_t ros_domain_id);
  void SetupSubscriptions();

  std::uint8_t ros_domain_id_;
  Hooks hooks_;
  rclcpp::Node::SharedPtr node_;
  rclcpp::Subscription<std_msgs::msg::String>::SharedPtr status_sub_;
  rclcpp::Subscription<sensor_msgs::msg::CompressedImage>::SharedPtr video_sub_;
  std::shared_ptr<rclcpp::executors::SingleThreadedExecutor> executor_;
  std::thread spin_thread_;
};
