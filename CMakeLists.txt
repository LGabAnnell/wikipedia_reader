cmake_minimum_required(VERSION 3.16)

project(wikipedia_qt VERSION 0.1 LANGUAGES CXX)

option(BUILD_TESTING "Build tests" OFF)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(QT_QML_GENERATE_QMLLS_INI ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY bin)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(DEBUG TRUE)
endif()

if (DEBUG) 
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DDEBUG -g")
endif()

if (QT_QML_DEBUG)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DQT_QML_DEBUG")
endif()

set(QML_DIRS
    "src"
    "src/modules/SearchBarModule"
    "src/modules/SidebarModule"
    "src/modules/HistoryModule"
    "src/modules/HeaderModule"
)

# Convert the list to a string with semicolons
list(JOIN QML_DIRS ";" QML_IMPORT_PATH_STR)

# Set as a cached variable
set(QML_IMPORT_PATH "${QML_IMPORT_PATH_STR}")

find_package(Qt6 REQUIRED COMPONENTS Quick Core Network Sql Gui Svg Widgets SvgWidgets Test)
find_package(tinyxml2 REQUIRED)

qt_standard_project_setup(REQUIRES 6.8)

include_directories(
    src
    src/state
    src/state/db
    src/modules/HistoryModule
    src/modules/SearchBarModule
    src/modules/SidebarModule
    src/modules/HeaderModule
    src/modules/HomeModule
)

qt_add_executable(appwikipedia_qt
    src/main.cpp
    src/state/GlobalState.cpp
    src/wikipedia_client.cpp
    src/html_processor.cpp
    src/state/HistoryState.cpp
    src/state/NavigationState.cpp
    src/state/db/HistoryDatabase.cpp
    src/modules/HeaderModule/SvgImageProvider.cpp
)

qt_add_resources(appwikipedia_qt
    "icons"
    FILES
        icons/home.svg
        icons/history.svg
        icons/back.svg
        icons/search.svg
)

qt_add_resources(appwikipedia_qt
    "styles"
    FILES
        styles/table_style.css
)

add_subdirectory(src/modules/SearchBarModule)
add_subdirectory(src/modules/SidebarModule)
add_subdirectory(src/modules/HistoryModule)
add_subdirectory(src/modules/ContentDisplayModule)
add_subdirectory(src/modules/HeaderModule)
add_subdirectory(src/modules/HomeModule)
add_subdirectory(src)

set_target_properties(appwikipedia_qt PROPERTIES
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

target_link_libraries(appwikipedia_qt 
PRIVATE 
    Qt::Quick 
    Qt::Core 
    Qt::Network 
    Qt::Sql
    Qt::Gui
    Qt::Svg
    Qt::Widgets
    Qt::SvgWidgets
    wikipedia_qt 
    SearchBarModule 
    SidebarModule 
    HistoryModule
    ContentDisplayModule
    HeaderModule
    HomeModule
    tinyxml2
)

include(GNUInstallDirs)
install(TARGETS appwikipedia_qt
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if (BUILD_TESTS)
    add_subdirectory(tests)
endif()

