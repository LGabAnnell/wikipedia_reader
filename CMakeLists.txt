cmake_minimum_required(VERSION 3.16)

project(wikipedia_qt VERSION 0.1 LANGUAGES CXX)

option(BUILD_TESTING "Build tests" OFF)
option(WIKIPEDIA_FLATPAK "Install app modules using Qt from the Flatpak runtime" OFF)
option(WIKIPEDIA_BUNDLE_QT "Deploy Qt libraries and plugins into Linux installs" OFF)

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
    src/wikipedia_summary_client.cpp
    src/wikipedia_summary_client.h
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
    elseif (WIKIPEDIA_BUNDLE_QT)
        # The Linux Qt deployment helper needs patchelf to set RPATH entries
        # on copied Qt plugins that were built without an existing RPATH.
        find_program(QT_DEPLOY_PATCHELF_EXECUTABLE patchelf)
        if (NOT QT_DEPLOY_PATCHELF_EXECUTABLE)
            message(FATAL_ERROR "patchelf is required for Linux Qt deployment")
        endif()
        set(QT_DEPLOY_USE_PATCHELF ON)
        qt_generate_deploy_qml_app_script(
            TARGET appwikipedia_qt
            OUTPUT_SCRIPT wikipedia_deploy_script
        )
        install(SCRIPT ${wikipedia_deploy_script})

        # Qt's deployment helper finds QML plugins from imports, but it cannot
        # infer the platform integration plugin that QGuiApplication loads
        # before QML starts. Deploy XCB for normal desktop sessions and the
        # offscreen integration used by headless smoke tests. Scan each plugin
        # and its dependencies through Qt's deployment helper, then place the
        # plugins beneath the directory written into the installed qt.conf.
        if (IS_ABSOLUTE "${QT6_INSTALL_PLUGINS}")
            set(wikipedia_qt_plugins_dir "${QT6_INSTALL_PLUGINS}")
        else()
            set(wikipedia_qt_plugins_dir "${QT6_INSTALL_PREFIX}/${QT6_INSTALL_PLUGINS}")
        endif()
        if (NOT IS_DIRECTORY "${wikipedia_qt_plugins_dir}")
            message(FATAL_ERROR "Unable to locate the Qt platform plugin directory")
        endif()
        if (QT_DEPLOY_PLUGINS_DIR)
            set(wikipedia_deploy_plugins_dir "${QT_DEPLOY_PLUGINS_DIR}")
        elseif (IS_ABSOLUTE "${QT6_INSTALL_PLUGINS}")
            file(RELATIVE_PATH wikipedia_deploy_plugins_dir
                "${QT6_INSTALL_PREFIX}" "${QT6_INSTALL_PLUGINS}")
        else()
            set(wikipedia_deploy_plugins_dir "${QT6_INSTALL_PLUGINS}")
        endif()
        if (QT_DEPLOY_LIB_DIR)
            set(wikipedia_deploy_lib_dir "${QT_DEPLOY_LIB_DIR}")
        else()
            set(wikipedia_deploy_lib_dir "${CMAKE_INSTALL_LIBDIR}")
        endif()
        file(RELATIVE_PATH wikipedia_qpa_rel_lib
            "/${wikipedia_deploy_plugins_dir}/platforms"
            "/${wikipedia_deploy_lib_dir}")
        set(wikipedia_qpa_plugins "")
        foreach (plugin_name IN ITEMS libqxcb.so libqoffscreen.so)
            set(plugin_path "${wikipedia_qt_plugins_dir}/platforms/${plugin_name}")
            if (EXISTS "${plugin_path}")
                list(APPEND wikipedia_qpa_plugins "${plugin_path}")
            endif()
        endforeach()
        if (NOT EXISTS "${wikipedia_qt_plugins_dir}/platforms/libqxcb.so"
            OR NOT EXISTS "${wikipedia_qt_plugins_dir}/platforms/libqoffscreen.so")
            message(FATAL_ERROR "Qt must provide the XCB and offscreen platform plugins")
        endif()
        string(REPLACE ";" "\" \"" wikipedia_qpa_plugins_for_script "${wikipedia_qpa_plugins}")
        set(wikipedia_qpa_install_code [=[
include("@CMAKE_BINARY_DIR@/.qt/QtDeploySupport.cmake")
set(wikipedia_qpa_plugins "@wikipedia_qpa_plugins_for_script@")
qt6_deploy_runtime_dependencies(
    EXECUTABLE "${QT_DEPLOY_PREFIX}/${QT_DEPLOY_BIN_DIR}/appwikipedia_qt"
    ADDITIONAL_MODULES ${wikipedia_qpa_plugins}
)
set(wikipedia_qpa_destination "${QT_DEPLOY_PREFIX}/${QT_DEPLOY_PLUGINS_DIR}/platforms")
file(MAKE_DIRECTORY "${wikipedia_qpa_destination}")
foreach(wikipedia_qpa_plugin IN LISTS wikipedia_qpa_plugins)
    get_filename_component(wikipedia_qpa_filename "${wikipedia_qpa_plugin}" NAME)
    file(INSTALL DESTINATION "${wikipedia_qpa_destination}"
        TYPE SHARED_LIBRARY FILES "${wikipedia_qpa_plugin}")
    execute_process(
        COMMAND "@QT_DEPLOY_PATCHELF_EXECUTABLE@" --set-rpath
            "$ORIGIN/@wikipedia_qpa_rel_lib@"
            "${wikipedia_qpa_destination}/${wikipedia_qpa_filename}"
        RESULT_VARIABLE wikipedia_qpa_patch_result
    )
    if (NOT wikipedia_qpa_patch_result EQUAL 0)
        message(FATAL_ERROR "Unable to set the relocated RPATH for ${wikipedia_qpa_filename}")
    endif()
endforeach()
]=])
        string(CONFIGURE "${wikipedia_qpa_install_code}" wikipedia_qpa_install_code @ONLY)
        install(CODE "${wikipedia_qpa_install_code}")
    else()
        # Install only app modules and libraries; the target system supplies Qt.
        include(packaging/Flatpak.cmake)
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
