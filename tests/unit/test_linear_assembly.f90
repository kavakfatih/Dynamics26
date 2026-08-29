program test_linear_assembly
    use fem_kinds, only : rk, id_kind, index_kind
    use fem_ids, only : INVALID_ID
    use fem_element_dof_map, only : element_dof_map_t
    use fem_sparsity_graph, only : sparsity_graph_t
    use fem_sparse_matrix, only : csr_matrix_t
    use fem_linear_assembly, only : assemble_stiffness_with_constraints
    use fem_status, only : status_t
    use test_support, only : assert_true, assert_close
    implicit none

    type(element_dof_map_t) :: map, graph_maps(1)
    type(sparsity_graph_t) :: graph
    type(csr_matrix_t) :: stiffness
    type(status_t) :: status
    real(rk) :: ke(2,2), fe(2), rhs(1)

    allocate(map%dof_ids(2), map%equation_ids(2), map%constrained(2), map%prescribed_values(2))
    map%dof_ids = [10_id_kind, 11_id_kind]
    map%equation_ids = [INVALID_ID, 0_id_kind]
    map%constrained = [.true., .false.]
    map%prescribed_values = [2.0_rk, 0.0_rk]
    graph_maps(1) = map
    call graph%build(1_index_kind, graph_maps, status)
    call assert_true(status%is_ok(), "one active equation graph")
    call stiffness%initialize_from_graph(graph, status=status)
    call assert_true(status%is_ok(), "stiffness csr")

    ke = reshape([10.0_rk, -10.0_rk, -10.0_rk, 10.0_rk], [2,2])
    fe = 0.0_rk
    rhs = 0.0_rk
    call assemble_stiffness_with_constraints(stiffness, rhs, map, ke, fe, status)
    call assert_true(status%is_ok(), "nonzero essential BC elimination")
    call assert_close(stiffness%value_at(0_id_kind,0_id_kind), 10.0_rk, 1e-14_rk, 1e-14_rk, "active K")
    call assert_close(rhs(1), 20.0_rk, 1e-14_rk, 1e-14_rk, "prescribed displacement RHS correction")

    write(*,'(A)') "PASS unit_linear_assembly"
end program test_linear_assembly
