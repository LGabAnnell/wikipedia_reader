# Qt is supplied by org.kde.Platform. Install only this project's QML modules,
# their backing libraries and plugins, without copying the runtime's Qt stack.
foreach(module IN ITEMS
    wikipedia_qt
    SearchBarModule
    SidebarModule
    HistoryModule
    ContentDisplayModule
    HeaderModule
    HomeModule
    ImageDisplayModule
    SectionModule
)
    qt_query_qml_module(${module}
        PLUGIN_TARGET plugin
        TARGET_PATH module_path
        QMLDIR qmldir
    )
    set_target_properties(${module} PROPERTIES INSTALL_RPATH "$ORIGIN")
    install(TARGETS ${module} LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})

    set(module_destination "qml/${module_path}")
    install(FILES ${qmldir} DESTINATION ${module_destination})
    if (plugin)
        file(RELATIVE_PATH plugin_to_lib
            "${CMAKE_INSTALL_PREFIX}/${module_destination}"
            "${CMAKE_INSTALL_FULL_LIBDIR}"
        )
        set_target_properties(${plugin} PROPERTIES
            INSTALL_RPATH "$ORIGIN/${plugin_to_lib}"
        )
        install(TARGETS ${plugin} LIBRARY DESTINATION ${module_destination})
    endif()
endforeach()
