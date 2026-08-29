program test_element_dof_map
    use fem_kinds, only : rk, id_kind, index_kind
    use fem_ids, only : INVALID_ID
    use fem_model, only : model_t
    use fem_topology, only : TOPOLOGY_BAR2
    use fem_fields, only : FIELD_ID_DISPLACEMENT
    use fem_element_dof_map, only : element_dof_map_t
    use fem_status, only : status_t
    use test_support, only : assert_true, assert_close, assert_equal_id, assert_equal_index
    implicit none

    type(model_t) :: model
    type(element_dof_map_t) :: map
    type(status_t) :: status
    integer(index_kind) :: pos
    integer(id_kind) :: constrained_dof, cid

    call model%mesh%add_node(100_id_kind, [0.0_rk, 0.0_rk, 0.0_rk], status)
    call assert_true(status%is_ok(), "node 100")
    call model%mesh%add_node(7_id_kind, [1.0_rk, 0.0_rk, 0.0_rk], status)
    call assert_true(status%is_ok(), "node 7")
    call model%mesh%add_element(999_id_kind, TOPOLOGY_BAR2, [100_id_kind, 7_id_kind], status)
    call assert_true(status%is_ok(), "bar element")
    call model%initialize_standard_registries(status)
    call assert_true(status%is_ok(), "standard registry")
    call model%build_nodal_field_dofs(FIELD_ID_DISPLACEMENT, status)
    call assert_true(status%is_ok(), "displacement dofs")

    pos = model%dofs%find_by_address(100_id_kind, FIELD_ID_DISPLACEMENT, 1)
    constrained_dof = model%dofs%dofs(pos)%id
    call model%constraints%add(constrained_dof, 1.25_rk, cid, status)
    call assert_true(status%is_ok(), "nonzero prescribed displacement")
    call model%renumber(status)
    call assert_true(status%is_ok(), "numbering")

    call map%build_nodal_field(model%mesh, 999_id_kind, FIELD_ID_DISPLACEMENT, 3, &
        model%dofs, model%constraints, model%numbering, status)
    call assert_true(status%is_ok(), "element dof map build")
    call assert_equal_index(map%size(), 6_index_kind, "BAR2 3D map 6 DOF")
    call assert_equal_id(map%dof_ids(1), constrained_dof, "node-major first DOF")
    call assert_equal_id(map%equation_ids(1), INVALID_ID, "constraint equation almaz")
    call assert_true(map%constrained(1), "first DOF constrained")
    call assert_close(map%prescribed_values(1), 1.25_rk, 1.0e-14_rk, 1.0e-14_rk, "prescribed value map'te korunur")
    call assert_equal_id(map%equation_ids(2), 0_id_kind, "ilk serbest DOF equation 0")
    call assert_equal_id(map%equation_ids(4), 2_id_kind, "ikinci node Ux equation mapping")

    write(*,'(A)') "PASS unit_element_dof_map"
end program test_element_dof_map
