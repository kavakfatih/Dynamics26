set(FEMCAE_TEST_MODULE_DIR "${CMAKE_BINARY_DIR}/test-fortran-modules")
file(MAKE_DIRECTORY "${FEMCAE_TEST_MODULE_DIR}")

add_library(femcae_test_support STATIC test_support.f90 nonlinear_test_models.f90)
set_target_properties(femcae_test_support PROPERTIES
    Fortran_MODULE_DIRECTORY "${FEMCAE_TEST_MODULE_DIR}"
)
target_include_directories(femcae_test_support PUBLIC "${FEMCAE_TEST_MODULE_DIR}")
target_link_libraries(femcae_test_support PUBLIC femcae_core)
femcae_enable_warnings(femcae_test_support)

function(add_femcae_test test_name source_file label)
    add_executable(${test_name} ${source_file})
    # femcae_test_support already links femcae_core publicly; linking it again here
    # causes macOS ld to emit "ignoring duplicate libraries: 'libfemcae_core.a'".
    target_link_libraries(${test_name} PRIVATE femcae_test_support)
    femcae_enable_warnings(${test_name})
    add_test(NAME ${test_name} COMMAND ${test_name})
    set_tests_properties(${test_name} PROPERTIES LABELS "${label}")
endfunction()

function(add_femcae_cpp_test test_name source_file label)
    add_executable(${test_name} ${source_file})
    target_link_libraries(${test_name} PRIVATE femcae_geometry)
    femcae_enable_warnings(${test_name})
    add_test(NAME ${test_name} COMMAND ${test_name})
    set_tests_properties(${test_name} PROPERTIES LABELS "${label}")
endfunction()


function(add_femcae_meshing_test test_name source_file label)
    add_executable(${test_name} ${source_file})
    target_link_libraries(${test_name} PRIVATE femcae_meshing)
    femcae_enable_warnings(${test_name})
    add_test(NAME ${test_name} COMMAND ${test_name})
    set_tests_properties(${test_name} PROPERTIES LABELS "${label}")
endfunction()

function(add_femcae_prepost_integration_test test_name source_file label)
    add_executable(${test_name} ${source_file})
    target_link_libraries(${test_name} PRIVATE femcae_meshing femcae_api)
    femcae_enable_warnings(${test_name})
    add_test(NAME ${test_name} COMMAND ${test_name})
    set_tests_properties(${test_name} PROPERTIES LABELS "${label}")
endfunction()

function(add_femcae_application_test test_name source_file label)
    add_executable(${test_name} ${source_file})
    target_link_libraries(${test_name} PRIVATE femcae_application)
    femcae_enable_warnings(${test_name})
    add_test(NAME ${test_name} COMMAND ${test_name})
    set_tests_properties(${test_name} PROPERTIES LABELS "${label}")
endfunction()
add_femcae_test(unit_kinds unit/test_kinds.f90 "unit")
add_femcae_test(unit_constants unit/test_constants.f90 "unit")
add_femcae_test(unit_status unit/test_status.f90 "unit")
add_femcae_test(unit_ids unit/test_ids.f90 "unit")
add_femcae_test(unit_state_buffer unit/test_state_buffer.f90 "unit")
add_femcae_test(unit_vector_math unit/test_vector_math.f90 "unit")
add_femcae_test(unit_matrix_math unit/test_matrix_math.f90 "unit")
add_femcae_test(unit_tensor_notation unit/test_tensor_notation.f90 "unit")
add_femcae_test(unit_metadata unit/test_metadata.f90 "unit")
add_femcae_test(unit_tolerances unit/test_tolerances.f90 "unit")
add_femcae_test(unit_error_paths unit/test_error_paths.f90 "unit")
add_femcae_test(unit_mesh unit/test_mesh.f90 "unit;model")
add_femcae_test(unit_fields unit/test_fields.f90 "unit;model")
add_femcae_test(unit_dofs_constraints unit/test_dofs_constraints.f90 "unit;model")
add_femcae_test(unit_numbering unit/test_numbering.f90 "unit;model")
add_femcae_test(unit_coordinate_frames unit/test_coordinate_frames.f90 "unit;model")
add_femcae_test(unit_topology_sets unit/test_topology_sets.f90 "unit;model")
add_femcae_test(unit_element_properties unit/test_element_properties.f90 "unit;model")

