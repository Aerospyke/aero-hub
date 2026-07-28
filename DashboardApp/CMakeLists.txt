find_package(rclcpp REQUIRED) # Required to use ROS/ROS node functionality

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
)

set(APPLICATION_HEADER_FILES
        src/include/animation.h
        src/include/jsb_settings_tree_model.h
)

set(APPLICATION_INCLUDE_DIRECTORIES
        src/include)

set(LINKED_LIBRARIES_INTERNAL
        QmlFlightInstruments
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
        LINKED_LIBRARIES_INTERNAL ${LINKED_LIBRARIES_INTERNAL}
        LINKED_LIBRARIES_QT ${LINKED_LIBRARIES_QT})