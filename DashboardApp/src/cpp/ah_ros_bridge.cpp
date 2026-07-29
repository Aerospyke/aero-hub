#include "ah_ros_bridge.h"

#include <iostream>
#include <utility>

#include <rclcpp/init_options.hpp>

AhRosBridge::AhRosBridge(std::uint8_t ros_domain_id, ExecutorStoppedCallback on_executor_stopped)
    : ros_domain_id_(SanitizeDomainId(ros_domain_id)),
      on_executor_stopped_(std::move(on_executor_stopped)) {
  if (!rclcpp::ok()) {
    rclcpp::InitOptions init_options;
    init_options.set_domain_id(static_cast<size_t>(ros_domain_id_));
    rclcpp::init(0, nullptr, init_options);
  }

  // Graph name matches interface map §2 (ah_dashboard).
  node_ = std::make_shared<rclcpp::Node>("ah_dashboard");

  executor_ = std::make_shared<rclcpp::executors::SingleThreadedExecutor>();
  executor_->add_node(node_);

  spin_thread_ = std::thread([this]() {
    RCLCPP_INFO(node_->get_logger(), "ah_dashboard online — executor spinning (Task_15), ROS_DOMAIN_ID=%u",
                static_cast<unsigned>(ros_domain_id_));
    executor_->spin();
    RCLCPP_INFO(node_->get_logger(), "ah_dashboard executor stopped");

    if (on_executor_stopped_) {
      on_executor_stopped_();
    }
  });
}

AhRosBridge::~AhRosBridge() {
  if (executor_) {
    executor_->cancel();
  }
  if (spin_thread_.joinable()) {
    spin_thread_.join();
  }
  if (executor_ && node_) {
    executor_->remove_node(node_);
  }
  node_.reset();
  executor_.reset();

  if (rclcpp::ok()) {
    rclcpp::shutdown();
  }
}

std::uint8_t AhRosBridge::SanitizeDomainId(std::uint8_t ros_domain_id) {
  if (ros_domain_id > MaxRosDomainId) {
    // Before rclcpp::init — log without the ROS logging system.
    std::cerr << "AhRosBridge: invalid ROS domain id " << static_cast<int>(ros_domain_id)
              << " (valid range 0–" << static_cast<int>(MaxRosDomainId) << "); using default "
              << static_cast<int>(DefaultRosDomainId) << '\n';
    return DefaultRosDomainId;
  }
  return ros_domain_id;
}
