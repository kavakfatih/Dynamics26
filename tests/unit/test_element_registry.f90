program test_element_registry
    use fem_kinds, only : index_kind
    use fem_element_registry, only : element_registry_t, ELEMENT_TRUSS2, ELEMENT_PLANE_STRESS_QUAD4, ELEMENT_PLANE_STRAIN_QUAD4, &
        ELEMENT_AXISYM_QUAD4, ELEMENT_SOLID_HEX8, ELEMENT_BEAM2_PROTOTYPE, ELEMENT_SHELL_QUAD4_PROTOTYPE, &
        ELEMENT_STATE_KERNEL_READY, ELEMENT_STATE_PROTOTYPE, ELEMENT_MIXED_UP_HEX8_P0, PRESSURE_INTERPOLATION_P0
    use fem_status, only : status_t
    use test_support, only : assert_true, assert_equal_index
    implicit none

    type(element_registry_t) :: registry
    type(status_t) :: status
    integer(index_kind) :: pos

    call registry%register_standard(status)
    call assert_true(status%is_ok(), "standard element registry")
    call assert_equal_index(registry%count(), 9_index_kind, "element registry count")

    pos = registry%find_position(ELEMENT_TRUSS2)
    call assert_true(pos > 0, "TRUSS2 registered")
    call assert_true(registry%elements(pos)%implementation_state == ELEMENT_STATE_KERNEL_READY, "TRUSS2 kernel ready")

    pos = registry%find_position(ELEMENT_PLANE_STRESS_QUAD4)
    call assert_true(pos > 0, "PLANE_STRESS_QUAD4 registered")
    call assert_true(.not. registry%elements(pos)%axisymmetric, "plane stress quad non-axisymmetric")
    call assert_true(registry%elements(pos)%requires_section, "plane stress thickness/section metadata")

    pos = registry%find_position(ELEMENT_PLANE_STRAIN_QUAD4)
    call assert_true(pos > 0, "PLANE_STRAIN_QUAD4 registered")
    call assert_true(.not. registry%elements(pos)%axisymmetric, "plane strain quad non-axisymmetric")

    pos = registry%find_position(ELEMENT_AXISYM_QUAD4)
    call assert_true(pos > 0, "AXISYM_QUAD4 registered")
    call assert_true(registry%elements(pos)%axisymmetric, "axisym flag")
    call assert_true(registry%elements(pos)%implementation_state == ELEMENT_STATE_KERNEL_READY, "axisym kernel ready")

    pos = registry%find_position(ELEMENT_SOLID_HEX8)
    call assert_true(pos > 0, "SOLID_HEX8 registered")

    pos = registry%find_position(ELEMENT_MIXED_UP_HEX8_P0)
    call assert_true(pos > 0, "MIXED_UP_HEX8_P0 registered")
    call assert_true(registry%elements(pos)%pressure_components == 1, "mixed pressure component metadata")
    call assert_true(registry%elements(pos)%pressure_interpolation == PRESSURE_INTERPOLATION_P0, "mixed P0 pressure interpolation")

    pos = registry%find_position(ELEMENT_BEAM2_PROTOTYPE)
    call assert_true(pos > 0, "BEAM2 linear registered")
    call assert_true(registry%elements(pos)%rotation_components == 1, "beam rotation metadata")
    call assert_true(registry%elements(pos)%requires_section, "beam section metadata")
    call assert_true(registry%elements(pos)%implementation_state == ELEMENT_STATE_KERNEL_READY, "beam linear kernel ready")

    pos = registry%find_position(ELEMENT_SHELL_QUAD4_PROTOTYPE)
    call assert_true(pos > 0, "SHELL_QUAD4 prototype registered")
    call assert_true(registry%elements(pos)%rotation_components == 3, "shell rotation metadata")
    call assert_true(registry%elements(pos)%requires_section, "shell section metadata")
    call assert_true(registry%elements(pos)%implementation_state == ELEMENT_STATE_PROTOTYPE, "shell remains prototype")

    write(*,'(A)') "PASS unit_element_registry"
end program test_element_registry
