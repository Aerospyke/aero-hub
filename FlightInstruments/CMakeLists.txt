#find_package(Qt6 ${QT_VERSION_TO_USE} COMPONENTS
#        Core
#        Gui
#        Quick
#        Graphs
#        ShaderTools
#        QuickLayouts
#        QuickControls2
#        REQUIRED
#)

# The parent project (root CMakeLists.txt) is expected to have already performed
# find_package(Qt6 ...) and qt_standard_project_setup so that qt_add_* commands
# are available here.

qt_add_library(QmlFlightInstruments STATIC)

target_link_libraries(QmlFlightInstruments
        PRIVATE
        Qt::Core
        Qt::Gui
        Qt::Quick
        Qt::Graphs
        Qt::ShaderTools
        Qt::QuickLayouts
        Qt::QuickControls2
        Qt::Svg
)

target_include_directories(QmlFlightInstruments
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# --- QML module declaration (this also registers C++ types for QML) ---
# We list the QML files so that the module system / qmllint / tooling know about them
# and so PrimaryFlightData can be used as a QML type via "import FlightInstruments".
qt_add_qml_module(QmlFlightInstruments
        URI FlightInstruments
        VERSION 1.0
        RESOURCE_PREFIX "/qt/qml"
        # C++ type(s) that should be usable from QML when the module is imported.
        SOURCES
        include/primary_flight_data.h
        src/primary_flight_data.cpp
        QML_FILES
        qml/basic_six_root_display.qml
        qml/efis_root_display.qml
        qml/gauges_root_display.qml
        qml/basic-six/airspeed_indicator.qml
        qml/basic-six/altimeter.qml
        qml/basic-six/attitude_indicator.qml
        qml/basic-six/basic_six_image.qml
        qml/basic-six/heading_indicator.qml
        qml/basic-six/turn_coordinator.qml
        qml/basic-six/vertical_speed_indicator.qml
        qml/efis/eadi/airspeed_indicator.qml
        qml/efis/eadi/altimeter.qml
        qml/efis/eadi/attitude_indicator.qml
        qml/efis/eadi/eadi_image.qml
        qml/efis/eadi/electronic_attitude_direction_indicator.qml
        qml/efis/eadi/heading_indicator.qml
        qml/efis/eadi/labels.qml
        qml/efis/eadi/vertical_speed_indicator.qml
        qml/efis/ehsi/ehsi_image.qml
        qml/efis/ehsi/electronic_horizontal_situation_inidicator.qml
        qml/efis/ehsi/labels.qml
        qml/gauges/circular_gauge.qml
        qml/gauges/circular_gauge_style.qml
        qml/gauges/egt-fuel-flow-gauge/egt_fuel_flow_gauge.qml
        qml/gauges/egt-fuel-flow-gauge/egt_gauge.qml
        qml/gauges/egt-fuel-flow-gauge/fuel_flow_gauge.qml
        qml/gauges/propeller-gauge/propeller_gauge.qml
        qml/gauges/propeller-gauge/propeller_gauge_style.qml
        qml/gauges/propeller-gauge/singular_gauge_foreground.qml
        qml/gauges/tank-gauge/base_tank_gauge.qml
        qml/gauges/tank-gauge/base_tank_gauge_style.qml
        qml/gauges/tank-gauge/left_tank_gauge.qml
        qml/gauges/tank-gauge/right_tank_gauge.qml
        qml/gauges/tank-gauge/tank_gauge.qml
        qml/gauges/temperature-pressure-gauge/pressure_gauge.qml
        qml/gauges/temperature-pressure-gauge/temperature_gauge.qml
        qml/gauges/temperature-pressure-gauge/temperature_pressure_gauge.qml
        qml/gauges/util/gauge_canvas.qml
        qml/gauges/util/gauge_image.qml
        qml/gauges/util/gauge_mask.qml
        qml/gauges/util/gauge_needle_standard.qml
        qml/gauges/util/gauge_text.qml
        qml/gauges/vac-amp-gauge/amp_gauge.qml
        qml/gauges/vac-amp-gauge/vac_amp_gauge.qml
        qml/gauges/vac-amp-gauge/vac_gauge.qml
)

target_sources(QmlFlightInstruments PRIVATE QmlFlightInstruments.qrc)

if (TARGET QmlFlightInstrumentsplugin)
    target_link_libraries(QmlFlightInstruments INTERFACE QmlFlightInstrumentsplugin)
endif ()

# Namespaced alias for modern consuming CMakeLists.txt style.
add_library(QmlFlightInstruments::QmlFlightInstruments ALIAS QmlFlightInstruments)

# Convenience cache variables for advanced consumers.
set(QML_FLIGHT_INSTRUMENTS_QML_IMPORT_PATH ${CMAKE_CURRENT_SOURCE_DIR} CACHE INTERNAL "Base dir for this QML module (contains qmldir after build)")
