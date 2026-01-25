# HistoryModule CMakeLists.txt
cmake_minimum_required(VERSION 3.16)

set(QT_QML_GENERATE_QMLLS_INI ON)

# Enable AUTOMOC for Qt meta-object compiler
set(CMAKE_AUTOMOC ON)

# Add include directories for dependencies
include_directories(
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Create a simple library for the History component
qt_add_qml_module(ContentDisplayModule
    URI wikipedia_qt.ContentDisplay
    VERSION 1.0
    QML_FILES
        ContentDisplay.qml
    SOURCES
        ContentDisplayModel.cpp
        ContentDisplayModel.h
    OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/wikipedia_qt/ContentDisplay
)

target_link_libraries(ContentDisplayModule PRIVATE Qt::Quick Qt::Core Qt::Network)