add_femcae_test(unit_reference_shape unit/test_reference_shape.f90 "unit;element")
add_femcae_test(unit_quadrature unit/test_quadrature.f90 "unit;element")
add_femcae_test(unit_jacobian unit/test_jacobian.f90 "unit;element")
add_femcae_test(unit_element_registry unit/test_element_registry.f90 "unit;element")
add_femcae_test(unit_element_kernel unit/test_element_kernel.f90 "unit;element")
add_femcae_test(unit_element_quality unit/test_element_quality.f90 "unit;element")

add_femcae_test(patch_v030_001 patch/patch_v030_001_plane_affine.f90 "patch;element")
add_femcae_test(patch_v030_002 patch/patch_v030_002_solid_affine.f90 "patch;element")
add_femcae_test(patch_v030_003 patch/patch_v030_003_axisymmetric_affine.f90 "patch;element")
add_femcae_test(patch_v030_004 patch/patch_v030_004_truss_affine.f90 "patch;element")
add_femcae_test(reg_v030_001 regression/reg_v030_001_element_kernel_contracts.f90 "regression;element")

add_femcae_test(ver_v010_001 verification/ver_v010_001_axial_bar.f90 "verification")
add_femcae_test(reg_v010_001 regression/reg_v010_001_contracts.f90 "regression")
add_femcae_test(reg_v020_001 regression/reg_v020_001_model_numbering.f90 "regression;model")
add_femcae_test(reg_v020_002 regression/reg_v020_002_mixed_field_numbering.f90 "regression;model")


add_femcae_test(unit_element_dof_map unit/test_element_dof_map.f90 "unit;assembly")
add_femcae_test(unit_sparsity_graph unit/test_sparsity_graph.f90 "unit;assembly")
add_femcae_test(unit_sparse_matrix unit/test_sparse_matrix.f90 "unit;assembly")
add_femcae_test(unit_linear_assembly unit/test_linear_assembly.f90 "unit;assembly")
add_femcae_test(unit_linear_solvers unit/test_linear_solvers.f90 "unit;solver")
add_femcae_test(unit_v040_error_paths unit/test_v040_error_paths.f90 "unit;assembly;solver")

add_femcae_test(ver_v040_001 verification/ver_v040_001_two_bar_chain.f90 "verification;assembly;solver")
add_femcae_test(reg_v040_001 regression/reg_v040_001_assembly_contracts.f90 "regression;assembly")

add_executable(c_api_smoke c_api/test_c_api.c)
target_link_libraries(c_api_smoke PRIVATE femcae_api)
add_test(NAME c_api_smoke COMMAND c_api_smoke)
set_tests_properties(c_api_smoke PROPERTIES LABELS "api;smoke")

add_executable(c_api_beta3_nonlinear c_api/test_beta3_nonlinear_api.c)
target_link_libraries(c_api_beta3_nonlinear PRIVATE femcae_api)
femcae_enable_warnings(c_api_beta3_nonlinear)
add_test(NAME c_api_beta3_nonlinear COMMAND c_api_beta3_nonlinear)
set_tests_properties(c_api_beta3_nonlinear PROPERTIES
    LABELS "api;nonlinear;beta3;equilibrium;telemetry")

add_executable(c_api_rc1_nonlinear_termination
    c_api/test_rc1_nonlinear_termination_api.c)
target_link_libraries(c_api_rc1_nonlinear_termination PRIVATE femcae_api)
femcae_enable_warnings(c_api_rc1_nonlinear_termination)
add_test(NAME c_api_rc1_nonlinear_termination
    COMMAND c_api_rc1_nonlinear_termination)
