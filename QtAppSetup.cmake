macro(find_and_setup_qt)
    set(options)

    set(oneValueArgs
            VERSION
            APPLICATION_NAME
            APPLICATION_URI
            APPLICATION_BUNDLE_GUI_ID
            APPLICATION_RESOURCE_DECLARATION_FILE
            APPLICATION_ICON_PATH
    )
    set(multiValueArgs
            COMPONENTS
            APPLICATION_SOURCE_FILES
            APPLICATION_HEADER_FILES
            APPLICATION_INCLUDE_DIRECTORIES
            LINKED_LIBRARIES_NON_QT
            LINKED_LIBRARIES_QT
            DEPLOY_TOOL_OPTIONS
    )

    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    # Default version
    if (NOT ARG_VERSION)
        set(ARG_VERSION 6.10)
        message(WARNING "VERSION not passed to find_and_setup_qt - using default: ${ARG_VERSION}")
    endif ()

    # Default components
    if (NOT ARG_COMPONENTS)
        set(ARG_COMPONENTS Core Gui Quick Graphs ShaderTools QuickLayouts QuickControls2 Svg)
        message(WARNING "COMPONENTS not passed to find_and_setup_qt - using default: ${ARG_COMPONENTS}")
    endif ()

    find_package(Qt6 ${ARG_VERSION} COMPONENTS ${ARG_COMPONENTS} REQUIRED)

    # Optional extra warning if no components were actually found (rare)
    if (NOT Qt6_FOUND)
        message(FATAL_ERROR "Qt6 ${ARG_VERSION} not found with the requested components: ${ARG_COMPONENTS}")
    endif ()

    if (QT_KNOWN_POLICY_QTP0004)
        qt_policy(SET QTP0004 NEW)
    endif ()

    qt_standard_project_setup(REQUIRES ${ARG_VERSION})

    message(STATUS "Using Qt ${Qt6_VERSION} with components: ${ARG_COMPONENTS}")

    setup_qt_application(
            APPLICATION_NAME ${ARG_APPLICATION_NAME}
            APPLICATION_URI ${ARG_APPLICATION_URI}
            APPLICATION_BUNDLE_GUI_ID ${ARG_APPLICATION_BUNDLE_GUI_ID}
            APPLICATION_ICON_PATH ${ARG_APPLICATION_ICON_PATH}
            APPLICATION_RESOURCE_DECLARATION_FILE ${ARG_APPLICATION_RESOURCE_DECLARATION_FILE}
            APPLICATION_SOURCE_FILES ${ARG_APPLICATION_SOURCE_FILES}
            APPLICATION_HEADER_FILES ${ARG_APPLICATION_HEADER_FILES}
            APPLICATION_INCLUDE_DIRECTORIES ${ARG_APPLICATION_INCLUDE_DIRECTORIES}
            LINKED_LIBRARIES_NON_QT ${ARG_LINKED_LIBRARIES_NON_QT}
            LINKED_LIBRARIES_QT ${ARG_LINKED_LIBRARIES_QT}
            DEPLOY_TOOL_OPTIONS ${ARG_DEPLOY_TOOL_OPTIONS}
    )
endmacro()

