program reg_v020_002_mixed_field_numbering
    use fem_kinds,  only : rk, id_kind, index_kind
    use fem_model,  only : model_t
    use fem_fields, only : FIELD_ID_DISPLACEMENT, FIELD_ID_PRESSURE, FIELD_ID_ROTATION
    use fem_status, only : status_t
    use test_support, only : assert_true, assert_equal_index
    implicit none

    type(model_t) :: model
    type(status_t) :: status

    call model%mesh%add_node(10_id_kind, [0.0_rk, 0.0_rk, 0.0_rk], status)
    call model%mesh%add_node(99_id_kind, [1.0_rk, 0.0_rk, 0.0_rk], status)
    call model%initialize_standard_registries(status)
    call assert_true(status%is_ok(), "standard fields")

    call model%build_nodal_field_dofs(FIELD_ID_DISPLACEMENT, status)
    call assert_true(status%is_ok(), "u field DOFs")
    call model%build_nodal_field_dofs(FIELD_ID_PRESSURE, status)
    call assert_true(status%is_ok(), "p field DOFs")
    call model%build_nodal_field_dofs(FIELD_ID_ROTATION, status)
    call assert_true(status%is_ok(), "rotation field DOFs")

    ! 2 node * (3 u + 1 p + 3 rot) = 14 DOF.
    call assert_equal_index(model%dofs%count(), 14_index_kind, "mixed field DOF count")
    call model%renumber(status)
    call assert_true(status%is_ok(), "mixed field numbering")
    call assert_equal_index(model%numbering%active_equation_count, 14_index_kind, "mixed field active equations")

    write(*, '(A)') "PASS REG-V020-002"
    write(*, '(A)') "fields=displacement(3)+pressure(1)+rotation(3)"
end program reg_v020_002_mixed_field_numbering