set_tests_properties(c_api_rc1_nonlinear_termination PROPERTIES
    LABELS "api;nonlinear;rc1;termination;failure")

add_femcae_prepost_integration_test(
    c_api_beta3_nonlinear_refinement
    c_api/test_beta3_nonlinear_refinement.cpp
    "api;nonlinear;beta3;equilibrium;refinement;meshing")

add_test(NAME cli_smoke COMMAND femcae_cli)
set_tests_properties(cli_smoke PROPERTIES
    LABELS "smoke"
    PASS_REGULAR_EXPRESSION "Residual convention : R = f_ext - f_int"
)

# V0.5.0 — linear structural FEM
add_femcae_test(unit_linear_material unit/test_linear_material.f90 "unit;material;linear")
add_femcae_test(unit_sections_loads unit/test_sections_loads.f90 "unit;model;linear")
add_femcae_test(unit_linear_elements unit/test_linear_elements.f90 "unit;element;linear")
add_femcae_test(patch_v050_001 patch/patch_v050_001_plane_stress.f90 "patch;element;linear")
add_femcae_test(patch_v050_002 patch/patch_v050_002_axisymmetric.f90 "patch;element;linear")
add_femcae_test(patch_v050_003 patch/patch_v050_003_hex8.f90 "patch;element;linear")
add_femcae_test(ver_v050_001 verification/ver_v050_001_plane_stress_quad4.f90 "verification;assembly;linear")
add_femcae_test(ver_v050_002 verification/ver_v050_002_beam_cantilever.f90 "verification;assembly;linear")
add_femcae_test(unit_linear_results unit/test_linear_results.f90 "unit;post;linear")
add_femcae_test(ver_v050_003 verification/ver_v050_003_linear_driver.f90 "verification;analysis;linear")
add_femcae_test(unit_v050_analysis_error_paths unit/test_v050_analysis_error_paths.f90 "unit;analysis;linear")
add_femcae_test(patch_v050_004 patch/patch_v050_004_plane_strain.f90 "patch;element;linear")

# V0.6.0 — modal dynamics and mass matrices
add_femcae_test(unit_structural_mass unit/test_structural_mass.f90 "unit;element;modal")
add_femcae_test(unit_eigen_solvers unit/test_eigen_solvers.f90 "unit;solver;modal")
add_femcae_test(ver_v060_001 verification/ver_v060_001_axial_modal.f90 "verification;analysis;modal")
add_femcae_test(unit_v060_orientation unit/test_v060_orientation.f90 "unit;model;modal")
add_femcae_test(ver_v060_002 verification/ver_v060_002_beam_modal.f90 "verification;analysis;modal;beam")
add_femcae_test(ver_v060_003 verification/ver_v060_003_free_free_axial.f90 "verification;analysis;modal;rigid-body")
add_femcae_test(unit_v060_error_paths unit/test_v060_error_paths.f90 "unit;analysis;solver;modal")

# V0.7.0 — finite strain / geometric nonlinearity
add_femcae_test(unit_finite_strain_kinematics unit/test_finite_strain_kinematics.f90 "unit;nonlinear;finite-strain")
add_femcae_test(unit_stvk_material unit/test_stvk_material.f90 "unit;material;nonlinear;finite-strain")
add_femcae_test(unit_nonlinear_state unit/test_nonlinear_state.f90 "unit;nonlinear;state")
add_femcae_test(patch_v070_001 patch/patch_v070_001_hex8_affine_finite_strain.f90 "patch;element;nonlinear;finite-strain")
add_femcae_test(ver_v070_001 verification/ver_v070_001_rigid_rotation.f90 "verification;nonlinear;finite-strain;objectivity")
add_femcae_test(ver_v070_002 verification/ver_v070_002_hex8_tangent.f90 "verification;nonlinear;finite-strain;tangent")
add_femcae_test(unit_v070_error_paths unit/test_v070_error_paths.f90 "unit;nonlinear;error-path")
add_femcae_test(ver_v070_003 verification/ver_v070_003_global_nonlinear_assembly.f90 "verification;assembly;nonlinear;finite-strain;tangent")
add_femcae_test(ver_v070_004 verification/ver_v070_004_superposed_rotation.f90 "verification;nonlinear;finite-strain;objectivity")
add_femcae_test(unit_nonlinear_load_metadata unit/test_nonlinear_load_metadata.f90 "unit;nonlinear;loads")
add_femcae_test(ver_v070_005 verification/ver_v070_005_finite_stretch.f90 "verification;nonlinear;finite-strain")

