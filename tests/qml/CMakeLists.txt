qt_add_executable(QmlTests
    qml_test_main.cpp
    support/FakeNetworkAccessManager.cpp
    support/FakeNetworkAccessManager.h
    support/FakeNetworkReply.cpp
    support/FakeNetworkReply.h
    support/QmlTestSupport.cpp
    support/QmlTestSupport.h
)

target_include_directories(QmlTests PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/state
    ${CMAKE_SOURCE_DIR}/src/modules/HeaderModule
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_definitions(QmlTests PRIVATE
    QML_TEST_IMPORT_PATH="${CMAKE_BINARY_DIR}"
    QML_TEST_FIXTURE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/fixtures"
)

qt_add_resources(QmlTests "qml_test_icons"
    PREFIX "/icons"
    BASE "${CMAKE_SOURCE_DIR}/icons"
    FILES
        ${CMAKE_SOURCE_DIR}/icons/home.svg
        ${CMAKE_SOURCE_DIR}/icons/history.svg
        ${CMAKE_SOURCE_DIR}/icons/back.svg
        ${CMAKE_SOURCE_DIR}/icons/search.svg
)

target_link_libraries(QmlTests PRIVATE
    Qt::QuickTest
    Qt::Qml
    Qt::Quick
    Qt::Network
    Qt::Sql
    Qt::Core
    Qt::Gui
    Qt::Svg
    Qt::SvgWidgets
    wikipedia_qt
    SearchBarModule
    SidebarModule
    HistoryModule
    HeaderModule
    HomeModule
    ContentDisplayModule
    ImageDisplayModule
    SectionModule
    wikipedia_runtime
    tinyxml2
)

file(GLOB QML_TEST_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/tst_*.qml")
foreach(qml_test_file IN LISTS QML_TEST_FILES)
    get_filename_component(qml_test_name "${qml_test_file}" NAME_WE)
    add_test(NAME "QmlTests.${qml_test_name}" COMMAND QmlTests -input "${qml_test_file}")
    set_tests_properties("QmlTests.${qml_test_name}" PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QML_IMPORT_PATH=${CMAKE_BINARY_DIR}"
    )
endforeach()
