#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "ah_px4_sample.h"

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
  using Px4OdometryCallback = std::function<void(AhPx4OdomSample sample)>;
  using Px4StatusCallback = std::function<void(AhPx4StatusSample sample)>;
  using Px4GpsCallback = std::function<void(AhPx4GpsSample sample)>;

  struct Hooks {
    ExecutorStoppedCallback on_executor_stopped;
    StatusJsonCallback on_status_json;
    VideoJpegCallback on_video_jpeg;
    DetectionsJsonCallback on_detections_json;
    Px4OdometryCallback on_px4_odometry;
    Px4StatusCallback on_px4_status;
    Px4GpsCallback on_px4_gps;
  };

  static constexpr std::uint8_t DefaultRosDomainId = 42;
  static constexpr std::uint8_t MaxRosDomainId = 232;

  /// @param ros_domain_id DDS domain for this process.
  /// @param ros_namespace Node namespace (empty = root). Topics use relative
  ///        ah/... names so they become /{namespace}/ah/... when set.
  /// @param vehicle_namespace PX4 graph namespace (default "rover"). A second
  ///        node uses it so relative fmu/out/... names resolve under it.
  /// @param hooks Optional status/video/PX4/stop callbacks.
  explicit AhRosBridge(std::uint8_t ros_domain_id, std::string ros_namespace = {},
                       std::string vehicle_namespace = "rover", Hooks hooks = {});
  ~AhRosBridge();

  AhRosBridge(const AhRosBridge&) = delete;
  AhRosBridge& operator=(const AhRosBridge&) = delete;

  [[nodiscard]] rclcpp::Node::SharedPtr Node() const { return node_; }

  [[nodiscard]] std::uint8_t RosDomainId() const { return ros_domain_id_; }

  [[nodiscard]] const std::string& RosNamespace() const { return ros_namespace_; }

  [[nodiscard]] const std::string& VehicleNamespace() const { return vehicle_namespace_; }

 private:
  [[nodiscard]] static std::uint8_t SanitizeDomainId(std::uint8_t ros_domain_id);
  void SetupSubscriptions();
  void SetupPx4Subscriptions();

  std::uint8_t ros_domain_id_;
  std::string ros_namespace_;
  std::string vehicle_namespace_;
  Hooks hooks_;
  rclcpp::Node::SharedPtr node_;
  rclcpp::Node::SharedPtr px4_node_;
  bool px4_node_added_ = false;
  rclcpp::Subscription<std_msgs::msg::String>::SharedPtr status_sub_;
  rclcpp::Subscription<sensor_msgs::msg::CompressedImage>::SharedPtr video_sub_;
  rclcpp::Subscription<std_msgs::msg::String>::SharedPtr detections_sub_;
  // Type-erased so this header stays free of px4_msgs. Real types live in the cpp.
  rclcpp::SubscriptionBase::SharedPtr px4_odom_sub_;
  rclcpp::SubscriptionBase::SharedPtr px4_status_sub_;
  rclcpp::SubscriptionBase::SharedPtr px4_gps_sub_;
  bool logged_px4_odom_ = false;
  bool logged_px4_status_ = false;
  bool logged_px4_gps_ = false;
  std::shared_ptr<rclcpp::executors::SingleThreadedExecutor> executor_;
  std::thread spin_thread_;
};
