# src/modules/HomeModule/CMakeLists.txt

cmake_minimum_required(VERSION 3.16)

qt_standard_project_setup(REQUIRES 6.8)

set(QT_QML_GENERATE_QMLLS_INI ON)

# Enable AUTOMOC for Qt meta-object compiler
set(CMAKE_AUTOMOC ON)

# Add include directories for dependencies
include_directories(
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}
)

qt6_add_qml_module(HomeModule
    URI wikipedia_qt.Home
    VERSION 1.0
    QML_FILES
        HomeScreen.qml
        DidYouKnowSection.qml
        OnThisDaySection.qml
        NewsSection.qml
    SOURCES
        HomeModel.cpp
        HomeModel.h
    RESOURCES
        home_resources.qrc
    OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/wikipedia_qt/Home
)

# Add this module to the main library
target_link_libraries(HomeModule PRIVATE Qt::Quick Qt::Core)