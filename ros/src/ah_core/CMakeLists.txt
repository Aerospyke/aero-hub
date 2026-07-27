cmake_minimum_required(VERSION 3.8)
project(ah_core)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(std_msgs REQUIRED)

add_executable(ah_core_node src/ah_core_node.cpp)
ament_target_dependencies(ah_core_node rclcpp std_msgs)

install(TARGETS ah_core_node
  DESTINATION lib/${PROJECT_NAME}
)

ament_package()
