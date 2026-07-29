#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <thread>

#include "rclcpp/rclcpp.hpp"

/// Hosts the ah_dashboard rclcpp node and spins it on a background thread so
/// the Qt event loop stays free. Task_15: appear on the ROS graph.
/// Task_16+: subscribe /ah/system/status and marshal into Qt/QML.
class AhRosBridge {
 public:
  /// Invoked on the executor thread after spin() returns (e.g. SIGINT/SIGTERM).
  using ExecutorStoppedCallback = std::function<void()>;

  /// DDS domain id valid range is [0, MaxRosDomainId]. Out-of-range values log a
  /// warning and fall back to DefaultRosDomainId; the object remains usable.
  static constexpr std::uint8_t DefaultRosDomainId = 42;
  static constexpr std::uint8_t MaxRosDomainId = 232;

  /// @param ros_domain_id DDS domain for this process.
  /// @param on_executor_stopped Optional hook when the background spin ends
  ///        (does not know about Qt; the app decides what to do).
  explicit AhRosBridge(std::uint8_t ros_domain_id,
                       ExecutorStoppedCallback on_executor_stopped = {});
  ~AhRosBridge();

  AhRosBridge(const AhRosBridge&) = delete;
  AhRosBridge& operator=(const AhRosBridge&) = delete;

  [[nodiscard]] rclcpp::Node::SharedPtr Node() const { return node_; }
  [[nodiscard]] std::uint8_t RosDomainId() const { return ros_domain_id_; }

 private:
  [[nodiscard]] static std::uint8_t SanitizeDomainId(std::uint8_t ros_domain_id);

  std::uint8_t ros_domain_id_;
  ExecutorStoppedCallback on_executor_stopped_;
  rclcpp::Node::SharedPtr node_;
  std::shared_ptr<rclcpp::executors::SingleThreadedExecutor> executor_;
  std::thread spin_thread_;
};
