cmake_minimum_required(VERSION 3.25)

project(FEMCAE
    VERSION 1.0.2
    DESCRIPTION "Modern Fortran tabanli nonlinear FEM/CAE cekirdegi"
    LANGUAGES C CXX Fortran
)

include(CTest)
include(GNUInstallDirs)

option(FEMCAE_BUILD_TESTING "Unit/verification/regression testlerini derle" ON)
option(FEMCAE_ENABLE_RUNTIME_CHECKS "Debug derlemelerinde Fortran runtime kontrollerini etkinlestir" ON)
option(FEMCAE_BUILD_GUI "Qt tabanli macOS masaustu GUI'yi derle" OFF)
option(FEMCAE_GUI_WITH_VTK "Qt GUI icinde VTK viewport kullan" ON)
option(FEMCAE_ENABLE_OCCT_GEOMETRY "OCCT STEP/XDE geometry adapter'ini etkinlestir" ON)
option(FEMCAE_REQUIRE_OCCT "OCCT bulunamazsa configure asamasini fail et" OFF)

set(CMAKE_Fortran_STANDARD 2018)
set(CMAKE_Fortran_STANDARD_REQUIRED ON)
set(CMAKE_Fortran_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Projenin desteklenen hedefi macOS/Apple Silicon'dur. Linux yalnizca gelistirme
# ortami icinde kaynak seviyesinde derleme dogrulamasi icin tolere edilir; release
# veya CI hedefi degildir.
if(APPLE)
    if(NOT CMAKE_OSX_ARCHITECTURES)
        set(CMAKE_OSX_ARCHITECTURES "arm64" CACHE STRING "macOS hedef mimarisi" FORCE)
    endif()
else()
    message(WARNING "FEMCAE V1.0.2 desteklenen release hedefi macOS/arm64'dir. Bu host yalnizca kaynak dogrulamasi icin kullaniliyor.")
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(CompilerWarnings)

set(FEMCAE_MODULE_DIR "${CMAKE_BINARY_DIR}/fortran-modules")
file(MAKE_DIRECTORY "${FEMCAE_MODULE_DIR}")

set(FEMCAE_GEOMETRY_SOURCES
    src/geometry/GeometryDocument.cpp
    src/geometry/SectionProfile.cpp
    src/geometry/DxfSectionReader.cpp
    src/geometry/OcctStepImporter.cpp
)

set(FEMCAE_OCCT_FOUND OFF)
if(FEMCAE_ENABLE_OCCT_GEOMETRY)
    find_package(OpenCASCADE QUIET)
    if(OpenCASCADE_FOUND)
        set(FEMCAE_OCCT_FOUND ON)
        message(STATUS "FEMCAE geometry: OpenCASCADE enabled (${OpenCASCADE_VERSION})")
    else()
        if(FEMCAE_REQUIRE_OCCT)
            message(FATAL_ERROR "FEMCAE_REQUIRE_OCCT=ON fakat OpenCASCADE bulunamadi.")
        endif()
        message(STATUS "FEMCAE geometry: OpenCASCADE bulunamadi; portable geometry/section core + STEP stub kullanilacak")
    endif()
elseif(FEMCAE_REQUIRE_OCCT)
    message(FATAL_ERROR "FEMCAE_REQUIRE_OCCT=ON iken FEMCAE_ENABLE_OCCT_GEOMETRY kapatilamaz.")
endif()

set(FEMCAE_MESHING_SOURCES
    src/meshing/RobustPredicates.cpp
    src/meshing/RobustGeometry.cpp
    src/meshing/MeshTypes.cpp
    src/meshing/StructuredHexMesher.cpp
    src/meshing/AbaqusInpMeshReader.cpp
    src/meshing/Assignments.cpp
    src/meshing/ResultDatabase.cpp
    src/meshing/MeshingPlan.cpp
    src/meshing/AssignmentResolver.cpp
    src/meshing/GeometryMeshBridge.cpp
    src/meshing/SurfaceLoadAssembler.cpp
)

if(CMAKE_CXX_COMPILER_ID MATCHES "AppleClang|Clang|GNU")
    set_property(
        SOURCE src/meshing/RobustPredicates.cpp
        APPEND PROPERTY COMPILE_OPTIONS
        "-fno-fast-math"
        "-ffp-contract=off"
    )
endif()

set(FEMCAE_APPLICATION_SOURCES
    src/application/AnalysisSnapshot.cpp
    src/application/SolverInputBuilder.cpp
    src/application/StructuralStability.cpp
)

add_library(femcae_geometry STATIC ${FEMCAE_GEOMETRY_SOURCES})
target_include_directories(femcae_geometry PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_compile_features(femcae_geometry PUBLIC cxx_std_20)
femcae_enable_warnings(femcae_geometry)
if(FEMCAE_OCCT_FOUND)
    target_compile_definitions(femcae_geometry PUBLIC FEMCAE_GEOMETRY_HAS_OCCT=1)
    if(OpenCASCADE_INCLUDE_DIR)
        target_include_directories(femcae_geometry PRIVATE ${OpenCASCADE_INCLUDE_DIR})
    endif()
    if(OpenCASCADE_INCLUDE_DIRS)
        target_include_directories(femcae_geometry PRIVATE ${OpenCASCADE_INCLUDE_DIRS})
    endif()
    target_link_libraries(femcae_geometry PUBLIC ${OpenCASCADE_LIBRARIES})
endif()

add_library(femcae_meshing STATIC ${FEMCAE_MESHING_SOURCES})
target_include_directories(femcae_meshing PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_compile_features(femcae_meshing PUBLIC cxx_std_20)
target_link_libraries(femcae_meshing PUBLIC femcae_geometry)
femcae_enable_warnings(femcae_meshing)

add_library(femcae_application STATIC ${FEMCAE_APPLICATION_SOURCES})
target_include_directories(femcae_application PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_compile_features(femcae_application PUBLIC cxx_std_20)
target_link_libraries(femcae_application PUBLIC femcae_meshing)
femcae_enable_warnings(femcae_application)

set(FEMCAE_FOUNDATION_SOURCES
    src/foundation/fem_kinds.f90
    src/foundation/fem_constants.f90
    src/foundation/fem_ids.f90
    src/foundation/fem_status.f90
    src/foundation/fem_logger.f90
    src/foundation/fem_tolerances.f90
    src/foundation/fem_version.f90
    src/foundation/fem_metadata.f90
    src/foundation/fem_state_buffer.f90
)

set(FEMCAE_MATH_SOURCES
    src/math/fem_vector_math.f90
    src/math/fem_matrix_math.f90
    src/math/fem_tensor_notation.f90
    src/kinematics/fem_finite_strain_kinematics.f90
)

set(FEMCAE_MATERIAL_SOURCES
    src/materials/fem_linear_elastic_material.f90
    src/materials/fem_stvk_material.f90
    src/materials/fem_hyperelastic_material.f90
    src/materials/fem_j2_plasticity.f90
    src/materials/fem_constitutive_interface.f90
)

set(FEMCAE_SECTION_SOURCES
    src/sections/fem_sections.f90
)

set(FEMCAE_ELEMENT_SOURCES
    src/elements/fem_reference_elements.f90
    src/elements/fem_shape_functions.f90
    src/elements/fem_quadrature.f90
    src/elements/fem_jacobian.f90
    src/elements/fem_element_registry.f90
    src/elements/fem_element_results.f90
    src/elements/fem_element_kinematics.f90
    src/elements/fem_element_kernel.f90
    src/elements/fem_linear_truss.f90
    src/elements/fem_linear_beam.f90
    src/elements/fem_linear_continuum.f90
    src/elements/fem_structural_mass.f90
    src/elements/fem_total_lagrangian_hex8.f90
    src/elements/fem_mixed_up_hex8.f90
)

set(FEMCAE_ASSEMBLY_SOURCES
    src/assembly/fem_element_dof_map.f90
    src/assembly/fem_sparsity_graph.f90
    src/assembly/fem_sparse_matrix.f90
    src/assembly/fem_reactions.f90
    src/assembly/fem_linear_assembly.f90
    src/assembly/fem_nonlinear_assembly.f90
)

option(FEMCAE_ENABLE_ARPACK "ARPACK-NG modal backend'ini etkinlestir" ON)
set(FEMCAE_MODAL_BACKEND_SOURCES)
if(FEMCAE_ENABLE_ARPACK)
    find_library(FEMCAE_ARPACK_LIBRARY NAMES arpack)
endif()
if(FEMCAE_ARPACK_LIBRARY)
    list(APPEND FEMCAE_MODAL_BACKEND_SOURCES src/solver/fem_arpack_eigen.f90)
    message(STATUS "FEMCAE modal: ARPACK-NG backend enabled: ${FEMCAE_ARPACK_LIBRARY}")
else()
    list(APPEND FEMCAE_MODAL_BACKEND_SOURCES src/solver/fem_arpack_eigen_stub.f90)
    message(STATUS "FEMCAE modal: ARPACK-NG bulunamadi; backend stub kullanilacak")
endif()
if(APPLE)
    list(APPEND FEMCAE_MODAL_BACKEND_SOURCES src/solver/fem_accelerate_eigen.f90)
else()
    list(APPEND FEMCAE_MODAL_BACKEND_SOURCES src/solver/fem_accelerate_eigen_stub.f90)
endif()

set(FEMCAE_SOLVER_SOURCES
    src/solver/fem_dense_reference_solver.f90
    src/solver/fem_sparse_cg_solver.f90
    src/solver/fem_accelerate_bridge.f90
    src/solver/fem_linear_solver.f90
    src/solver/fem_accelerate_adapter.c
    src/solver/fem_generalized_eigen_utils.f90
    src/solver/fem_dense_eigen_solver.f90
    ${FEMCAE_MODAL_BACKEND_SOURCES}
    src/solver/fem_eigen_solver.f90
)

add_library(femcae_core STATIC
    ${FEMCAE_FOUNDATION_SOURCES}
    ${FEMCAE_MATH_SOURCES}
    ${FEMCAE_MATERIAL_SOURCES}
    ${FEMCAE_SECTION_SOURCES}
    src/nonlinear/fem_nonlinear_contracts.f90
    src/model/fem_topology.f90
    ${FEMCAE_ELEMENT_SOURCES}
    src/model/fem_sets.f90
    src/model/fem_mesh.f90
    src/model/fem_fields.f90
    src/model/fem_dofs.f90
    src/model/fem_constraints.f90
    src/model/fem_numbering.f90
    src/model/fem_coordinate_frames.f90
    src/model/fem_loads.f90
    src/model/fem_nonlinear_loads.f90
    src/contact/fem_contact_types.f90
    src/model/fem_model.f90
    src/contact/fem_contact_geometry.f90
    src/contact/fem_contact_search.f90
    src/contact/fem_contact_enforcement.f90
    src/contact/fem_contact_assembly.f90
    src/post/fem_linear_results.f90
    src/post/fem_mixed_results.f90
    src/post/fem_nonlinear_results.f90
    ${FEMCAE_ASSEMBLY_SOURCES}
    ${FEMCAE_SOLVER_SOURCES}
    src/analysis/fem_linear_static_analysis.f90
    src/analysis/fem_modal_analysis.f90
    src/nonlinear/fem_nonlinear_state.f90
    src/nonlinear/fem_nonlinear_solver.f90
    src/nonlinear/fem_checkpoint_io.f90
)

set_target_properties(femcae_core PROPERTIES
    Fortran_MODULE_DIRECTORY "${FEMCAE_MODULE_DIR}"
    POSITION_INDEPENDENT_CODE ON
)

target_include_directories(femcae_core PUBLIC
    $<BUILD_INTERFACE:${FEMCAE_MODULE_DIR}>
)

femcae_enable_warnings(femcae_core)

if(FEMCAE_ARPACK_LIBRARY)
    target_link_libraries(femcae_core PUBLIC ${FEMCAE_ARPACK_LIBRARY})
endif()

if(APPLE)
    target_link_libraries(femcae_core PUBLIC "-framework Accelerate")
endif()

add_library(femcae_api SHARED
    src/api/fem_c_api.f90
    src/api/fem_nonlinear_diagnostics_api.f90
    src/api/fem_nonlinear_product_api.f90
)
target_link_libraries(femcae_api PRIVATE femcae_core)
set_target_properties(femcae_api PROPERTIES
    OUTPUT_NAME "femcae"
    VERSION "${PROJECT_VERSION}"
    SOVERSION "${PROJECT_VERSION_MAJOR}"
    Fortran_MODULE_DIRECTORY "${FEMCAE_MODULE_DIR}/api"
)
target_include_directories(femcae_api PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
femcae_enable_warnings(femcae_api)

add_executable(femcae_cli apps/fem_cli.f90)
target_link_libraries(femcae_cli PRIVATE femcae_core)
femcae_enable_warnings(femcae_cli)

if(FEMCAE_BUILD_GUI)
    find_package(Qt6 6.5 REQUIRED COMPONENTS Core Gui Widgets OpenGLWidgets)
    qt_standard_project_setup()
    add_subdirectory(gui)
endif()

if(FEMCAE_BUILD_TESTING AND BUILD_TESTING)
    add_subdirectory(tests)
endif()

install(TARGETS femcae_api femcae_cli femcae_geometry femcae_meshing femcae_application
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
