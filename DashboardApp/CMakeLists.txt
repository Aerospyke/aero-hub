find_package(rclcpp REQUIRED) # Required to use ROS/ROS node functionality
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
)

set(APPLICATION_RESOURCE_DECLARATION_FILE
        ui/aero_hub.qrc)

set(APPLICATION_SOURCE_FILES
        src/cpp/main.cpp
        src/cpp/animation.cpp
        src/cpp/jsb_settings_tree_model.cpp
        src/cpp/ah_ros_bridge.cpp
        src/cpp/ah_settings.cpp
)

set(APPLICATION_HEADER_FILES
        src/include/animation.h
        src/include/jsb_settings_tree_model.h
        src/include/ah_ros_bridge.h
        src/include/ah_settings.h
)

set(APPLICATION_INCLUDE_DIRECTORIES
        src/include)

# RoboStack / plain CMake: rclcpp + CPython (for generator_py typesupport)
set(LINKED_LIBRARIES_NON_QT
        QmlFlightInstruments
        rclcpp::rclcpp
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
