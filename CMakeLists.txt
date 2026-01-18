cmake_minimum_required(VERSION 3.16)

project(wikipedia_qt VERSION 0.1 LANGUAGES CXX)

option(BUILD_TESTING "Build tests" OFF)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(QT_QML_GENERATE_QMLLS_INI ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY bin)
set(QML_DIRS
    "src"
    "src/modules/SearchBarModule"
    "src/modules/SidebarModule"
)


if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(DEBUG TRUE)
    message("Debug set???")
endif()

if (DEBUG) 
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DDEBUG -g -DQT_QML_DEBUG")
    message(${CMAKE_CXX_FLAGS})
endif()

# Convert the list to a string with semicolons
list(JOIN QML_DIRS ";" QML_IMPORT_PATH_STR)

# Set as a cached variable
set(QML_IMPORT_PATH "${QML_IMPORT_PATH_STR}" CACHE PATH "QML import paths" FORCE)

find_package(Qt6 REQUIRED COMPONENTS Quick Core Network Test)
qt_standard_project_setup(REQUIRES 6.8)

include_directories(
    src/modules/SearchBarModule
    src/modules/SidebarModule
    src
)

qt_add_executable(appwikipedia_qt
    src/main.cpp
    src/GlobalState.cpp
    src/wikipedia_client.cpp
)

qt_add_resources(appwikipedia_qt
    "icons"
    FILES
        icons/home.jpg
        icons/history.jpg
        icons/bookmark.jpg
        icons/safes.jpg
        icons/location.png
)


add_subdirectory(src/modules/SearchBarModule)
add_subdirectory(src/modules/SidebarModule)
add_subdirectory(src)

set_target_properties(appwikipedia_qt PROPERTIES
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

target_link_libraries(appwikipedia_qt PRIVATE Qt::Quick Qt::Core Qt::Network wikipedia_qt SearchBarModule SidebarModule)

include(GNUInstallDirs)
install(TARGETS appwikipedia_qt
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if (BUILD_TESTING)
    add_subdirectory(tests)
endif()
