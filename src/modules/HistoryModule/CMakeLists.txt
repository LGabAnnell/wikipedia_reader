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
qt_add_qml_module(HistoryModule 
    URI wikipedia_qt.History
    VERSION 1.0
    QML_FILES
        History.qml
    SOURCES
        HistoryModel.cpp
        HistoryModel.h
    OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/wikipedia_qt/History
)

target_link_libraries(HistoryModule PRIVATE Qt::Quick Qt::Core Qt::Network)

