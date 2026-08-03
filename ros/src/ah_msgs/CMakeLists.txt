cmake_minimum_required(VERSION 3.16)
project(ah_msgs)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake REQUIRED)
find_package(rosidl_default_generators REQUIRED)

rosidl_generate_interfaces(${PROJECT_NAME}
  "srv/StartTracking.srv"
  "srv/ListCameras.srv"
  "srv/SelectCamera.srv"
  "srv/AiTrackingClick.srv"
  "srv/SetYoloProfile.srv"
)

ament_export_dependencies(rosidl_default_runtime)
ament_package()