# V0.8.0 — nonlinear Newton solver / load stepping
add_femcae_test(ver_v080_001 verification/ver_v080_001_full_newton.f90 "verification;nonlinear;newton")
add_femcae_test(ver_v080_002 verification/ver_v080_002_modified_newton.f90 "verification;nonlinear;newton;modified-newton")
add_femcae_test(unit_v080_nonlinear_error_paths unit/test_v080_nonlinear_error_paths.f90 "unit;nonlinear;error-path;rollback")
add_femcae_test(unit_v080_convergence_history unit/test_v080_convergence_history.f90 "unit;nonlinear;convergence;line-search")

add_femcae_test(ver_v080_003 verification/ver_v080_003_checkpoint_restart.f90 "verification;nonlinear;restart;checkpoint")

# V0.9.0 — hyperelastic / plastic constitutive models
add_femcae_test(unit_hyperelastic_material unit/test_hyperelastic_material.f90 "unit;material;hyperelastic")
add_femcae_test(ver_v090_001 verification/ver_v090_001_hyperelastic_tangent.f90 "verification;material;hyperelastic;tangent")
add_femcae_test(ver_v090_002 verification/ver_v090_002_homogeneous_hyperelastic.f90 "verification;material;hyperelastic;homogeneous")
add_femcae_test(unit_j2_plasticity unit/test_j2_plasticity.f90 "unit;material;plasticity;state")
add_femcae_test(ver_v090_003 verification/ver_v090_003_j2_tangent.f90 "verification;material;plasticity;tangent")
add_femcae_test(ver_v090_004 verification/ver_v090_004_hyperelastic_hex8_tangent.f90 "verification;element;hyperelastic;tangent")
add_femcae_test(ver_v090_005 verification/ver_v090_005_hyperelastic_newton.f90 "verification;nonlinear;newton;hyperelastic")
add_femcae_test(unit_constitutive_interface unit/test_constitutive_interface.f90 "unit;material;constitutive")
add_femcae_test(ver_v090_006 verification/ver_v090_006_energy_stress.f90 "verification;material;hyperelastic;energy")
add_femcae_test(ver_v090_007 verification/ver_v090_007_j2_loading.f90 "verification;material;plasticity;return-map;state")

add_femcae_test(ver_v090_008 verification/ver_v090_008_ogden_near_repeated.f90 "verification;material;hyperelastic;tangent;ogden")

# V0.10.0 — mixed displacement-pressure incompressibility
add_femcae_test(ver_v100_001 verification/ver_v100_001_mixed_up_hex8_tangent.f90 "verification;element;nonlinear;hyperelastic;mixed-up;incompressibility;tangent")
add_femcae_test(ver_v100_002 verification/ver_v100_002_global_mixed_tangent.f90 "verification;assembly;nonlinear;hyperelastic;mixed-up;incompressibility;tangent")
add_femcae_test(ver_v100_003 verification/ver_v100_003_mixed_newton_shear.f90 "verification;nonlinear;newton;hyperelastic;mixed-up;incompressibility")
add_femcae_test(unit_v100_mixed_dof_map unit/test_v100_mixed_dof_map.f90 "unit;model;assembly;mixed-up;incompressibility")
add_femcae_test(unit_v100_mixed_error_paths unit/test_v100_mixed_error_paths.f90 "unit;nonlinear;solver;error-path;mixed-up;incompressibility")
add_femcae_test(ver_v100_004 verification/ver_v100_004_locking_benchmark.f90 "verification;nonlinear;newton;hyperelastic;mixed-up;incompressibility;locking")
add_femcae_test(unit_v100_pressure_results unit/test_v100_pressure_results.f90 "unit;post;mixed-up;incompressibility;pressure")

