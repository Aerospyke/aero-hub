find_package(rclcpp REQUIRED) # Required to use ROS/ROS node functionality
find_package(std_msgs REQUIRED) # /ah/system/status is std_msgs/String (JSON)
find_package(sensor_msgs REQUIRED) # /ah/video/compressed is sensor_msgs/CompressedImage
find_package(std_srvs REQUIRED) # /ah/tracking/stop|cancel
# Local colcon install (ah_msgs StartTracking.srv) — build ros/ first if missing
list(PREPEND CMAKE_PREFIX_PATH "${CMAKE_SOURCE_DIR}/ros/install")
find_package(ah_msgs REQUIRED)
# RoboStack's rclcpp deps pull in *_rosidl_generator_py dylibs that reference
# CPython symbols but do not link libpython. A pure C++ app must link Python
# or dyld fails at launch with: symbol not found … '_PyExc_RuntimeError'.
find_package(Python3 REQUIRED COMPONENTS Development)

set(QT_PACKAGES
        Gui
        Quick
        Graphs
        ShaderTools
        QuickLayouts
        QuickControls2
        Svg
)

set(APPLICATION_RESOURCE_DECLARATION_FILE
        ui/aero_hub.qrc)

set(APPLICATION_SOURCE_FILES
        src/cpp/main.cpp
        src/cpp/animation.cpp
        src/cpp/jsb_settings_tree_model.cpp
        src/cpp/ah_ros_bridge.cpp
        src/cpp/ah_settings.cpp
        src/cpp/ah_system_status.cpp
        src/cpp/ah_video_feed.cpp
        src/cpp/ah_track_controller.cpp
        src/cpp/ah_camera_controller.cpp
        src/cpp/ah_detections_model.cpp
        src/cpp/ah_yolo_controller.cpp
)

set(APPLICATION_HEADER_FILES
        src/include/animation.h
        src/include/jsb_settings_tree_model.h
        src/include/ah_ros_bridge.h
        src/include/ah_settings.h
        src/include/ah_system_status.h
        src/include/ah_video_feed.h
        src/include/ah_track_controller.h
        src/include/ah_camera_controller.h
        src/include/ah_detections_model.h
        src/include/ah_yolo_controller.h
)

set(APPLICATION_INCLUDE_DIRECTORIES
        src/include)

# RoboStack / plain CMake: rclcpp + msgs + CPython (for generator_py typesupport)
set(LINKED_LIBRARIES_NON_QT
        QmlFlightInstruments
        AhCommon::ah_common
        rclcpp::rclcpp
        ${std_msgs_TARGETS}
        ${sensor_msgs_TARGETS}
        ${std_srvs_TARGETS}
        ${ah_msgs_TARGETS}
        Python3::Python
)

set(LINKED_LIBRARIES_QT
        Qt::Core
        Qt::Gui
        Qt::Quick
        Qt::Graphs
        Qt::ShaderTools
        Qt::QuickLayouts
        Qt::QuickControls2
        Qt::Svg
)

find_and_setup_qt(COMPONENTS ${QT_PACKAGES}
        VERSION ${QT_VERSION_TO_USE}
        APPLICATION_NAME ${APPLICATION_NAME}
        APPLICATION_URI ${APPLICATION_URI}
        APPLICATION_BUNDLE_GUI_ID ${APPLICATION_BUNDLE_GUI_ID}
        APPLICATION_ICON_PATH ${APPLICATION_ICON_PATH}
        APPLICATION_RESOURCE_DECLARATION_FILE ${APPLICATION_RESOURCE_DECLARATION_FILE}
        APPLICATION_SOURCE_FILES ${APPLICATION_SOURCE_FILES}
        APPLICATION_HEADER_FILES ${APPLICATION_HEADER_FILES}
        APPLICATION_INCLUDE_DIRECTORIES ${APPLICATION_INCLUDE_DIRECTORIES}
        LINKED_LIBRARIES_NON_QT ${LINKED_LIBRARIES_NON_QT}
        LINKED_LIBRARIES_QT ${LINKED_LIBRARIES_QT})

# Lab install RPATH: Qt Frameworks + RoboStack lib + workspace ah_msgs.
# macdeployqt rewrites the binary and often drops non-Frameworks rpaths — re-apply after deploy.
if(DEFINED ENV{CONDA_PREFIX} AND NOT "$ENV{CONDA_PREFIX}" STREQUAL "")
    set(_AH_LAB_ROS_LIB "$ENV{CONDA_PREFIX}/lib")
else()
    set(_AH_LAB_ROS_LIB "$ENV{HOME}/miniconda3/envs/ros_env/lib")
endif()
set(_AH_MSGS_LIB "${CMAKE_SOURCE_DIR}/ros/install/ah_msgs/lib")
if(APPLE)
    set_target_properties(${APPLICATION_NAME} PROPERTIES
        INSTALL_RPATH "@executable_path/../Frameworks;${_AH_LAB_ROS_LIB};${_AH_MSGS_LIB}")
    install(CODE "
        set(_exe \"\${CMAKE_INSTALL_PREFIX}/${APPLICATION_NAME}.app/Contents/MacOS/${APPLICATION_NAME}\")
        if(EXISTS \"\${_exe}\")
            execute_process(COMMAND install_name_tool -add_rpath \"${_AH_LAB_ROS_LIB}\" \"\${_exe}\" ERROR_QUIET)
            execute_process(COMMAND install_name_tool -add_rpath \"${_AH_MSGS_LIB}\" \"\${_exe}\" ERROR_QUIET)
            execute_process(COMMAND codesign --force --deep --sign - \"\${CMAKE_INSTALL_PREFIX}/${APPLICATION_NAME}.app\" ERROR_QUIET)
            message(STATUS \"AeroHub: re-applied lab INSTALL_RPATH after macdeployqt\")
        endif()
    ")
else()
    set_target_properties(${APPLICATION_NAME} PROPERTIES
        INSTALL_RPATH "$ORIGIN;$ORIGIN/../lib;${_AH_LAB_ROS_LIB};${_AH_MSGS_LIB}")
endif()
