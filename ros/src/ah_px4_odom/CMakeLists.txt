cmake_minimum_required(VERSION 3.16)
project(ah_px4_odom)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(px4_msgs REQUIRED)
find_package(nav_msgs REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(tf2_ros REQUIRED)

add_executable(ah_px4_odom_node
  src/main.cpp
  src/ah_px4_odom_node.cpp
)

target_include_directories(ah_px4_odom_node
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

ament_target_dependencies(ah_px4_odom_node
  rclcpp
  px4_msgs
  nav_msgs
  geometry_msgs
  tf2_ros
)

add_executable(ah_px4_odom_flatten_test
  test/test_flatten.cpp
)

target_include_directories(ah_px4_odom_flatten_test
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

install(TARGETS ah_px4_odom_node
  DESTINATION lib/${PROJECT_NAME}
)

ament_package()
