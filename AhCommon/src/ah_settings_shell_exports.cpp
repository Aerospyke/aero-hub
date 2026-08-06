// Prints shell exports derived from AhCommon settings (for init_ah_ros_in_terminal.sh).
// Settings file is the source of truth; this bridges values that ros2 CLI / rmw read
// from the process environment.

#include "ah_common/settings.hpp"

#include <iostream>

int main() {
  const ah::Settings settings = ah::Settings::Load();
  // Load() already published env in this process; re-print for parent `eval`.
  std::cout << "export ROS_DOMAIN_ID=" << static_cast<unsigned>(settings.Ros().DomainId()) << '\n';
  std::cout << "export RMW_IMPLEMENTATION=" << settings.Ros().RmwImplementation() << '\n';
  const std::string YoloModelsDir = settings.Ros().YoloModelsDir();
  if (!YoloModelsDir.empty()) {
    std::cout << "export AERO_HUB_YOLO_MODELS=" << YoloModelsDir << '\n';
  }
  // Namespace is node-level only — not exported for the shell.
  return 0;
}
