cmake_minimum_required(VERSION 3.16)

project(wikipedia_qt VERSION 0.1 LANGUAGES CXX)

option(BUILD_TESTING "Build tests" OFF)
option(WIKIPEDIA_FLATPAK "Install app modules using Qt from the Flatpak runtime" OFF)

set(WIKIPEDIA_APP_ID "io.github.lgabannell.wikipedia_reader")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(QT_QML_GENERATE_QMLLS_INI ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY bin)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

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

# Shared backend implementation and Qt meta-objects, used by the app and tests.
add_library(wikipedia_qt_core SHARED
    src/wikipedia_search_client.cpp
    src/wikipedia_search_client.h
    src/wikipedia_page_client.cpp
    src/wikipedia_page_client.h
    src/wikipedia_featured_client.cpp
    src/wikipedia_featured_client.h
    src/wikipedia_home_client.cpp
    src/wikipedia_home_client.h
    src/wikipedia_models.h
    src/wikipedia_network_access_manager.h
    src/wikipedia_request_context.h
    src/state/SettingsState.cpp
    src/state/SettingsState.h
    src/state/ImageSelectionState.cpp
    src/state/ImageSelectionState.h
    src/state/ArticleState.cpp
    src/state/ArticleState.h
    src/state/SearchState.cpp
    src/state/SearchState.h
    src/state/ClipboardHelper.cpp
    src/state/ClipboardHelper.h
    src/state/HistoryState.cpp
    src/state/HistoryState.h
    src/state/db/HistoryDatabase.cpp
    src/state/db/HistoryDatabase.h
    src/html_processor.cpp
    src/html_processor.h
)

set_target_properties(wikipedia_qt_core PROPERTIES
    AUTOMOC ON
    POSITION_INDEPENDENT_CODE ON
)

target_compile_definitions(wikipedia_qt_core PUBLIC
    WIKIPEDIA_READER_VERSION="${PROJECT_VERSION}"
)

target_include_directories(wikipedia_qt_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/state
    ${CMAKE_CURRENT_SOURCE_DIR}/src/state/db
)

target_link_libraries(wikipedia_qt_core PUBLIC
    Qt::Core
    Qt::Network
    Qt::Sql
    Qt::Gui
    Qt::Qml
    Qt::Svg
    tinyxml2::tinyxml2
    PRIVATE lexbor::lexbor
)

if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    # Prevent executable copy relocations from moving backend static storage
    # and meta-objects out of the shared core (PIE alone does not prevent them).
    target_compile_options(wikipedia_qt_core INTERFACE
        "$<$<CXX_COMPILER_ID:GNU,Clang>:-fPIC>"
    )
endif()

# Make the backend meta-objects available to the QML foreign-type registrar.
qt_extract_metatypes(wikipedia_qt_core)

# Navigation stays separate so the backend does not depend on Qt Quick.
add_library(wikipedia_runtime STATIC
    src/state/NavigationState.cpp
    src/state/NavigationState.h
)

target_include_directories(wikipedia_runtime PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/state
)

target_link_libraries(wikipedia_runtime PUBLIC Qt::Quick)

add_subdirectory(src/modules/SearchBarModule)
add_subdirectory(src/modules/SidebarModule)
add_subdirectory(src/modules/HistoryModule)
add_subdirectory(src/modules/ContentDisplayModule)
add_subdirectory(src/modules/HeaderModule)
add_subdirectory(src/modules/HomeModule)
add_subdirectory(src/modules/ImageDisplayModule)
add_subdirectory(src/modules/SectionModule)
add_subdirectory(src)

include(GNUInstallDirs)
if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    # Resolve the core relative to the executable even after relocating an install.
    file(RELATIVE_PATH wikipedia_install_bin_to_lib
        "${CMAKE_INSTALL_FULL_BINDIR}" "${CMAKE_INSTALL_FULL_LIBDIR}")
    set_target_properties(appwikipedia_qt PROPERTIES
        INSTALL_RPATH "$ORIGIN/${wikipedia_install_bin_to_lib}"
    )
    set_target_properties(wikipedia_qt_core PROPERTIES INSTALL_RPATH "$ORIGIN")
endif()

install(TARGETS appwikipedia_qt wikipedia_qt_core
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    if (WIKIPEDIA_FLATPAK)
        include(packaging/Flatpak.cmake)
    else()
        qt_generate_deploy_qml_app_script(
            TARGET appwikipedia_qt
            OUTPUT_SCRIPT wikipedia_deploy_script
        )
        install(SCRIPT ${wikipedia_deploy_script})
    endif()

    install(FILES icons/app_icon.svg
        DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps
        RENAME ${WIKIPEDIA_APP_ID}.svg
    )
    install(FILES packaging/${WIKIPEDIA_APP_ID}.desktop
        DESTINATION ${CMAKE_INSTALL_DATADIR}/applications
    )
    install(FILES packaging/${WIKIPEDIA_APP_ID}.metainfo.xml
        DESTINATION ${CMAKE_INSTALL_DATADIR}/metainfo
    )
    install(FILES LICENSE
        DESTINATION ${CMAKE_INSTALL_DATADIR}/licenses/${WIKIPEDIA_APP_ID}
    )
endif()

if (BUILD_TESTING)
    enable_testing()
    add_subdirectory(tests)
endif()
