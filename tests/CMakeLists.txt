cmake_minimum_required(VERSION 3.16)

project(GlobalStateTests)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Core Test Quick Network Sql Qml Gui)

qt_standard_project_setup()

include_directories(
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/state
    ${CMAKE_SOURCE_DIR}/src/state/db
    ${CMAKE_SOURCE_DIR}/src/modules/SearchBarModule
    ${CMAKE_SOURCE_DIR}/src/modules/SidebarModule
)

# ---------------------------------------------------------------------------
# GlobalStateTest
# ---------------------------------------------------------------------------
qt_add_executable(GlobalStateTest
    GlobalStateTest.cpp
    ${CMAKE_SOURCE_DIR}/src/state/GlobalState.cpp
    ${CMAKE_SOURCE_DIR}/src/state/HistoryState.cpp
    ${CMAKE_SOURCE_DIR}/src/state/db/HistoryDatabase.cpp
    ${CMAKE_SOURCE_DIR}/src/wikipedia_search_client.cpp
    ${CMAKE_SOURCE_DIR}/src/wikipedia_page_client.cpp
    ${CMAKE_SOURCE_DIR}/src/wikipedia_featured_client.cpp
    ${CMAKE_SOURCE_DIR}/src/wikipedia_home_client.cpp
    ${CMAKE_SOURCE_DIR}/src/html_processor.cpp
    ${CMAKE_SOURCE_DIR}/src/wikipedia_models.h
)

target_link_libraries(GlobalStateTest PRIVATE
    Qt::Core
    Qt::Network
    Qt::Sql
    Qt::Test
    Qt::Qml
    Qt::Gui
    tinyxml2
)

target_include_directories(GlobalStateTest PRIVATE ${CMAKE_SOURCE_DIR}/src)

add_test(NAME GlobalStateTest COMMAND GlobalStateTest)

# ---------------------------------------------------------------------------
# test_search_display (network integration test, guarded by RUN_NETWORK_TESTS)
# ---------------------------------------------------------------------------
qt_add_executable(test_search_display
    test_search_display.cpp
    ${CMAKE_SOURCE_DIR}/src/state/GlobalState.cpp
    ${CMAKE_SOURCE_DIR}/src/state/HistoryState.cpp
    ${CMAKE_SOURCE_DIR}/src/state/db/HistoryDatabase.cpp
    ${CMAKE_SOURCE_DIR}/src/modules/SearchBarModule/SearchBarModel.cpp
    ${CMAKE_SOURCE_DIR}/src/wikipedia_search_client.cpp
    ${CMAKE_SOURCE_DIR}/src/wikipedia_page_client.cpp
    ${CMAKE_SOURCE_DIR}/src/wikipedia_featured_client.cpp
    ${CMAKE_SOURCE_DIR}/src/wikipedia_home_client.cpp
    ${CMAKE_SOURCE_DIR}/src/html_processor.cpp
    ${CMAKE_SOURCE_DIR}/src/wikipedia_models.h
)

target_link_libraries(test_search_display PRIVATE
    Qt::Core
    Qt::Network
    Qt::Quick
    Qt::Sql
    Qt::Test
    Qt::Qml
    Qt::Gui
    tinyxml2
)

add_test(NAME test_search_display COMMAND test_search_display)

# ---------------------------------------------------------------------------
# test_sidebar_layout (QML layout test, headless)
# ---------------------------------------------------------------------------
qt_add_executable(test_sidebar_layout
    test_sidebar_layout.cpp
    ${CMAKE_SOURCE_DIR}/src/modules/SidebarModule/SidebarModel.cpp
    ${CMAKE_SOURCE_DIR}/src/wikipedia_models.h
)

target_link_libraries(test_sidebar_layout PRIVATE
    Qt::Core
    Qt::Quick
    Qt::Test
    Qt::Qml
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
    ${CMAKE_SOURCE_DIR}/src/html_processor.cpp
)

target_link_libraries(HtmlProcessorTest PRIVATE
    Qt::Core
    Qt::Test
    Qt::Gui
    tinyxml2
)

