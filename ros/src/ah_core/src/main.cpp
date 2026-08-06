// ah_core_node entry point — load settings, spin AhCoreNode.

#include "ah_core/ah_core_node.hpp"
#include "ah_core/ros_runtime_settings.hpp"

#include <memory>

#include "rclcpp/rclcpp.hpp"

int main(int argument_count, char ** argument_values)
{
  const auto [ros_namespace, settings_path, camera] = ah_core::LoadRosRuntimeSettings();
  rclcpp::init(argument_count, argument_values);

  // Multithreaded so camera probe / list refresh cannot starve status+video
  // (camera callbacks use a dedicated callback group on AhCoreNode)
  rclcpp::executors::MultiThreadedExecutor executor;
  const auto Node = std::make_shared<ah_core::AhCoreNode>(ros_namespace, camera, settings_path);
  executor.add_node(Node);
  executor.spin();

  rclcpp::shutdown();
  return 0;
}
