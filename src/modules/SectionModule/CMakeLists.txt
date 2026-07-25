# SectionModule CMakeLists.txt
cmake_minimum_required(VERSION 3.16)

set(QT_QML_GENERATE_QMLLS_INI ON)

# Enable AUTOMOC for Qt meta-object compiler
set(CMAKE_AUTOMOC ON)

# Add include directories for dependencies
include_directories(
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Create a simple library for the Section component
qt_add_qml_module(SectionModule
    URI wikipedia_qt.Section
    VERSION 1.0
    QML_FILES
        Section.qml
    SOURCES
        SectionModel.cpp
        SectionModel.h
    OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/wikipedia_qt/Section
)

target_link_libraries(SectionModule PRIVATE Qt::Quick Qt::Core Qt::Network)

