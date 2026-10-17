cmake_minimum_required(VERSION 3.16)

set(QT_QML_GENERATE_QMLLS_INI ON)

# Enable AUTOMOC for Qt meta-object compiler
set(CMAKE_AUTOMOC ON)

# Add include directories for dependencies
include_directories(
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/shared
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Create a simple library for the SearchBar component
# (The QML file is included directly in the main application)
qt_add_qml_module(wikipedia_qt 
    URI wikipedia_qt
    VERSION 1.0
    QML_FILES
        Main.qml
        SearchScreen.qml
    SOURCES
        wikipedia_model_qml_types.h
        constants.cpp
        constants.h
    OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/wikipedia_qt
    RESOURCES
        resources.qrc
        ../icons/home.svg
        ../icons/history.svg
        ../icons/back.svg
        ../icons/app_icon.svg
    DEPENDENCIES TARGET 
        SearchBarModule 
        SidebarModule
        HistoryModule
        ContentDisplayModule
        HeaderModule
        ImageDisplayModule
)

target_link_libraries(wikipedia_qt PRIVATE wikipedia_qt_core)

# Qt scans the executable's source directory recursively for QML imports.
# Declare it here so the scan stays inside src/, away from generated Flatpak
# trees whose runtime symlinks can lead into the host's QML installation.
qt_add_executable(appwikipedia_qt
    MANUAL_FINALIZATION
    main.cpp
)

# Keep the existing binary location after moving the target into this directory.
set_target_properties(appwikipedia_qt PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

# BASE preserves resource paths such as :/icons/app_icon.svg.
qt_add_resources(appwikipedia_qt "icons"
    BASE "${PROJECT_SOURCE_DIR}"
    FILES
        "${PROJECT_SOURCE_DIR}/icons/home.svg"
        "${PROJECT_SOURCE_DIR}/icons/history.svg"
        "${PROJECT_SOURCE_DIR}/icons/back.svg"
        "${PROJECT_SOURCE_DIR}/icons/search.svg"
        "${PROJECT_SOURCE_DIR}/icons/app_icon.svg"
)

qt_add_resources(appwikipedia_qt "styles"
    BASE "${PROJECT_SOURCE_DIR}"
    FILES "${PROJECT_SOURCE_DIR}/styles/table_style.css"
)

target_link_libraries(appwikipedia_qt PRIVATE
    Qt::Quick
    Qt::Widgets
    Qt::SvgWidgets
    wikipedia_qt
    SearchBarModule
    SidebarModule
    HistoryModule
    ContentDisplayModule
    HeaderModule
    HomeModule
    ImageDisplayModule
    SectionModule
    wikipedia_runtime
    wikipedia_qt_core
)

# Finalize in the target's directory before generating deployment scripts.
qt_finalize_executable(appwikipedia_qt)
