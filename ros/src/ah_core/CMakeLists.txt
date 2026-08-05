cmake_minimum_required(VERSION 3.16)
project(ah_core)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(std_msgs REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(std_srvs REQUIRED)
find_package(ah_msgs REQUIRED)
find_package(OpenCV REQUIRED COMPONENTS core imgproc imgcodecs videoio)

# Sibling AhCommon (std-only settings) — same sources as Dashboard root build.
set(_AH_COMMON_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../../AhCommon")
if(NOT TARGET AhCommon::ah_common)
  if(NOT EXISTS "${_AH_COMMON_DIR}/CMakeLists.txt")
    message(FATAL_ERROR "AhCommon not found at ${_AH_COMMON_DIR}")
  endif()
  add_subdirectory("${_AH_COMMON_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/AhCommon")
endif()

add_executable(ah_core_node
  src/main.cpp
  src/ah_core_node.cpp
  src/camera_devices.cpp
  src/ros_runtime_settings.cpp
)

target_include_directories(ah_core_node
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${OpenCV_INCLUDE_DIRS}
)

target_link_libraries(ah_core_node
  ${OpenCV_LIBS}
  AhCommon::ah_common
)

ament_target_dependencies(ah_core_node
  rclcpp
  std_msgs
  sensor_msgs
  std_srvs
  ah_msgs
)

install(TARGETS ah_core_node
  DESTINATION lib/${PROJECT_NAME}
)

ament_package()
