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
  using ExecutorStoppedCallback = std::function<void()>;
  using StatusJsonCallback = std::function<void(const std::string& json)>;
  using VideoJpegCallback = std::function<void(std::vector<uint8_t> jpeg)>;
  using DetectionsJsonCallback = std::function<void(const std::string& json)>;

  struct Hooks {
    ExecutorStoppedCallback on_executor_stopped;
    StatusJsonCallback on_status_json;
    VideoJpegCallback on_video_jpeg;
    DetectionsJsonCallback on_detections_json;
  };

  static constexpr std::uint8_t DefaultRosDomainId = 42;
  static constexpr std::uint8_t MaxRosDomainId = 232;

  /// @param ros_domain_id DDS domain for this process.
  /// @param ros_namespace Node namespace (empty = root). Topics use relative
  ///        ah/... names so they become /{namespace}/ah/... when set.
  /// @param hooks Optional status/video/stop callbacks.
  explicit AhRosBridge(std::uint8_t ros_domain_id, std::string ros_namespace = {},
                       Hooks hooks = {});
  ~AhRosBridge();

  AhRosBridge(const AhRosBridge&) = delete;
  AhRosBridge& operator=(const AhRosBridge&) = delete;

  [[nodiscard]] rclcpp::Node::SharedPtr Node() const { return node_; }
  [[nodiscard]] std::uint8_t RosDomainId() const { return ros_domain_id_; }
  [[nodiscard]] const std::string& RosNamespace() const { return ros_namespace_; }

 private:
  [[nodiscard]] static std::uint8_t SanitizeDomainId(std::uint8_t ros_domain_id);
  [[nodiscard]] static std::string SanitizeNamespace(std::string ros_namespace);
  void SetupSubscriptions();

  std::uint8_t ros_domain_id_;
  std::string ros_namespace_;
  Hooks hooks_;
  rclcpp::Node::SharedPtr node_;
  rclcpp::Subscription<std_msgs::msg::String>::SharedPtr status_sub_;
  rclcpp::Subscription<sensor_msgs::msg::CompressedImage>::SharedPtr video_sub_;
  rclcpp::Subscription<std_msgs::msg::String>::SharedPtr detections_sub_;
  std::shared_ptr<rclcpp::executors::SingleThreadedExecutor> executor_;
  std::thread spin_thread_;
};
