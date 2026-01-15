# SearchBarModule CMakeLists.txt
cmake_minimum_required(VERSION 3.16)

# Enable AUTOMOC for Qt meta-object compiler
set(CMAKE_AUTOMOC ON)

# Add include directories for dependencies
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/wikipedia_client
)

# Create a QML module for the SearchBar component
qt_add_qml_module(SearchBarModule
    URI SearchBar
    VERSION 1.0
    QML_FILES
        SearchBar.qml
    SOURCES
        SearchBarModel.cpp
        SearchBarModel.h
    RESOURCES
        # Add any resources needed by the SearchBar module
)

target_link_libraries(SearchBarModule PRIVATE Qt::Quick Qt::Core Qt::Network)

# Install the module
install(TARGETS SearchBarModule
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
