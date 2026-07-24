# ImageDisplayModule CMakeLists.txt
cmake_minimum_required(VERSION 3.16)

set(QT_QML_GENERATE_QMLLS_INI ON)

# Enable AUTOMOC for Qt meta-object compiler
set(CMAKE_AUTOMOC ON)

# Add include directories for dependencies
include_directories(
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Create a simple library for the ImageDisplay component
qt_add_qml_module(ImageDisplayModule
    URI wikipedia_qt.ImageDisplay
    VERSION 1.0
    QML_FILES
        ImageHome.qml
        ImageGallery.qml
    SOURCES
        ImageHomeModel.cpp
        ImageHomeModel.h
    OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/wikipedia_qt/ImageDisplay
)

target_link_libraries(ImageDisplayModule PRIVATE Qt::Quick Qt::Core Qt::Network)

