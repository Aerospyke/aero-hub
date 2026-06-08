cmake_minimum_required(VERSION 4.1)
project(qml-flight-instruments)

include(QtAppSetup.cmake)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_INSTALL_PREFIX "${CMAKE_SOURCE_DIR}/install-target")

# Set your QT Install Location and Version Tag Here! /Users/${username}/Qt/6.11.0/macos is a typical macOS installation
set(QT_INSTALL_LOCATION "/Users/aerospyke/Qt/6.11.0/macos")
set(QT_VERSION_TO_USE 6.11)
# Set your custom build information here (Bundle ID and Icon Path are needed for macOS deployment)
set(APPLICATION_URI flight_instruments_demo)
set(APPLICATION_BUNDLE_GUI_ID de.conorco.flight_instruments_demo)
set(APPLICATION_ICON_PATH ${CMAKE_CURRENT_SOURCE_DIR}/application/ui/icons/conor_co.icns)

set(APPLICATION_NAME "QmlFlightInstrumentsDemo")

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(QT_QML_GENERATE_QML_CACHE ON)
set(CMAKE_AUTOMAC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
list(APPEND CMAKE_PREFIX_PATH ${QT_INSTALL_LOCATION})

add_subdirectory(application)
