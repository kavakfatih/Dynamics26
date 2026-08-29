program reg_v020_001_model_numbering
    use fem_kinds,       only : rk, id_kind, index_kind
    use fem_ids,         only : INVALID_ID
    use fem_model,       only : model_t
    use fem_topology,    only : TOPOLOGY_BAR2
    use fem_fields,      only : FIELD_ID_DISPLACEMENT
    use fem_status,      only : status_t
    use test_support, only : assert_true, assert_equal_id, assert_equal_index
    implicit none

    type(model_t) :: model
    type(status_t) :: status
    integer(id_kind) :: conn(2), fixed_dof, cid
    integer(index_kind) :: pos

    call model%mesh%add_node(1000_id_kind, [0.0_rk, 0.0_rk, 0.0_rk], status)
    call assert_true(status%is_ok(), "node 1000")
    call model%mesh%add_node(42_id_kind, [2.0_rk, 0.0_rk, 0.0_rk], status)
    call assert_true(status%is_ok(), "node 42")
    conn = [1000_id_kind, 42_id_kind]
    call model%mesh%add_element(700_id_kind, TOPOLOGY_BAR2, conn, status)
    call assert_true(status%is_ok(), "element 700")

    call model%initialize_standard_registries(status)
    call assert_true(status%is_ok(), "standard topology/field registries")
    call model%add_node_set(55_id_kind, "bar_nodes", [1000_id_kind, 42_id_kind], status)
    call assert_true(status%is_ok(), "validated node set")
    call model%add_element_set(56_id_kind, "bar_elements", [700_id_kind], status)
    call assert_true(status%is_ok(), "validated element set")
    call model%add_node_set(57_id_kind, "dangling", [999999_id_kind], status)
    call assert_true(.not. status%is_ok(), "dangling node set reddedilmeli")
    call model%build_nodal_field_dofs(FIELD_ID_DISPLACEMENT, status)
    call assert_true(status%is_ok(), "displacement DOF generation")
    call assert_equal_index(model%dofs%count(), 6_index_kind, "2 node x 3 displacement DOF")

    pos = model%dofs%find_by_address(1000_id_kind, FIELD_ID_DISPLACEMENT, 1)
    call assert_true(pos > 0_index_kind, "Ux DOF bulunmali")
    fixed_dof = model%dofs%dofs(pos)%id
    call model%constraints%add(fixed_dof, 0.0_rk, cid, status)
    call assert_true(status%is_ok(), "Ux constraint")
    call model%renumber(status)
    call assert_true(status%is_ok(), "model numbering")

    call assert_equal_id(model%numbering%equation_of(fixed_dof), INVALID_ID, "fixed DOF equation ID almaz")
    call assert_equal_index(model%numbering%active_equation_count, 5_index_kind, "6 DOF - 1 constraint = 5 equation")
    call assert_true(model%mesh%find_node_position(42_id_kind) == 2_index_kind, "Node ID 42 array position 2 olabilir")
    call assert_true(model%dofs%dofs(1)%id /= model%dofs%dofs(1)%entity_id, "DOF ID ve Node ID ayri uzaylar")

    write(*, '(A)') "PASS REG-V020-001"
    write(*, '(A)') "contract=NodeID!=ArrayPosition!=DOFID!=EquationID"
end program reg_v020_001_model_numbering
