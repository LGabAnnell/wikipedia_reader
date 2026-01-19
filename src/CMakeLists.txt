cmake_minimum_required(VERSION 3.16)

set(QT_QML_GENERATE_QMLLS_INI ON)

# Enable AUTOMOC for Qt meta-object compiler
set(CMAKE_AUTOMOC ON)

# Add include directories for dependencies
include_directories(
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Create a simple library for the SearchBar component
# (The QML file is included directly in the main application)
qt_add_qml_module(wikipedia_qt 
    URI wikipedia_qt
    VERSION 1.0
    QML_FILES
        Main.qml
        Header.qml
        MainContent.qml
    SOURCES
        wikipedia_client.cpp
        wikipedia_client.h
    OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/wikipedia_qt
    RESOURCES
        ../icons/home.jpg
        ../icons/history.jpg
        ../icons/bookmark.jpg
        ../icons/safes.jpg
        ../icons/location.png
    DEPENDENCIES TARGET 
        SearchBarModule 
        SidebarModule
        HistoryModule
)
