cmake_minimum_required(VERSION 3.16)

project(wikipedia_qt VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(QT_QML_GENERATE_QMLLS_INI ON)

find_package(Qt6 REQUIRED COMPONENTS Quick)
find_package(Qt6 REQUIRED COMPONENTS Core)
find_package(Qt6 REQUIRED COMPONENTS Network)
find_package(nlohmann_json REQUIRED)
find_package(CURL REQUIRED)

qt_standard_project_setup(REQUIRES 6.8)

qt_add_executable(appwikipedia_qt
    src/main.cpp
    src/components/searchbar.cpp
    src/wikipedia_client/wikipedia_client.cpp
)

target_link_libraries(appwikipedia_qt PRIVATE Qt6::Network)
target_link_libraries(appwikipedia_qt PRIVATE nlohmann_json::nlohmann_json)
target_link_libraries(appwikipedia_qt PRIVATE CURL::libcurl)

qt_add_resources(appwikipedia_qt 
    "icons"
    FILES
        icons/home.jpg
        icons/history.jpg
        icons/bookmark.jpg
        icons/safes.jpg
        icons/location.png
)

qt_add_qml_module(appwikipedia_qt
    URI wikipedia_qt
    VERSION 1.0
    QML_FILES
        qml/Main.qml
        qml/components/searchbar/SearchBar.qml
        qml/components/Header.qml
        qml/components/Sidebar.qml
        qml/components/MainContent.qml
    RESOURCES
        icons/home.jpg
        icons/history.jpg
        icons/bookmark.jpg
        icons/safes.jpg
        icons/location.png
)

set_target_properties(appwikipedia_qt PROPERTIES
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

target_link_libraries(appwikipedia_qt
    PRIVATE Qt6::Quick
)

include(GNUInstallDirs)
install(TARGETS appwikipedia_qt
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