# V0.11.0 — contact / friction
add_femcae_test(unit_v110_contact_geometry unit/test_v110_contact_geometry.f90 "unit;contact;search;geometry")
add_femcae_test(unit_v110_contact_state unit/test_v110_contact_state.f90 "unit;contact;state;rollback")
add_femcae_test(ver_v110_001 verification/ver_v110_001_contact_tangent.f90 "verification;contact;friction;tangent")
add_femcae_test(ver_v110_002 verification/ver_v110_002_augmented_lagrangian.f90 "verification;contact;augmented-lagrangian;state")
add_femcae_test(unit_v110_contact_error_paths unit/test_v110_contact_error_paths.f90 "unit;contact;error-path")
add_femcae_test(ver_v110_003 verification/ver_v110_003_contact_newton.f90 "verification;contact;nonlinear;newton;equilibrium")
add_femcae_test(ver_v110_004 verification/ver_v110_004_contact_rollback.f90 "verification;contact;nonlinear;rollback;augmented-lagrangian")
add_femcae_test(ver_v110_005 verification/ver_v110_005_global_coulomb.f90 "verification;contact;friction;coulomb;nonlinear;newton")
add_femcae_test(unit_v110_solver_error_paths unit/test_v110_solver_error_paths.f90 "unit;contact;solver;error-path;restart")

# V0.12.0 — CAD / geometry / custom sections
add_femcae_cpp_test(unit_v120_geometry_document geometry/test_geometry_document.cpp "unit;cad;geometry")
add_femcae_cpp_test(unit_v120_geometry_separation geometry/test_geometry_separation.cpp "unit;cad;geometry;provenance")
add_femcae_cpp_test(ver_v120_001 geometry/ver_v120_001_section_rectangle.cpp "verification;section;geometry")
add_femcae_cpp_test(ver_v120_002 geometry/ver_v120_002_section_hollow.cpp "verification;section;geometry")
add_femcae_cpp_test(ver_v120_003 geometry/ver_v120_003_dxf_section.cpp "verification;section;dxf;geometry")
add_femcae_cpp_test(unit_v120_dxf_curves geometry/unit_v120_dxf_curves.cpp "unit;section;dxf;geometry")
add_femcae_cpp_test(unit_v120_occt_adapter geometry/unit_v120_occt_adapter.cpp "unit;cad;geometry;occt")
if(FEMCAE_OCCT_FOUND)
    add_femcae_cpp_test(ver_v120_004 geometry/ver_v120_004_occt_step_box.cpp "verification;cad;geometry;occt;step")
endif()

