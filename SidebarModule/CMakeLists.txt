# SidebarModule CMakeLists.txt
cmake_minimum_required(VERSION 3.16)

set(QT_QML_GENERATE_QMLLS_INI ON)

# Enable AUTOMOC for Qt meta-object compiler
set(CMAKE_AUTOMOC ON)

# Add include directories for dependencies
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Create a simple library for the Sidebar component
qt_add_qml_module(SidebarModule 
    URI Sidebar
    VERSION 1.0
    QML_FILES
        Sidebar.qml
    SOURCES
        SidebarModel.cpp
        SidebarModel.h
    OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/wikipedia_qt/Sidebar
)

target_link_libraries(SidebarModule PRIVATE Qt::Quick Qt::Core)
