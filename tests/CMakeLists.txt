cmake_minimum_required(VERSION 3.16)

project(GlobalStateTests)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/../SearchBarModule
)

find_package(Qt6 REQUIRED COMPONENTS Core Test)

# Add the test executable
qt_add_executable(GlobalStateTest
    GlobalStateTest.cpp
    ${CMAKE_SOURCE_DIR}/src/GlobalState.cpp
)

# Link Qt Test
qt_add_qml_module(GlobalStateTest
    URI wikipedia_qt
    VERSION 1.0
)

target_link_libraries(GlobalStateTest PRIVATE 
    Qt::Core 
    Qt::Network 
    Qt::Test
)

# Include the source directory for headers
target_include_directories(GlobalStateTest PRIVATE ${CMAKE_SOURCE_DIR}/src)

# Add test executables
qt_add_executable(test_wikipedia_client
    test_wikipedia_client.cpp
    ${CMAKE_SOURCE_DIR}/src/wikipedia_client/wikipedia_client.cpp
)

qt_add_executable(simple_test
    simple_test.cpp
    ${CMAKE_SOURCE_DIR}/src/wikipedia_client/wikipedia_client.cpp
)

qt_add_executable(verbose_test
    verbose_test.cpp
    ${CMAKE_SOURCE_DIR}/src/wikipedia_client/wikipedia_client.cpp
)

qt_add_executable(basic_test
    basic_test.cpp
)

target_link_libraries(test_wikipedia_client PRIVATE 
    Qt::Core 
    Qt::Network 
    Qt::Test
)

target_link_libraries(simple_test PRIVATE 
    Qt::Core 
    Qt::Network
)

target_link_libraries(verbose_test PRIVATE 
    Qt::Core 
    Qt::Network
)

target_link_libraries(basic_test PRIVATE 
    Qt::Core 
    Qt::Network
)

# Enable testing
enable_testing()

# Add integration test
qt_add_executable(integration_test
    test_integration.cpp
    ${CMAKE_SOURCE_DIR}/src/GlobalState.cpp
    ${CMAKE_SOURCE_DIR}/SearchBarModule/SearchBarModel.cpp
    ${CMAKE_SOURCE_DIR}/src/wikipedia_client/wikipedia_client.cpp
)

target_link_libraries(integration_test PRIVATE Qt::Core Qt::Network Qt::Quick)

# Link the required Qt modules
qt_standard_project_setup()

# Enable testing
enable_testing()

# Add the test to CTest
add_test(NAME GlobalStateTest COMMAND GlobalStateTest)
add_test(NAME IntegrationTest COMMAND integration_test)
add_test(NAME wikipedia_client_tests COMMAND test_wikipedia_client)

# Add new test executable
qt_add_executable(test_search_display 
    test_search_display.cpp
    ${CMAKE_SOURCE_DIR}/src/GlobalState.cpp
    ${CMAKE_SOURCE_DIR}/SearchBarModule/SearchBarModel.cpp
    ${CMAKE_SOURCE_DIR}/src/wikipedia_client/wikipedia_client.cpp
)

# Link the test executable with the required libraries and source files
target_link_libraries(test_search_display PRIVATE
    Qt::Core
    Qt::Network
    Qt::Quick
    Qt::Test
)



# Add the test to CTest
add_test(NAME test_search_display COMMAND test_search_display)

# Add sidebar layout test
qt_add_executable(test_sidebar_layout
    test_sidebar_layout.cpp
    ${CMAKE_SOURCE_DIR}/SidebarModule/SidebarModel.cpp
)

target_link_libraries(test_sidebar_layout PRIVATE
    Qt::Core
    Qt::Quick
    Qt::Test
)

# Add the test to CTest
add_test(NAME test_sidebar_layout COMMAND test_sidebar_layout)

