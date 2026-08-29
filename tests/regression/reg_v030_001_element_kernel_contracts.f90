program reg_v030_001_element_kernel_contracts
    use fem_kinds, only : rk, index_kind
    use fem_model, only : model_t
    use fem_element_registry, only : ELEMENT_TRUSS2, ELEMENT_PLANE_STRESS_QUAD4, ELEMENT_PLANE_STRAIN_QUAD4, &
        ELEMENT_AXISYM_QUAD4, ELEMENT_SOLID_HEX8
    use fem_topology, only : TOPOLOGY_QUAD4
    use fem_element_kernel, only : element_quality_t, assess_element_quality
    use fem_status, only : status_t
    use test_support, only : assert_true, assert_equal_index
    implicit none

    type(model_t) :: model
    type(element_quality_t) :: quality
    type(status_t) :: status
    real(rk) :: quad(2,4)

    call model%initialize_standard_registries(status)
    call assert_true(status%is_ok(), "model standard registries V0.3")
    call assert_equal_index(model%element_formulations%count(), 9_index_kind, "nine registered formulation definitions")
    call assert_true(model%element_formulations%find_position(ELEMENT_TRUSS2) > 0, "TRUSS2 formulation")
    call assert_true(model%element_formulations%find_position(ELEMENT_PLANE_STRESS_QUAD4) > 0, "plane stress formulation")
    call assert_true(model%element_formulations%find_position(ELEMENT_PLANE_STRAIN_QUAD4) > 0, "plane strain formulation")
    call assert_true(model%element_formulations%find_position(ELEMENT_AXISYM_QUAD4) > 0, "axisym formulation")
    call assert_true(model%element_formulations%find_position(ELEMENT_SOLID_HEX8) > 0, "solid formulation")

    quad(:,1)=[0.0_rk,0.0_rk]; quad(:,2)=[0.0_rk,1.0_rk]
    quad(:,3)=[1.0_rk,1.0_rk]; quad(:,4)=[1.0_rk,0.0_rk]
    call assess_element_quality(TOPOLOGY_QUAD4, quad, quality, status)
    call assert_true(.not. status%is_ok(), "clockwise connectivity must remain rejected")
    call assert_true(quality%is_inverted, "inverted detection regression")

    write(*,'(A)') "PASS reg_v030_001_element_kernel_contracts"
end program reg_v030_001_element_kernel_contracts
