cmake_minimum_required(VERSION 3.16)
project(maintelliq-ui-mockups LANGUAGES CXX C)
set(CMAKE_INSTALL_PREFIX "${CMAKE_SOURCE_DIR}/install-check")

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

list(APPEND CMAKE_PREFIX_PATH "/Users/aerospyke/Qt/6.11.0/macos")
set(QT_VERSION_TO_USE 6.11)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(QT_QML_GENERATE_QML_CACHE ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
#set(QT_QML_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/qml_modules" CACHE PATH "Base for QML module outputs")

include(QtAppSetup.cmake)

set(SASVEA_MESSAGE_PROCESSING_LIBRARY_NAME sasvea_message_processing)
add_subdirectory(sasvea-message-processing)# For Testing Only

add_subdirectory(CyberTheme)
add_subdirectory(IshmInterface)
add_subdirectory(WindFarmFleetOverview)
add_subdirectory(WindFarmPhmsPitch)

