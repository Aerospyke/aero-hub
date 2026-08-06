cmake_minimum_required(VERSION 3.16)
project(AhCommon LANGUAGES CXX)

set(AH_COMMON_CORE_SOURCES
  src/string_util.cpp
  src/settings.cpp
)

# Static lib for C++ clients (ah_core, Dashboard) — no runtime dylib dependency.
add_library(ah_common STATIC
  ${AH_COMMON_CORE_SOURCES}
)
add_library(AhCommon::ah_common ALIAS ah_common)

target_include_directories(ah_common
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# Shared lib with C ABI for Python (ctypes) — same settings engine as ah_core.
add_library(ah_common_c SHARED
  ${AH_COMMON_CORE_SOURCES}
  src/c_api.cpp
)
add_library(AhCommon::ah_common_c ALIAS ah_common_c)

target_include_directories(ah_common_c
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# Match Dashboard (C++23). Explicit so colcon/ah_core (CMAKE_CXX_STANDARD 17) still
# builds AhCommon correctly via add_subdirectory.
target_compile_features(ah_common PUBLIC cxx_std_23)
target_compile_features(ah_common_c PUBLIC cxx_std_23)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  target_compile_options(ah_common PRIVATE -Wall -Wextra -Wpedantic)
  target_compile_options(ah_common_c PRIVATE -Wall -Wextra -Wpedantic)
endif()

set_target_properties(ah_common_c PROPERTIES
  OUTPUT_NAME "ah_common_c"
  MACOSX_RPATH ON
  BUILD_RPATH "@loader_path"
  INSTALL_RPATH "@loader_path"
)

install(TARGETS ah_common ah_common_c
  EXPORT AhCommonTargets
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  RUNTIME DESTINATION bin
  INCLUDES DESTINATION include
)

# Small CLI so terminal init can eval domain from settings (no hard-coded 42).
add_executable(ah_settings_shell_exports src/ah_settings_shell_exports.cpp)
target_link_libraries(ah_settings_shell_exports PRIVATE ah_common)
target_compile_features(ah_settings_shell_exports PRIVATE cxx_std_23)

install(DIRECTORY include/ah_common
  DESTINATION include
)

install(TARGETS ah_settings_shell_exports
  RUNTIME DESTINATION bin
)
