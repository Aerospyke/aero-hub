cmake_minimum_required(VERSION 3.16)
project(ah_common)

find_package(ament_cmake REQUIRED)

set(_AH_COMMON_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../../AhCommon")
if(NOT EXISTS "${_AH_COMMON_DIR}/CMakeLists.txt")
  message(FATAL_ERROR "AhCommon not found at ${_AH_COMMON_DIR}")
endif()

if(NOT TARGET AhCommon::ah_common)
  add_subdirectory("${_AH_COMMON_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/AhCommon")
endif()

# Libraries for C++ / Python ctypes; shell helper installed to run/bin by build_ros.
# RUNTIME → bin so path is always: install/ah_common/bin/ah_settings_shell_exports
install(TARGETS ah_common ah_common_c
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION lib
)
install(TARGETS ah_settings_shell_exports
  RUNTIME DESTINATION bin
)

install(DIRECTORY "${_AH_COMMON_DIR}/include/ah_common"
  DESTINATION include
)

ament_export_include_directories(include)
ament_export_libraries(ah_common ah_common_c)
ament_package()