function(setup_qt_application)
    # Argument Handling - Start
    set(options)
    set(oneValueArgs
            APPLICATION_NAME
            APPLICATION_URI
            APPLICATION_BUNDLE_GUI_ID
            APPLICATION_RESOURCE_DECLARATION_FILE
            APPLICATION_ICON_PATH
    )
    set(multiValueArgs
            APPLICATION_SOURCE_FILES
            APPLICATION_HEADER_FILES
            APPLICATION_INCLUDE_DIRECTORIES
            LINKED_LIBRARIES_NON_QT
            LINKED_LIBRARIES_QT
            DEPLOY_TOOL_OPTIONS
    )

    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    # Required arguments - fail hard if missing
    if (NOT ARG_APPLICATION_NAME)
        message(FATAL_ERROR "setup_qt_application: APPLICATION_NAME is required")
    endif ()
    if (NOT ARG_APPLICATION_URI)
        message(FATAL_ERROR "setup_qt_application: APPLICATION_URI is required")
    endif ()

    # Optional one-value args with defaults + warning
    if (NOT ARG_APPLICATION_BUNDLE_GUI_ID)
        set(ARG_APPLICATION_BUNDLE_GUI_ID "de.conorco.${ARG_APPLICATION_NAME}")
        message(WARNING "APPLICATION_BUNDLE_GUI_ID not passed - using default: ${ARG_APPLICATION_BUNDLE_GUI_ID}")
    endif ()

    if (NOT ARG_APPLICATION_RESOURCE_DECLARATION_FILE)
        set(ARG_APPLICATION_RESOURCE_DECLARATION_FILE "app/rsc/${ARG_APPLICATION_NAME}.qrc")
        message(WARNING "APPLICATION_RESOURCE_DECLARATION_FILE not passed - using default: ${ARG_APPLICATION_RESOURCE_DECLARATION_FILE}")
    endif ()

    if (NOT ARG_APPLICATION_ICON_PATH)
        message(FATAL_ERROR "APPLICATION_ICON_PATH is required (full path to .icns file)")
    endif ()

    # Extract just the filename for MACOSX_BUNDLE_ICON_FILE
    get_filename_component(ICON_FILE_NAME "${ARG_APPLICATION_ICON_PATH}" NAME)

    # Optional multi-value args with defaults + warning
    if (NOT ARG_APPLICATION_SOURCE_FILES)
        message(WARNING "APPLICATION_SOURCE_FILES not passed - assuming none")
    endif ()

    if (NOT ARG_APPLICATION_HEADER_FILES)
        message(STATUS "APPLICATION_HEADER_FILES not passed - assuming none")
    endif ()

    if (NOT ARG_APPLICATION_INCLUDE_DIRECTORIES)
        set(ARG_APPLICATION_INCLUDE_DIRECTORIES "app/src/include")
        message(WARNING "APPLICATION_INCLUDE_DIRECTORIES not passed - using default: ${ARG_APPLICATION_INCLUDE_DIRECTORIES}")
    endif ()

    if (NOT ARG_LINKED_LIBRARIES_NON_QT)
        message(STATUS "LINKED_LIBRARIES_NON_QT not passed - assuming none")
    endif ()

    if (NOT ARG_LINKED_LIBRARIES_QT)
        set(ARG_LINKED_LIBRARIES_QT Qt::Core Qt::Gui Qt::Quick Qt::Graphs Qt::ShaderTools Qt::QuickLayouts Qt::QuickControls2 Qt::Svg)
        message(WARNING "LINKED_LIBRARIES_QT not passed - using default: ${ARG_LINKED_LIBRARIES_QT}")
    endif ()
    # Argument Handling - Start

    qt_add_resources(APP_RESOURCES ${ARG_APPLICATION_RESOURCE_DECLARATION_FILE})

    # Project Setup - Start
    qt_add_executable(${ARG_APPLICATION_NAME}
            ${ARG_APPLICATION_SOURCE_FILES}
            ${ARG_APPLICATION_HEADER_FILES}
            ${APP_RESOURCES}
    )

    target_include_directories(${ARG_APPLICATION_NAME} PRIVATE
            app/src/include
            ${ARG_APPLICATION_INCLUDE_DIRECTORIES}
    )

    set_target_properties(${ARG_APPLICATION_NAME} PROPERTIES
            WIN32_EXECUTABLE TRUE
            MACOSX_BUNDLE TRUE
    )

    target_link_libraries(${ARG_APPLICATION_NAME} PRIVATE
            ${ARG_LINKED_LIBRARIES_QT}
            ${ARG_LINKED_LIBRARIES_NON_QT}
    )

    # Project Setup - Stop

    # Install/Deployment Handling - Start
    if (APPLE)
        set(MACOSX_BUNDLE_ICON_FILE ${ICON_FILE_NAME})

        set_target_properties(${ARG_APPLICATION_NAME} PROPERTIES
                MACOSX_BUNDLE TRUE
                MACOSX_BUNDLE_GUI_IDENTIFIER ${ARG_APPLICATION_BUNDLE_GUI_ID}
                MACOSX_BUNDLE_BUNDLE_NAME "${ARG_APPLICATION_NAME}"
                MACOSX_BUNDLE_SHORT_VERSION_STRING "1.0"
                MACOSX_BUNDLE_BUNDLE_VERSION "1"
                MACOSX_RPATH ON
                INSTALL_RPATH "@executable_path/../Frameworks"
                BUILD_WITH_INSTALL_RPATH OFF
        )
    endif ()

    if (APPLE)
        add_custom_command(TARGET ${ARG_APPLICATION_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy
                "${ARG_APPLICATION_ICON_PATH}"   # ← full path here
                "$<TARGET_FILE_DIR:${ARG_APPLICATION_NAME}>/../Resources/${ICON_FILE_NAME}"
        )
    endif ()

    install(TARGETS ${ARG_APPLICATION_NAME}
            BUNDLE DESTINATION .
    )

    # Optional extra flags for macdeployqt / windeployqt (caller-supplied; no project paths here).
    qt_generate_deploy_qml_app_script(
            TARGET ${ARG_APPLICATION_NAME}
            OUTPUT_SCRIPT deploy_script
            NO_UNSUPPORTED_PLATFORM_ERROR
            DEPLOY_TOOL_OPTIONS ${ARG_DEPLOY_TOOL_OPTIONS}
    )

    install(SCRIPT ${deploy_script})

    if (APPLE)
        install(CODE "
            execute_process(
                COMMAND codesign --force --deep --sign - \${CMAKE_INSTALL_PREFIX}/${ARG_APPLICATION_NAME}.app
                RESULT_VARIABLE SIGN_RESULT
            )
            if(NOT SIGN_RESULT EQUAL 0)
                message(FATAL_ERROR \"Ad-hoc signing failed with code \${SIGN_RESULT}\")
            endif()
        " COMPONENT Runtime)
    endif ()
    # Install/Deployment Handling - Stop
endfunction()