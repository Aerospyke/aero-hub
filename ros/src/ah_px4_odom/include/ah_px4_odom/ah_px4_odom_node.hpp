#pragma once

#include <memory>
#include <string>

#include "rclcpp/rclcpp.hpp"
#include "tf2_ros/transform_broadcaster.hpp"

#include "px4_msgs/msg/vehicle_odometry.hpp"
#include "nav_msgs/msg/odometry.hpp"

namespace ah_px4_odom {

class AhPx4OdomNode : public rclcpp::Node {
 public:
  explicit AhPx4OdomNode(const rclcpp::NodeOptions& options = rclcpp::NodeOptions());

 private:
  void OnOdometry(const px4_msgs::msg::VehicleOdometry::SharedPtr msg);

  std::string vehicle_namespace_;
  bool warned_velocity_frame_ = false;
  bool warned_pose_frame_ = false;
  bool logged_first_ = false;
  rclcpp::Subscription<px4_msgs::msg::VehicleOdometry>::SharedPtr odom_sub_;
  rclcpp::Publisher<nav_msgs::msg::Odometry>::SharedPtr odom_pub_;
  std::unique_ptr<tf2_ros::TransformBroadcaster> tf_broadcaster_;
};

}  // namespace ah_px4_odom
