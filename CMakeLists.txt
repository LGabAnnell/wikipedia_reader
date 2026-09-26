cmake_minimum_required(VERSION 3.16)

project(wikipedia_qt VERSION 0.1 LANGUAGES CXX)

option(BUILD_TESTING "Build tests" OFF)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(QT_QML_GENERATE_QMLLS_INI ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY bin)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_compile_definitions(
    $<$<CONFIG:Debug>:DEBUG>
    $<$<CONFIG:Debug>:QT_QML_DEBUG>
)

set(QML_DIRS
    "src"
    "src/modules/SearchBarModule"
    "src/modules/SidebarModule"
    "src/modules/HistoryModule"
    "src/modules/HeaderModule"
    "src/modules/HomeModule"
    "src/modules/ContentDisplayModule"
    "src/modules/ImageDisplayModule"
    "src/modules/SectionModule"
)

# Convert the list to a string with semicolons
list(JOIN QML_DIRS ";" QML_IMPORT_PATH_STR)

# Set as a cached variable
set(QML_IMPORT_PATH "${QML_IMPORT_PATH_STR}")

find_package(Qt6 REQUIRED COMPONENTS Quick Core Network Sql Gui Svg Widgets SvgWidgets)
if (BUILD_TESTING)
    find_package(Qt6 REQUIRED COMPONENTS Test QuickTest)
endif()
find_package(tinyxml2 REQUIRED)
find_package(lexbor 3.0 CONFIG REQUIRED)

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
    src/modules/ImageDisplayModule
    src/modules/SectionModule
)

add_library(wikipedia_runtime STATIC
    src/state/GlobalState.cpp
    src/state/HistoryState.cpp
    src/state/NavigationState.cpp
    src/state/db/HistoryDatabase.cpp
    src/html_processor.cpp
)

target_include_directories(wikipedia_runtime PUBLIC
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/state
    ${CMAKE_SOURCE_DIR}/src/state/db
)

target_link_libraries(wikipedia_runtime PUBLIC
    Qt::Core
    Qt::Network
    Qt::Sql
    Qt::Gui
    Qt::Qml
    Qt::Quick
    Qt::Svg
)
target_link_libraries(wikipedia_runtime PRIVATE lexbor::lexbor)

qt_add_executable(appwikipedia_qt
    src/main.cpp
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
add_subdirectory(src/modules/ImageDisplayModule)
add_subdirectory(src/modules/SectionModule)
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
    ImageDisplayModule
    SectionModule
    wikipedia_runtime
)

include(GNUInstallDirs)
install(TARGETS appwikipedia_qt
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if (BUILD_TESTING)
    enable_testing()
    add_subdirectory(tests)
endif()
