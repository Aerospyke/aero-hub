cmake_minimum_required(VERSION 3.16)
project(aero-hub LANGUAGES CXX C)

include(QtAppSetup.cmake)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_INSTALL_PREFIX "${CMAKE_SOURCE_DIR}/run")

set(QT_INSTALL_LOCATION "/Users/aerospyke/Qt/6.11.0/macos")
set(QT_VERSION_TO_USE 6.11)

set(APPLICATION_NAME "AeroHub")
set(APPLICATION_URI aero_hub)
set(APPLICATION_BUNDLE_GUI_ID de.conorco.aero-hub)
set(APPLICATION_ICON_PATH ${CMAKE_CURRENT_SOURCE_DIR}/DashboardApp/ui/icons/conor_co.icns)

set(FLIGHT_INSTRUMENTS_MODULE_NAME "FlightInstruments")

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(QT_QML_GENERATE_QML_CACHE ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
list(APPEND CMAKE_PREFIX_PATH ${QT_INSTALL_LOCATION})


# RoboStack: prefer configuring with conda env active (conda activate ros_env && open -na CLion.app).
# adding the ROS environment via CMAKE_PREFIX_PATH is only necessary if building from a terminal which has not already
# loaded a ROS virtual environment (e.g. via Conda on macOS) or run the ROS setup script or started CLion without first
# activating the ros_env
# Cold start (CLion from Dock): CONDA_PREFIX is unset — fall back to default ros_env so
# find_package(rclcpp) can resolve ament typesupports (avoids "No 'rosidl_typesupport_c' found").
if(NOT DEFINED ENV{CONDA_PREFIX} OR "$ENV{CONDA_PREFIX}" STREQUAL "")
    set(_ah_default_ros_env "$ENV{HOME}/miniconda3/envs/ros_env")
    if(EXISTS "${_ah_default_ros_env}/share/rclcpp/cmake")
        set(ENV{CONDA_PREFIX} "${_ah_default_ros_env}")
        set(ENV{AMENT_PREFIX_PATH} "${_ah_default_ros_env}")
        set(ENV{RMW_IMPLEMENTATION} "rmw_fastrtps_cpp")
        list(PREPEND CMAKE_PREFIX_PATH "${_ah_default_ros_env}")
        message(STATUS "ROS: cold start — using ${_ah_default_ros_env}")
    else()
        message(WARNING
            "ROS: CONDA_PREFIX unset and ${_ah_default_ros_env} not found. "
            "Use: conda activate ros_env && open -na CLion.app")
    endif()
endif()

find_package(Qt6 ${QT_VERSION_TO_USE} COMPONENTS
        Core
        Gui
        Quick
        Graphs
        ShaderTools
        QuickLayouts
        QuickControls2
        REQUIRED
)

if (QT_KNOWN_POLICY_QTP0004)
    qt_policy(SET QTP0004 NEW)
endif ()

# Std-only shared library (settings, string helpers). C++17; linked by Dashboard + ah_core.
add_subdirectory(AhCommon)

# Optional: rebuild ROS packages via colcon (not a hard dependency of AeroHub).
# Invokes ros/scripts/build_ros.sh so colcon is found when CMake/CLion lack conda PATH.
# Builds: ah_common (AhCommon lib + libah_common_c for ah_yolo), ah_msgs, ah_core, ah_yolo.
# Ends by populating run/bin (ah_settings_shell_exports).
# Usage:  cmake --build <build-dir> --target build_ros
# Then:   cd run && source ./init_ah_ros_in_terminal.sh
# Equivalent:  ./ros/scripts/build_ros.sh
add_custom_target(build_ros
  COMMAND bash "${CMAKE_SOURCE_DIR}/ros/scripts/build_ros.sh"
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/ros
  COMMENT "colcon build ROS stack + populate run/"
  VERBATIM
  USES_TERMINAL
)

# Install/copy runtime helper into run/ without a full colcon rebuild (if already built).
# Build-side only — operators never invoke this; use run/ for day-to-day work.
add_custom_target(populate_run
  COMMAND bash "${CMAKE_SOURCE_DIR}/ros/scripts/populate_runtime.sh"
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  COMMENT "Populate aero-hub/run/bin (ah_settings_shell_exports)"
  VERBATIM
  USES_TERMINAL
)

add_subdirectory(FlightInstruments)
add_subdirectory(DashboardApp)
