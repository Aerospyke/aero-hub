#pragma once

#include <memory>
#include <thread>

#include "rclcpp/rclcpp.hpp"

/// Hosts the `ah_dashboard` rclcpp node and spins it on a background thread so
/// the Qt event loop stays free. Task_15: appear on the ROS graph.
/// Task_16+: subscribe /ah/system/status and marshal into Qt/QML.
class AhRosBridge {
 public:
  AhRosBridge(int argc, char** argv);
  ~AhRosBridge();

  AhRosBridge(const AhRosBridge&) = delete;
  AhRosBridge& operator=(const AhRosBridge&) = delete;

  [[nodiscard]] rclcpp::Node::SharedPtr node() const { return node_; }

 private:
  rclcpp::Node::SharedPtr node_;
  std::shared_ptr<rclcpp::executors::SingleThreadedExecutor> executor_;
  std::thread spin_thread_;
};
