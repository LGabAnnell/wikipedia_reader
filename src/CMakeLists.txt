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
        wikipedia_search_client.cpp
        wikipedia_search_client.h
        wikipedia_page_client.cpp
        wikipedia_page_client.h
        wikipedia_featured_client.cpp
        wikipedia_featured_client.h
        wikipedia_home_client.cpp
        wikipedia_home_client.h
        wikipedia_models.h
        constants.cpp
        constants.h
    OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/wikipedia_qt
    RESOURCES
        ../icons/home.svg
        ../icons/history.svg
        ../icons/back.svg
    DEPENDENCIES TARGET 
        SearchBarModule 
        SidebarModule
        HistoryModule
        ContentDisplayModule
        HeaderModule
)

