cmake_minimum_required(VERSION 3.16)

project(GlobalStateTests)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Core Test Quick Network Sql Qml Gui Svg)

qt_standard_project_setup()

# ---------------------------------------------------------------------------
# GlobalStateTest
# ---------------------------------------------------------------------------
qt_add_executable(GlobalStateTest
    GlobalStateTest.cpp
)

target_link_libraries(GlobalStateTest PRIVATE
    wikipedia_qt_core
    Qt::Test
)

add_test(NAME GlobalStateTest COMMAND GlobalStateTest)

# ---------------------------------------------------------------------------
# test_search_display (network integration test, guarded by RUN_NETWORK_TESTS)
# ---------------------------------------------------------------------------
qt_add_executable(test_search_display
    test_search_display.cpp
    ${CMAKE_SOURCE_DIR}/src/modules/SearchBarModule/SearchBarModel.cpp
)

target_link_libraries(test_search_display PRIVATE wikipedia_qt_core)

target_include_directories(test_search_display PRIVATE
    ${CMAKE_SOURCE_DIR}/src/modules/SearchBarModule
)

add_test(NAME test_search_display COMMAND test_search_display)

# ---------------------------------------------------------------------------
# test_sidebar_layout (QML layout test, headless)
# ---------------------------------------------------------------------------
qt_add_executable(test_sidebar_layout
    test_sidebar_layout.cpp
    ${CMAKE_SOURCE_DIR}/src/modules/SidebarModule/SidebarModel.cpp
)

target_link_libraries(test_sidebar_layout PRIVATE
    wikipedia_qt_core
    Qt::Test
    Qt::Quick
)

target_include_directories(test_sidebar_layout PRIVATE
    ${CMAKE_SOURCE_DIR}/src/modules/SidebarModule
)

add_test(NAME test_sidebar_layout COMMAND test_sidebar_layout
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/src/modules
)
set_tests_properties(test_sidebar_layout PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QML_IMPORT_PATH=${CMAKE_BINARY_DIR}"
)

# ---------------------------------------------------------------------------
# HtmlProcessorTest
# ---------------------------------------------------------------------------
qt_add_executable(HtmlProcessorTest
    HtmlProcessorTest.cpp
)

target_link_libraries(HtmlProcessorTest PRIVATE
    wikipedia_qt_core
    Qt::Test
)

add_test(NAME HtmlProcessorTest COMMAND HtmlProcessorTest)

# ---------------------------------------------------------------------------
# HistoryDatabaseTest
# ---------------------------------------------------------------------------
qt_add_executable(HistoryDatabaseTest
    HistoryDatabaseTest.cpp
)

target_link_libraries(HistoryDatabaseTest PRIVATE
    wikipedia_qt_core
    Qt::Test
)

add_test(NAME HistoryDatabaseTest COMMAND HistoryDatabaseTest)

# ---------------------------------------------------------------------------
# WikipediaSearchClientTest
# ---------------------------------------------------------------------------
qt_add_executable(WikipediaSearchClientTest
    WikipediaSearchClientTest.cpp
)

target_link_libraries(WikipediaSearchClientTest PRIVATE
    wikipedia_qt_core
    Qt::Test
)

add_test(NAME WikipediaSearchClientTest COMMAND WikipediaSearchClientTest)

# ---------------------------------------------------------------------------
# WikipediaPageClientTest
# ---------------------------------------------------------------------------
qt_add_executable(WikipediaPageClientTest
    WikipediaPageClientTest.cpp
)

target_link_libraries(WikipediaPageClientTest PRIVATE
    wikipedia_qt_core
    Qt::Test
)

add_test(NAME WikipediaPageClientTest COMMAND WikipediaPageClientTest)

# ---------------------------------------------------------------------------
# WikipediaFeaturedClientTest
# ---------------------------------------------------------------------------
qt_add_executable(WikipediaFeaturedClientTest
    WikipediaFeaturedClientTest.cpp
)

target_link_libraries(WikipediaFeaturedClientTest PRIVATE
    wikipedia_qt_core
    Qt::Test
)

add_test(NAME WikipediaFeaturedClientTest COMMAND WikipediaFeaturedClientTest)

# ---------------------------------------------------------------------------
# WikipediaHomeClientTest
# ---------------------------------------------------------------------------
qt_add_executable(WikipediaHomeClientTest
    WikipediaHomeClientTest.cpp
)

target_link_libraries(WikipediaHomeClientTest PRIVATE
    wikipedia_qt_core
    Qt::Test
)

add_test(NAME WikipediaHomeClientTest COMMAND WikipediaHomeClientTest)

# ---------------------------------------------------------------------------
# QML Quick Test harness
# ---------------------------------------------------------------------------
add_subdirectory(qml)