target_include_directories(HtmlProcessorTest PRIVATE ${CMAKE_SOURCE_DIR}/src)

add_test(NAME HtmlProcessorTest COMMAND HtmlProcessorTest)

# ---------------------------------------------------------------------------
# HistoryDatabaseTest
# ---------------------------------------------------------------------------
qt_add_executable(HistoryDatabaseTest
    HistoryDatabaseTest.cpp
    ${CMAKE_SOURCE_DIR}/src/state/db/HistoryDatabase.cpp
    ${CMAKE_SOURCE_DIR}/src/wikipedia_models.h
)

target_link_libraries(HistoryDatabaseTest PRIVATE
    Qt::Core
    Qt::Test
    Qt::Sql
    Qt::Qml
)

target_include_directories(HistoryDatabaseTest PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/state/db
)

add_test(NAME HistoryDatabaseTest COMMAND HistoryDatabaseTest)

# ---------------------------------------------------------------------------
# WikipediaSearchClientTest
# ---------------------------------------------------------------------------
qt_add_executable(WikipediaSearchClientTest
    WikipediaSearchClientTest.cpp
    ${CMAKE_SOURCE_DIR}/src/wikipedia_search_client.cpp
    ${CMAKE_SOURCE_DIR}/src/wikipedia_models.h
)

target_link_libraries(WikipediaSearchClientTest PRIVATE
    Qt::Core
    Qt::Test
    Qt::Network
    Qt::Qml
)

target_include_directories(WikipediaSearchClientTest PRIVATE ${CMAKE_SOURCE_DIR}/src)

add_test(NAME WikipediaSearchClientTest COMMAND WikipediaSearchClientTest)

# ---------------------------------------------------------------------------
# WikipediaPageClientTest
# ---------------------------------------------------------------------------
qt_add_executable(WikipediaPageClientTest
    WikipediaPageClientTest.cpp
    ${CMAKE_SOURCE_DIR}/src/wikipedia_page_client.cpp
    ${CMAKE_SOURCE_DIR}/src/html_processor.cpp
    ${CMAKE_SOURCE_DIR}/src/wikipedia_models.h
)

target_link_libraries(WikipediaPageClientTest PRIVATE
    Qt::Core
    Qt::Test
    Qt::Network
    Qt::Qml
    Qt::Gui
    tinyxml2
)

target_include_directories(WikipediaPageClientTest PRIVATE ${CMAKE_SOURCE_DIR}/src)

add_test(NAME WikipediaPageClientTest COMMAND WikipediaPageClientTest)

# ---------------------------------------------------------------------------
# WikipediaFeaturedClientTest
# ---------------------------------------------------------------------------
qt_add_executable(WikipediaFeaturedClientTest
    WikipediaFeaturedClientTest.cpp
    ${CMAKE_SOURCE_DIR}/src/wikipedia_featured_client.cpp
    ${CMAKE_SOURCE_DIR}/src/wikipedia_models.h
)

target_link_libraries(WikipediaFeaturedClientTest PRIVATE
    Qt::Core
    Qt::Test
    Qt::Network
    Qt::Qml
)

target_include_directories(WikipediaFeaturedClientTest PRIVATE ${CMAKE_SOURCE_DIR}/src)

add_test(NAME WikipediaFeaturedClientTest COMMAND WikipediaFeaturedClientTest)

# ---------------------------------------------------------------------------
# WikipediaHomeClientTest
# ---------------------------------------------------------------------------
qt_add_executable(WikipediaHomeClientTest
    WikipediaHomeClientTest.cpp
    ${CMAKE_SOURCE_DIR}/src/wikipedia_home_client.cpp
    ${CMAKE_SOURCE_DIR}/src/wikipedia_models.h
)

target_link_libraries(WikipediaHomeClientTest PRIVATE
    Qt::Core
    Qt::Test
    Qt::Network
    Qt::Qml
)

target_include_directories(WikipediaHomeClientTest PRIVATE ${CMAKE_SOURCE_DIR}/src)

add_test(NAME WikipediaHomeClientTest COMMAND WikipediaHomeClientTest)

# Enable testing
enable_testing()
