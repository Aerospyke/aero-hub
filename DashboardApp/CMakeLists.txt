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

# Lab deploy (macOS): not a self-contained ROS distribution.
# macdeployqt follows link-time deps only. For a lab ROS install we still:
#  1) stage colcon ah_msgs into Frameworks (workspace overlay, not on system rpath)
#  2) add LC_RPATH → lab ROS lib (RoboStack) so RMW/typesupport dlopen works
# Linux: rely on LD_LIBRARY_PATH / RPATH from the sourced ROS env + colcon overlay.
if(APPLE)
    set(_AH_MSGS_LIB_DIR "${CMAKE_SOURCE_DIR}/ros/install/ah_msgs/lib")
    if(DEFINED ENV{CONDA_PREFIX} AND NOT "$ENV{CONDA_PREFIX}" STREQUAL "")
        set(_AH_CONDA_LIB_DIR "$ENV{CONDA_PREFIX}/lib")
        set(_AH_CONDA_PREFIX "$ENV{CONDA_PREFIX}")
    else()
        set(_AH_CONDA_PREFIX "$ENV{HOME}/miniconda3/envs/ros_env")
        set(_AH_CONDA_LIB_DIR "${_AH_CONDA_PREFIX}/lib")
    endif()
    set(_AH_ROS_INSTALL "${CMAKE_SOURCE_DIR}/ros/install")

    install(CODE "
        set(_app \"\${CMAKE_INSTALL_PREFIX}/${APPLICATION_NAME}.app\")
        set(_exe \"\${_app}/Contents/MacOS/${APPLICATION_NAME}\")
        set(_fw  \"\${_app}/Contents/Frameworks\")
        set(_ah_msgs \"${_AH_MSGS_LIB_DIR}\")
        set(_conda_lib \"${_AH_CONDA_LIB_DIR}\")
        set(_conda_prefix \"${_AH_CONDA_PREFIX}\")
        set(_ros_install \"${_AH_ROS_INSTALL}\")
        if(NOT EXISTS \"\${_exe}\")
            message(FATAL_ERROR \"AeroHub ROS fixup: missing \${_exe}\")
        endif()
        file(MAKE_DIRECTORY \"\${_fw}\")

        # 1) Bundle local ah_msgs into Frameworks and fix main binary load commands
        if(EXISTS \"\${_ah_msgs}\")
            file(GLOB _ah_dylibs \"\${_ah_msgs}/libah_msgs*.dylib\")
            foreach(_lib \${_ah_dylibs})
                get_filename_component(_name \"\${_lib}\" NAME)
                file(COPY \"\${_lib}\" DESTINATION \"\${_fw}\")
                execute_process(COMMAND install_name_tool
                    -id \"@loader_path/../Frameworks/\${_name}\" \"\${_fw}/\${_name}\"
                    ERROR_QUIET)
                execute_process(COMMAND install_name_tool
                    -change \"@rpath/\${_name}\"
                            \"@loader_path/../Frameworks/\${_name}\"
                            \"\${_exe}\"
                    ERROR_QUIET)
            endforeach()
            foreach(_lib \${_ah_dylibs})
                get_filename_component(_name \"\${_lib}\" NAME)
                set(_dst \"\${_fw}/\${_name}\")
                execute_process(COMMAND otool -L \"\${_dst}\" OUTPUT_VARIABLE _otool_out)
                string(REGEX MATCHALL \"@rpath/[A-Za-z0-9_.+-]+\" _rpath_deps \"\${_otool_out}\")
                foreach(_dep \${_rpath_deps})
                    string(REPLACE \"@rpath/\" \"\" _dname \"\${_dep}\")
                    if(EXISTS \"\${_fw}/\${_dname}\")
                        execute_process(COMMAND install_name_tool
                            -change \"\${_dep}\" \"@loader_path/\${_dname}\" \"\${_dst}\"
                            ERROR_QUIET)
                    endif()
                endforeach()
            endforeach()
        else()
            message(WARNING \"AeroHub ROS fixup: \${_ah_msgs} missing (build ROS first)\")
        endif()

        # 2) Match IDE/build-tree: rpath into RoboStack lib for RMW/typesupport plugins
        if(EXISTS \"\${_conda_lib}\")
            execute_process(
                COMMAND install_name_tool -add_rpath \"\${_conda_lib}\" \"\${_exe}\"
                ERROR_QUIET)
            message(STATUS \"AeroHub: added rpath \${_conda_lib}\")
        else()
            message(WARNING \"AeroHub ROS fixup: \${_conda_lib} missing; activate ros_env before install\")
        endif()

        # 3) Optional: rpath to colcon install package libs (ah_msgs already copied)
        if(EXISTS \"\${_ros_install}/ah_msgs/lib\")
            execute_process(
                COMMAND install_name_tool -add_rpath \"\${_ros_install}/ah_msgs/lib\" \"\${_exe}\"
                ERROR_QUIET)
        endif()

        execute_process(
            COMMAND codesign --force --deep --sign - \"\${_app}\"
            RESULT_VARIABLE _sign_rc)
        if(NOT _sign_rc EQUAL 0)
            message(FATAL_ERROR \"AeroHub ROS fixup: codesign failed (\${_sign_rc})\")
        endif()
        message(STATUS \"AeroHub: ah_msgs bundled + RoboStack rpath applied\")
    ")
endif()
