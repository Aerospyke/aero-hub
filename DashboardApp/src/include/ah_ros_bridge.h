#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <thread>

#include "rclcpp/rclcpp.hpp"
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

  /// DDS domain id valid range is [0, MaxRosDomainId]. Out-of-range values log a
  /// warning and fall back to DefaultRosDomainId; the object remains usable.
  static constexpr std::uint8_t DefaultRosDomainId = 42;
  static constexpr std::uint8_t MaxRosDomainId = 232;

  /// @param ros_domain_id DDS domain for this process.
  /// @param on_executor_stopped Optional hook when the background spin ends.
  /// @param on_status_json Optional hook for `/ah/system/status` payloads.
  explicit AhRosBridge(std::uint8_t ros_domain_id, ExecutorStoppedCallback on_executor_stopped = {},
                       StatusJsonCallback on_status_json = {});
  ~AhRosBridge();

  AhRosBridge(const AhRosBridge&) = delete;
  AhRosBridge& operator=(const AhRosBridge&) = delete;

  [[nodiscard]] rclcpp::Node::SharedPtr Node() const { return node_; }
  [[nodiscard]] std::uint8_t RosDomainId() const { return ros_domain_id_; }

 private:
  [[nodiscard]] static std::uint8_t SanitizeDomainId(std::uint8_t ros_domain_id);
  void SetupStatusSubscription();

  std::uint8_t ros_domain_id_;
  ExecutorStoppedCallback on_executor_stopped_;
  StatusJsonCallback on_status_json_;
  rclcpp::Node::SharedPtr node_;
  rclcpp::Subscription<std_msgs::msg::String>::SharedPtr status_sub_;
  std::shared_ptr<rclcpp::executors::SingleThreadedExecutor> executor_;
  std::thread spin_thread_;
};
