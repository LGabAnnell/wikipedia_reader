# SearchBarModule CMakeLists.txt
cmake_minimum_required(VERSION 3.16)

set(QT_QML_GENERATE_QMLLS_INI ON)

# Enable AUTOMOC for Qt meta-object compiler
set(CMAKE_AUTOMOC ON)

# Add include directories for dependencies
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/wikipedia_client
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Create a simple library for the SearchBar component
# (The QML file is included directly in the main application)
qt_add_qml_module(SearchBarModule 
    URI 
        SearchBar
    VERSION 1.0
    QML_FILES
        SearchBar.qml
    SOURCES
        SearchBarModel.cpp
        SearchBarModel.h
)

target_link_libraries(SearchBarModule PRIVATE Qt::Quick Qt::Core Qt::Network)