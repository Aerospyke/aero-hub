#include "ah_ros_bridge.h"

AhRosBridge::AhRosBridge(int argc, char** argv)
{
  if (!rclcpp::ok()) {
    rclcpp::init(argc, argv);
  }

  // Graph name matches interface map §2 (ah_dashboard).
  node_ = std::make_shared<rclcpp::Node>("ah_dashboard");

  executor_ = std::make_shared<rclcpp::executors::SingleThreadedExecutor>();
  executor_->add_node(node_);

  spin_thread_ = std::thread([this]() {
    RCLCPP_INFO(
      node_->get_logger(),
      "ah_dashboard online — executor spinning (Task_15)");
    executor_->spin();
    RCLCPP_INFO(node_->get_logger(), "ah_dashboard executor stopped");
  });
}

AhRosBridge::~AhRosBridge()
{
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
