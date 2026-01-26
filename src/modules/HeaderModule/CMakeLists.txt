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

# Find tinyxml2
find_package(tinyxml2 REQUIRED)
# Create a simple library for the Header component
qt_add_qml_module(HeaderModule
    URI wikipedia_qt.Header
    VERSION 1.0
    QML_FILES
        Header.qml
    SOURCES
        HeaderModel.cpp
        HeaderModel.h
        SvgImageProvider.cpp
        SvgImageProvider.h
    OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/wikipedia_qt/Header
)

target_link_libraries(HeaderModule PRIVATE Qt::Quick Qt::Core Qt::Network Qt::Svg tinyxml2::tinyxml2)

