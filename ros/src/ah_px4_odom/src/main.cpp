// ah_px4_odom_node — PX4 vehicle_odometry to /<ns>/odom and odom -> base_link.
//
//   export ROS_DOMAIN_ID=42
//   ros2 run ah_px4_odom ah_px4_odom_node --ros-args \
//     -p vehicle_namespace:=rover_sim -p use_sim_time:=true

#include "ah_px4_odom/ah_px4_odom_node.hpp"

#include <memory>

#include "rclcpp/rclcpp.hpp"

int main(int argument_count, char** argument_values) {
  rclcpp::init(argument_count, argument_values);
  rclcpp::spin(std::make_shared<ah_px4_odom::AhPx4OdomNode>());
  rclcpp::shutdown();
  return 0;
}
