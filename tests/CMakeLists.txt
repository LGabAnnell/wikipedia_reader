cmake_minimum_required(VERSION 3.16)

project(StateTests)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Core Test Quick Network Sql Qml Gui Svg)

qt_standard_project_setup()

add_library(wikipedia_test_network STATIC
    qml/support/FakeNetworkAccessManager.cpp
    qml/support/FakeNetworkAccessManager.h
    qml/support/FakeNetworkReply.cpp
    qml/support/FakeNetworkReply.h
)
target_link_libraries(wikipedia_test_network PUBLIC Qt::Network Qt::Qml)
target_include_directories(wikipedia_test_network PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/qml)
target_compile_definitions(wikipedia_test_network PRIVATE
    QML_TEST_FIXTURE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/qml/fixtures"
)

foreach(state_test ArticleStateTest SearchStateTest SettingsStateTest ImageSelectionStateTest)
    qt_add_executable(${state_test} ${state_test}.cpp)
    target_link_libraries(${state_test} PRIVATE wikipedia_qt_core Qt::Test wikipedia_test_network)
    add_test(NAME ${state_test} COMMAND ${state_test})
endforeach()

qt_add_executable(ContentDisplayModelTest ContentDisplayModelTest.cpp)
target_include_directories(ContentDisplayModelTest PRIVATE
    ${CMAKE_SOURCE_DIR}/src/modules/ContentDisplayModule
)
target_link_libraries(ContentDisplayModelTest PRIVATE ContentDisplayModule wikipedia_qt_core Qt::Test)
add_test(NAME ContentDisplayModelTest COMMAND ContentDisplayModelTest)
set_tests_properties(ContentDisplayModelTest PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

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
    lexbor::lexbor
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