# V0.13.0 — meshing + full pre/post integration
add_femcae_meshing_test(unit_v130_structured_hex meshing/unit_v130_structured_hex.cpp "unit;meshing;quality;provenance")
add_femcae_meshing_test(unit_v130_external_mesh meshing/unit_v130_external_mesh.cpp "unit;meshing;external-mesh")
add_femcae_meshing_test(unit_v130_assignments_results meshing/unit_v130_assignments_results.cpp "unit;prepost;assignment;probe;export")
add_femcae_meshing_test(unit_v130_sizing_assignments meshing/unit_v130_sizing_assignments.cpp "unit;meshing;sizing;refinement;assignment;provenance")
add_femcae_meshing_test(unit_v130_geometry_bridge meshing/unit_v130_geometry_bridge.cpp "unit;meshing;geometry;provenance")
add_femcae_meshing_test(unit_beta3_surface_load meshing/unit_beta3_surface_load.cpp "unit;meshing;surface-load;beta3")
add_femcae_application_test(unit_beta3_analysis_snapshot application/unit_beta3_analysis_snapshot.cpp "unit;application;snapshot;beta3")
add_femcae_application_test(unit_rc1_structural_stability application/unit_rc1_structural_stability.cpp "unit;application;preflight;stability;rigid-body;rc1")
add_femcae_prepost_integration_test(ver_v130_001 meshing/ver_v130_001_mesh_to_solve.cpp "verification;meshing;prepost;integration;linear")
if(FEMCAE_OCCT_FOUND)
    add_femcae_meshing_test(ver_v130_002 meshing/ver_v130_002_occt_step_box_meshing.cpp "verification;cad;step;occt;meshing;provenance")
endif()

# V1.0.0 — verification / hardening / release engineering
add_femcae_prepost_integration_test(ver_v1000_001 release/ver_v1000_001_mesh_convergence.cpp "verification;release;convergence;meshing;linear;benchmark")
add_femcae_test(ver_v1000_002 release/ver_v1000_002_checkpoint_file.f90 "verification;release;restart;checkpoint;checkpoint-io;corrupted-file")
add_femcae_meshing_test(unit_v1000_corrupted_inputs release/unit_v1000_corrupted_inputs.cpp "unit;release;error-path;corrupted-file;external-mesh;dxf")
add_executable(unit_v1000_api_error_paths release/unit_v1000_api_error_paths.c)
target_link_libraries(unit_v1000_api_error_paths PRIVATE femcae_api)
add_test(NAME unit_v1000_api_error_paths COMMAND unit_v1000_api_error_paths)
set_tests_properties(unit_v1000_api_error_paths PROPERTIES LABELS "unit;release;api;error-path")
femcae_enable_warnings(unit_v1000_api_error_paths)
add_femcae_prepost_integration_test(perf_v1000_001 release/perf_v1000_001_linear_hex8.cpp "release;performance;meshing;linear")

# V1.0.2 — GitHub/reproducible-source hardening
find_package(Python3 COMPONENTS Interpreter QUIET)
if(Python3_Interpreter_FOUND)
    add_test(
        NAME unit_v102_repository_hardening
        COMMAND ${Python3_EXECUTABLE}
                ${CMAKE_CURRENT_SOURCE_DIR}/release/unit_v102_repository_hardening.py
                ${CMAKE_SOURCE_DIR}
    )
    set_tests_properties(unit_v102_repository_hardening PROPERTIES
        LABELS "unit;release;repository;reproducibility"
    )
endif()


# M1.6 — executable robust-geometry verification prototype
add_executable(
    unit_m16_predicate_fixture_reader
    meshing/robust_geometry/test_predicate_fixture_reader.cpp
)
target_link_libraries(unit_m16_predicate_fixture_reader PRIVATE femcae_meshing)
femcae_enable_warnings(unit_m16_predicate_fixture_reader)
add_test(
    NAME unit_m16_predicate_fixture_reader
    COMMAND unit_m16_predicate_fixture_reader
            ${CMAKE_CURRENT_SOURCE_DIR}/meshing/robust_geometry/fixtures
)
set_tests_properties(unit_m16_predicate_fixture_reader PROPERTIES
    LABELS "unit;meshing;robust-geometry;predicate;fixture;determinism"
)

if(Python3_Interpreter_FOUND)
    add_test(
        NAME unit_m16_exact_oracle
        COMMAND ${Python3_EXECUTABLE}
                ${CMAKE_CURRENT_SOURCE_DIR}/meshing/robust_geometry/test_oracle_generation.py
                ${CMAKE_SOURCE_DIR}
    )
    set_tests_properties(unit_m16_exact_oracle PROPERTIES
        LABELS "unit;meshing;robust-geometry;predicate;exact-oracle;determinism"
    )
