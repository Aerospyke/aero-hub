find_package(Qt6 ${QT_VERSION_TO_USE} REQUIRED COMPONENTS Quick QuickControls2)   # 6.10 will work fine

qt_standard_project_setup(REQUIRES ${QT_VERSION_TO_USE})

set_source_files_properties(
        CyberTheme.qml
        PROPERTIES QT_QML_SINGLETON_TYPE TRUE
)

qt_add_library(CyberTheme STATIC)
set(FONT_RESOURCE_FILES
        fonts/ShareTechMono-Regular.ttf
        fonts/BarlowCondensed-Bold.ttf
)

qt_add_qml_module(CyberTheme
        URI CyberTheme
        VERSION 1.0
        QML_FILES CyberTheme.qml
        RESOURCES
        ${FONT_RESOURCE_FILES}
        #        OUTPUT_DIRECTORY ${QT_QML_OUTPUT_DIRECTORY}/CyberTheme   # ← this forces it to nest properly
)

target_link_libraries(CyberTheme PRIVATE Qt6::Quick)