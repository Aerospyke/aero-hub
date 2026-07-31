// ah_core_node entry point — load settings, spin AhCoreNode.

#include "ah_core/ah_core_node.hpp"
#include "ah_core/ros_runtime_settings.hpp"

#include <memory>

#include "rclcpp/rclcpp.hpp"

int main(int argc, char ** argv)
{
  const ah_core::RosRuntimeSettings runtime = ah_core::LoadRosRuntimeSettings();
  rclcpp::init(argc, argv);

  // Multi-threaded so camera probe / list refresh cannot starve status+video
  // (camera callbacks use a dedicated callback group on AhCoreNode).
  rclcpp::executors::MultiThreadedExecutor executor;
  auto node = std::make_shared<ah_core::AhCoreNode>(
    runtime.ros_namespace, runtime.camera, runtime.settings_path);
  executor.add_node(node);
  executor.spin();

  rclcpp::shutdown();
  return 0;
}
