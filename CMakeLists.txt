cmake_minimum_required(VERSION 3.16)
project(aero-hub LANGUAGES CXX C)

include(QtAppSetup.cmake)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_INSTALL_PREFIX "${CMAKE_SOURCE_DIR}/install-check")

set(QT_INSTALL_LOCATION "/Users/aerospyke/Qt/6.11.0/macos")
set(QT_VERSION_TO_USE 6.11)

set(APPLICATION_NAME "AeroHub")
set(APPLICATION_URI aero_hub)
set(APPLICATION_BUNDLE_GUI_ID de.conorco.aero-hub)
set(APPLICATION_ICON_PATH ${CMAKE_CURRENT_SOURCE_DIR}/app/ui/icons/conor_co.icns)

set(FLIGHT_INSTRUMENTS_MODULE_NAME "FlightInstruments")

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(QT_QML_GENERATE_QML_CACHE ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
list(APPEND CMAKE_PREFIX_PATH ${QT_INSTALL_LOCATION})

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

add_subdirectory(FlightInstruments)
add_subdirectory(app)