endif()


# M1.7 — exact production/reference robust predicates
add_executable(
    unit_m17_robust_predicates
    meshing/robust_geometry/test_robust_predicates.cpp
)
target_link_libraries(unit_m17_robust_predicates PRIVATE femcae_meshing)
femcae_enable_warnings(unit_m17_robust_predicates)

add_test(
    NAME unit_m17_robust_predicates_committed
    COMMAND unit_m17_robust_predicates
            ${CMAKE_CURRENT_SOURCE_DIR}/meshing/robust_geometry/fixtures
)
set_tests_properties(unit_m17_robust_predicates_committed PROPERTIES
    LABELS "unit;meshing;robust-geometry;predicate;exact;determinism;m1.7"
)

if(Python3_Interpreter_FOUND)
    set(FEMCAE_M17_GENERATED_DIR "${CMAKE_BINARY_DIR}/m17-predicate-corpus")
    add_test(
        NAME unit_m17_generate_predicate_corpus
        COMMAND ${Python3_EXECUTABLE}
                ${CMAKE_SOURCE_DIR}/tools/meshing_oracle/generate_predicate_corpus.py
                --output-dir ${FEMCAE_M17_GENERATED_DIR}
                --random-cases 256
                --seed 13789718
    )
    set_tests_properties(unit_m17_generate_predicate_corpus PROPERTIES
        LABELS "unit;meshing;robust-geometry;predicate;exact-oracle;determinism;m1.7"
        FIXTURES_SETUP d26_m17_predicate_corpus
    )

    add_test(
        NAME unit_m17_robust_predicates_generated
        COMMAND unit_m17_robust_predicates
                ${FEMCAE_M17_GENERATED_DIR}
    )
    set_tests_properties(unit_m17_robust_predicates_generated PROPERTIES
        LABELS "unit;meshing;robust-geometry;predicate;exact;generated;determinism;m1.7"
        FIXTURES_REQUIRED d26_m17_predicate_corpus
    )
endif()


# M1.9-A — adversarial/metamorphic corpus + one-case replay
if(Python3_Interpreter_FOUND)
    set(FEMCAE_M19_ADVERSARIAL_DIR "${CMAKE_BINARY_DIR}/m19-adversarial-corpus")
    add_test(
        NAME unit_m19_generate_adversarial
        COMMAND ${Python3_EXECUTABLE}
                ${CMAKE_CURRENT_SOURCE_DIR}/meshing/robust_geometry/test_m19_adversarial.py
                ${CMAKE_SOURCE_DIR}
                ${FEMCAE_M19_ADVERSARIAL_DIR}
    )
    set_tests_properties(unit_m19_generate_adversarial PROPERTIES
        LABELS "unit;meshing;robust-geometry;predicate;adversarial;metamorphic;replay;m1.9"
        FIXTURES_SETUP d26_m19_adversarial
    )

    add_test(
        NAME unit_m19_robust_predicates_adversarial
        COMMAND unit_m17_robust_predicates
                ${FEMCAE_M19_ADVERSARIAL_DIR}
    )
    set_tests_properties(unit_m19_robust_predicates_adversarial PROPERTIES
        LABELS "unit;meshing;robust-geometry;predicate;adversarial;metamorphic;m1.9"
        FIXTURES_REQUIRED d26_m19_adversarial
    )

    add_test(
        NAME unit_m19_replay_roundtrip
        COMMAND unit_m17_robust_predicates
                ${FEMCAE_M19_ADVERSARIAL_DIR}/replay_orient3d.d26pred
    )
    set_tests_properties(unit_m19_replay_roundtrip PROPERTIES
        LABELS "unit;meshing;robust-geometry;predicate;replay;determinism;m1.9"
        FIXTURES_REQUIRED d26_m19_adversarial
    )
endif()
