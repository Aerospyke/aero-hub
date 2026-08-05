cmake_minimum_required(VERSION 3.16)
project(AhCommon LANGUAGES CXX)

add_library(ah_common STATIC
  src/string_util.cpp
  src/settings.cpp
)

add_library(AhCommon::ah_common ALIAS ah_common)

target_include_directories(ah_common
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_compile_features(ah_common PUBLIC cxx_std_17)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  target_compile_options(ah_common PRIVATE -Wall -Wextra -Wpedantic)
endif()
