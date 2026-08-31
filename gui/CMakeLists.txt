# Dynamics26 V1.1 GUI kaynak ağacı.
#
#   core/      proje nesne modeli, ağaç adaptörü, semantik CAE ikonları
#   services/  mühendislik durumunun sahibi (geometri / mesh / malzeme / analiz)
#   viewport/  semantik rol tabanlı VTK render katmanı
#   details/   nesne türüne bağlı Details sayfaları
#   shell/     görünür kompozisyonun tek sahibi (pencere, komutlar, paneller)
#   support/   geliştirici araçları (ekran görüntüsü sürücüsü)
set(FEMCAE_GUI_SOURCES
    main.cpp

    core/ProjectTypes.cpp
    core/ProjectTypes.h
    core/ProjectModel.cpp
    core/ProjectModel.h
    core/ProjectTreeModel.cpp
    core/ProjectTreeModel.h
    core/CaeIcons.cpp
    core/CaeIcons.h
    core/ServiceContext.h
    core/UiTheme.cpp
    core/UiTheme.h
    core/DocumentCommandManager.cpp
    core/DocumentCommandManager.h
    core/DependencyEngine.cpp
    core/DependencyEngine.h
    core/SelectionTypes.h
    core/SelectionPolicy.h
    core/SelectionManager.cpp
    core/SelectionManager.h
    core/ScopeReferenceBuilder.h

    commands/DomainCommands.cpp
    commands/DomainCommands.h

    services/GeometryService.cpp
    services/GeometryService.h
    services/MeshService.cpp
    services/MeshService.h
    services/MaterialService.cpp
    services/MaterialService.h
    services/AnalysisService.cpp
    services/AnalysisService.h

    viewport/RenderRoles.cpp
    viewport/RenderRoles.h
    viewport/ViewportNavigation.h
    viewport/ViewportInputRouter.cpp
    viewport/ViewportInputRouter.h
    viewport/ViewportCameraController.cpp
    viewport/ViewportCameraController.h
    viewport/Dynamics26InteractorStyle.cpp
    viewport/Dynamics26InteractorStyle.h
    viewport/GeometrySelectionScene.h
    viewport/GeometryTopologyScene.h
    viewport/ViewportSelectionController.h
    viewport/ViewportSelectionBridge.cpp
    viewport/ViewportSelectionBridge.h
    viewport/ViewportWidget.cpp
    viewport/ViewportWidget.h

    details/DetailsPage.cpp
    details/DetailsPage.h
    details/GeometryDetails.cpp
    details/GeometryDetails.h
    details/MeshDetails.cpp
    details/MeshDetails.h
    details/MaterialDetails.cpp
    details/MaterialDetails.h
    details/AnalysisDetails.cpp
    details/AnalysisDetails.h
    details/BoundaryConditionDetails.cpp
    details/BoundaryConditionDetails.h
    details/ResultDetails.cpp
    details/ResultDetails.h
    details/ObjectDetails.cpp
    details/ObjectDetails.h

    shell/CommandRegistry.cpp
    shell/CommandRegistry.h
    shell/ProjectNavigator.cpp
    shell/ProjectNavigator.h
    shell/GraphicsWorkspace.cpp
    shell/GraphicsWorkspace.h
    shell/DetailsHost.cpp
    shell/DetailsHost.h
    shell/UtilityWorkspace.cpp
    shell/UtilityWorkspace.h
    shell/EngineeringStatusBar.cpp
    shell/EngineeringStatusBar.h
    shell/SelectionCoordinator.cpp
    shell/SelectionCoordinator.h
    shell/Dynamics26MainWindow.cpp
    shell/Dynamics26MainWindow.h

    support/ScreenshotDriver.cpp
    support/ScreenshotDriver.h
    support/SelfTest.cpp
    support/SelfTest.h

    widgets/MaterialCurveWidget.cpp
    widgets/MaterialCurveWidget.h

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
target_include_directories(femcae_gui PRIVATE
    "${PROJECT_SOURCE_DIR}/include"
    "${CMAKE_CURRENT_SOURCE_DIR}"
)
target_compile_definitions(femcae_gui PRIVATE
    FEMCAE_APP_VERSION="${PROJECT_VERSION}"
    DYNAMICS26_GUI_MILESTONE="1.1.0-alpha.3.3"
    $<$<CONFIG:Debug>:DYNAMICS26_VIEWPORT_INPUT_TRACE=1>
)

if(FEMCAE_GUI_WITH_VTK)
    # RenderingAnnotation: sonuç legend'i (vtkScalarBarActor)
    # FiltersCore: sınır şartı / yük sembolleri (vtkGlyph3D)
    find_package(VTK QUIET COMPONENTS
        CommonCore
        CommonDataModel
        FiltersCore
        FiltersSources
        RenderingCore
        RenderingAnnotation
        RenderingFreeType
        RenderingOpenGL2
        InteractionStyle
        InteractionWidgets
        GUISupportQt
    )
    if(VTK_FOUND)
        target_compile_definitions(femcae_gui PRIVATE FEMCAE_GUI_HAS_VTK=1)

        # vtkCameraOrientationWidget availability is an API capability, not a
        # version-number promise. Compile the exact header/method surface used
        # by ViewportWidget so older or differently packaged VTK builds fall
        # back safely without raising the project-wide VTK minimum.
        include(CheckCXXSourceCompiles)
        include(CMakePushCheckState)
        cmake_push_check_state(RESET)
        set(CMAKE_REQUIRED_LIBRARIES ${VTK_LIBRARIES})
        unset(FEMCAE_VTK_HAS_CAMERA_ORIENTATION_WIDGET CACHE)
        check_cxx_source_compiles([=[
            #include <vtkCameraOrientationWidget.h>
            #include <vtkNew.h>
            #include <vtkRenderer.h>
            int main()
            {
                vtkNew<vtkRenderer> renderer;
                vtkNew<vtkCameraOrientationWidget> widget;
                widget->SetParentRenderer(renderer);
                widget->AnimateOff();
                widget->ShouldResetCameraOn();
                widget->SetEnabled(0);
                return widget->GetEnabled();
            }
        ]=] FEMCAE_VTK_HAS_CAMERA_ORIENTATION_WIDGET)
        cmake_pop_check_state()

        if(FEMCAE_VTK_HAS_CAMERA_ORIENTATION_WIDGET)
            target_compile_definitions(femcae_gui PRIVATE FEMCAE_GUI_HAS_CAMERA_ORIENTATION_WIDGET=1)
            message(STATUS "FEMCAE GUI: vtkCameraOrientationWidget API available")
        else()
            message(STATUS "FEMCAE GUI: vtkCameraOrientationWidget unavailable; axis-triad fallback enabled")
        endif()
        target_link_libraries(femcae_gui PRIVATE ${VTK_LIBRARIES})
        vtk_module_autoinit(TARGETS femcae_gui MODULES ${VTK_LIBRARIES})
        message(STATUS "FEMCAE GUI: VTK viewport enabled")
    else()
        message(WARNING "FEMCAE GUI: VTK bulunamadi; Qt placeholder viewport derlenecek.")
    endif()
endif()

set_target_properties(femcae_gui PROPERTIES
    MACOSX_BUNDLE TRUE
    # Alpha.1'de kullanıcıya görünen bundle adı Dynamics26 olur. OUTPUT_NAME ve
    # bundle-path bilinçli olarak FEMCAE bırakılır; deployment/audit scriptleri
    # birlikte migrate edilmeden internal path/ABI isimleri değiştirilmez.
    MACOSX_BUNDLE_BUNDLE_NAME "Dynamics26"
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

if(APPLE)
    # libfemcae Qt deployment'tan once app bundle icine konur. Boylece
    # macdeployqt executable uzerindeki @rpath/libfemcae.1.dylib bagimliligini
    # ilk taramada cozebilir; kalan VTK/OCCT/ARPACK/gfortran zinciri daha sonra
    # BundleUtilities ile kapatilir.
    install(TARGETS femcae_api LIBRARY DESTINATION "FEMCAE.app/Contents/Frameworks")
endif()

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
    set(FEMCAE_BUNDLE_SEARCH_DIRS "${CMAKE_BINARY_DIR};${CMAKE_BINARY_DIR}/gui")
    foreach(_prefix IN LISTS CMAKE_PREFIX_PATH)
        list(APPEND FEMCAE_BUNDLE_SEARCH_DIRS "${_prefix}/lib" "${_prefix}/lib64" "${_prefix}/Frameworks")
    endforeach()
    if(FEMCAE_ARPACK_LIBRARY)
        get_filename_component(_arpack_dir "${FEMCAE_ARPACK_LIBRARY}" DIRECTORY)
        list(APPEND FEMCAE_BUNDLE_SEARCH_DIRS "${_arpack_dir}")
    endif()

    # gfortran runtime dylib'leri (libgfortran/libquadmath/libgcc_s) Homebrew
    # prefix'inin doğrudan lib dizininde olmak zorunda değildir. CMake Fortran
    # compiler ABI tespiti sırasında gerçek implicit link dizinlerini zaten
    # çıkardığından, compiler sürümüne özel bir Cellar yolu hard-code etmek
    # yerine aynı dizinleri bundle resolver'a aktarırız.
    foreach(_fortran_runtime_dir IN LISTS CMAKE_Fortran_IMPLICIT_LINK_DIRECTORIES)
        if(IS_DIRECTORY "${_fortran_runtime_dir}")
            list(APPEND FEMCAE_BUNDLE_SEARCH_DIRS "${_fortran_runtime_dir}")
        endif()
    endforeach()

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

    qt_add_executable(femcae_selection_manager_test
        core/SelectionTypes.h
        core/SelectionPolicy.h
        core/SelectionManager.cpp
        core/SelectionManager.h
        "${PROJECT_SOURCE_DIR}/tests/gui/test_selection_manager.cpp"
    )
    target_include_directories(femcae_selection_manager_test PRIVATE
        "${PROJECT_SOURCE_DIR}/include"
        "${CMAKE_CURRENT_SOURCE_DIR}"
    )
    target_link_libraries(femcae_selection_manager_test PRIVATE Qt6::Core)
    add_test(NAME gui.selection_manager COMMAND femcae_selection_manager_test)
    set_tests_properties(gui.selection_manager PROPERTIES LABELS "gui;selection;alpha32;alpha33")

    add_executable(femcae_scope_reference_test
        core/SelectionTypes.h
        core/ScopeReferenceBuilder.h
        "${PROJECT_SOURCE_DIR}/tests/gui/test_scope_reference.cpp"
    )
    target_include_directories(femcae_scope_reference_test PRIVATE
        "${PROJECT_SOURCE_DIR}/include"
        "${CMAKE_CURRENT_SOURCE_DIR}"
    )
    target_link_libraries(femcae_scope_reference_test PRIVATE Qt6::Core femcae_geometry)
    add_test(NAME gui.scope_reference COMMAND femcae_scope_reference_test)
    set_tests_properties(gui.scope_reference PROPERTIES LABELS "gui;selection;scope;alpha32;alpha33")

    qt_add_executable(femcae_viewport_selection_input_test
        viewport/ViewportSelectionController.h
        "${PROJECT_SOURCE_DIR}/tests/gui/test_viewport_selection_input.cpp"
    )
    target_include_directories(femcae_viewport_selection_input_test PRIVATE
        "${PROJECT_SOURCE_DIR}/include"
        "${CMAKE_CURRENT_SOURCE_DIR}"
    )
    target_link_libraries(femcae_viewport_selection_input_test PRIVATE Qt6::Core)
    add_test(NAME gui.viewport_selection_input COMMAND femcae_viewport_selection_input_test)
    set_tests_properties(gui.viewport_selection_input PROPERTIES LABELS "gui;selection;alpha32;alpha33")
    # Trusted fast gate femcae_selection_manager_test hedefini açıkça build eder.
    # Dependency sayesinde selection contract testlerinin tamami ayni gate icinde binary olarak üretilir.
    add_dependencies(femcae_selection_manager_test
        femcae_scope_reference_test
        femcae_viewport_selection_input_test
    )

    if(FEMCAE_GUI_WITH_VTK AND VTK_FOUND)
        add_executable(femcae_viewport_navigation_test
            viewport/ViewportInputRouter.cpp
            viewport/ViewportInputRouter.h
            viewport/ViewportNavigation.h
            viewport/ViewportCameraController.cpp
            viewport/ViewportCameraController.h
            viewport/Dynamics26InteractorStyle.cpp
            viewport/Dynamics26InteractorStyle.h
            "${PROJECT_SOURCE_DIR}/tests/gui/test_viewport_navigation.cpp"
        )
        target_include_directories(femcae_viewport_navigation_test PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
        target_link_libraries(femcae_viewport_navigation_test PRIVATE
            Qt6::Core
            Qt6::Gui
            Qt6::Widgets
            ${VTK_LIBRARIES}
        )
        target_compile_definitions(femcae_viewport_navigation_test PRIVATE FEMCAE_GUI_HAS_VTK=1)
        target_compile_definitions(femcae_viewport_navigation_test PRIVATE
            $<$<CONFIG:Debug>:DYNAMICS26_VIEWPORT_INPUT_TRACE=1>
        )
        if(FEMCAE_VTK_HAS_CAMERA_ORIENTATION_WIDGET)
            target_compile_definitions(femcae_viewport_navigation_test
                PRIVATE FEMCAE_GUI_HAS_CAMERA_ORIENTATION_WIDGET=1)
        endif()
        vtk_module_autoinit(TARGETS femcae_viewport_navigation_test MODULES ${VTK_LIBRARIES})
        add_test(NAME gui.viewport_navigation COMMAND femcae_viewport_navigation_test)
        set_tests_properties(gui.viewport_navigation PROPERTIES
            LABELS "gui;viewport;navigation;camera"
            ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
        )

        # Alpha.3.3 gerçek render/pick zinciri: surface cell -> Body/Face,
        # dedicated line -> Edge ve point -> Vertex provenance'i çözer. Display
        # primitive index'leri hiçbir zaman CAD kimliği değildir.
        add_executable(femcae_viewport_selection_test
            viewport/GeometrySelectionScene.h
            viewport/GeometryTopologyScene.h
            viewport/RenderRoles.cpp
            viewport/RenderRoles.h
            "${PROJECT_SOURCE_DIR}/tests/gui/test_viewport_selection.cpp"
        )
        target_include_directories(femcae_viewport_selection_test PRIVATE
            "${PROJECT_SOURCE_DIR}/include"
            "${CMAKE_CURRENT_SOURCE_DIR}"
        )
        target_link_libraries(femcae_viewport_selection_test PRIVATE
            Qt6::Core
            Qt6::Gui
            ${VTK_LIBRARIES}
        )
        vtk_module_autoinit(TARGETS femcae_viewport_selection_test MODULES ${VTK_LIBRARIES})
        add_test(NAME gui.viewport_selection COMMAND femcae_viewport_selection_test)
        set_tests_properties(gui.viewport_selection PROPERTIES
            LABELS "gui;viewport;selection;alpha32;alpha33"
            ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
        )

        # Self-hosted fast gate selection-manager hedefini build ettiğinde VTK
        # picker contract binary'si de mutlaka üretilmiş olur. VTK-OFF hosted
        # lightweight gate bu bağımlılığı doğal olarak görmez.
        add_dependencies(femcae_selection_manager_test femcae_viewport_selection_test)
    endif()
endif()
