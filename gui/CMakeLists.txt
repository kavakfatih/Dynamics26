set(FEMCAE_GUI_SOURCES
    main.cpp
    MainWindow.cpp
    MainWindow.h
    ViewportWidget.cpp
    ViewportWidget.h
    MaterialCurveWidget.cpp
    MaterialCurveWidget.h
    GeometryPanel.cpp
    GeometryPanel.h
    PrePostPanel.cpp
    PrePostPanel.h
    ProjectFileMigrator.cpp
    ProjectFileMigrator.h
)

qt_add_executable(femcae_gui MACOSX_BUNDLE ${FEMCAE_GUI_SOURCES})
target_link_libraries(femcae_gui PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::OpenGLWidgets
    femcae_api
    femcae_geometry
    femcae_meshing
)
target_include_directories(femcae_gui PRIVATE "${PROJECT_SOURCE_DIR}/include")
target_compile_definitions(femcae_gui PRIVATE FEMCAE_APP_VERSION="${PROJECT_VERSION}")

if(FEMCAE_GUI_WITH_VTK)
    find_package(VTK QUIET COMPONENTS
        CommonCore
        CommonDataModel
        FiltersSources
        RenderingCore
        RenderingOpenGL2
        InteractionStyle
        GUISupportQt
    )
    if(VTK_FOUND)
        target_compile_definitions(femcae_gui PRIVATE FEMCAE_GUI_HAS_VTK=1)
        target_link_libraries(femcae_gui PRIVATE ${VTK_LIBRARIES})
        vtk_module_autoinit(TARGETS femcae_gui MODULES ${VTK_LIBRARIES})
        message(STATUS "FEMCAE GUI: VTK viewport enabled")
    else()
        message(WARNING "FEMCAE GUI: VTK bulunamadi; Qt placeholder viewport derlenecek.")
    endif()
endif()

set_target_properties(femcae_gui PROPERTIES
    MACOSX_BUNDLE TRUE
    MACOSX_BUNDLE_BUNDLE_NAME "FEMCAE"
    MACOSX_BUNDLE_GUI_IDENTIFIER "org.femcae.app"
    MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
    MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
    OUTPUT_NAME "FEMCAE"
    INSTALL_RPATH "@executable_path/../Frameworks"
)

install(TARGETS femcae_gui
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Qt frameworks/plugins are deployed with Qt's supported deploy helper. On macOS
# BundleUtilities runs afterwards to make VTK/OCCT/ARPACK/gfortran dependencies
# self-contained and relocatable inside FEMCAE.app.
qt_generate_deploy_app_script(
    TARGET femcae_gui
    OUTPUT_SCRIPT FEMCAE_QT_DEPLOY_SCRIPT
    NO_TRANSLATIONS
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${FEMCAE_QT_DEPLOY_SCRIPT})

if(APPLE)
    # libfemcae is intentionally copied into the app before fixup_bundle so the
    # app never depends on a sibling install-prefix lib directory.
    install(TARGETS femcae_api LIBRARY DESTINATION "FEMCAE.app/Contents/Frameworks")
    set(FEMCAE_BUNDLE_SEARCH_DIRS "${CMAKE_BINARY_DIR};${CMAKE_BINARY_DIR}/gui")
    foreach(_prefix IN LISTS CMAKE_PREFIX_PATH)
        list(APPEND FEMCAE_BUNDLE_SEARCH_DIRS "${_prefix}/lib" "${_prefix}/lib64" "${_prefix}/Frameworks")
    endforeach()
    if(FEMCAE_ARPACK_LIBRARY)
        get_filename_component(_arpack_dir "${FEMCAE_ARPACK_LIBRARY}" DIRECTORY)
        list(APPEND FEMCAE_BUNDLE_SEARCH_DIRS "${_arpack_dir}")
    endif()
    list(REMOVE_DUPLICATES FEMCAE_BUNDLE_SEARCH_DIRS)
    configure_file(
        "${PROJECT_SOURCE_DIR}/cmake/FemcaeFixupBundle.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/FemcaeFixupBundle.cmake"
        @ONLY
    )
    install(SCRIPT "${CMAKE_CURRENT_BINARY_DIR}/FemcaeFixupBundle.cmake")
endif()

if(BUILD_TESTING)
    add_executable(femcae_project_migration_test
        ProjectFileMigrator.cpp
        ProjectFileMigrator.h
        "${PROJECT_SOURCE_DIR}/tests/gui/test_project_migration.cpp"
    )
    target_include_directories(femcae_project_migration_test PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
    target_link_libraries(femcae_project_migration_test PRIVATE Qt6::Core)
    add_test(NAME gui.project_schema_migration COMMAND femcae_project_migration_test)
    set_tests_properties(gui.project_schema_migration PROPERTIES LABELS "gui;migration;release")
endif()
