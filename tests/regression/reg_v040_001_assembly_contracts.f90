program reg_v040_001_assembly_contracts
    use fem_kinds, only : rk, id_kind, index_kind
    use fem_element_dof_map, only : element_dof_map_t
    use fem_sparsity_graph, only : sparsity_graph_t
    use fem_sparse_matrix, only : csr_matrix_t, matrix_properties_t, MATRIX_SYMMETRY_SYMMETRIC
    use fem_linear_assembly, only : assemble_matrix_by_equation
    use fem_linear_solver, only : linear_solver_backend_available, LINEAR_SOLVER_DENSE_REFERENCE, &
                                  LINEAR_SOLVER_SPARSE_CG
    use fem_status, only : status_t
    use test_support, only : assert_true, assert_equal_index
    implicit none

    type(element_dof_map_t) :: maps(3)
    type(sparsity_graph_t) :: graph
    type(csr_matrix_t) :: matrix
    type(matrix_properties_t) :: props
    type(status_t) :: status
    real(rk) :: local(2,2)

    allocate(maps(1)%equation_ids(2), maps(2)%equation_ids(2), maps(3)%equation_ids(2))
    maps(1)%equation_ids = [0_id_kind, 3_id_kind]
    maps(2)%equation_ids = [3_id_kind, 1_id_kind]
    maps(3)%equation_ids = [1_id_kind, 2_id_kind]
    call graph%build(4_index_kind, maps, status)
    call assert_true(status%is_ok(), "non-geometric equation order graph")
    call assert_true(graph%contains(0_id_kind,3_id_kind), "equation IDs array positiondan bagimsiz")
    call assert_true(.not. graph%contains(0_id_kind,2_id_kind), "sparse pattern dense'e donusmemeli")

    props%symmetry = MATRIX_SYMMETRY_SYMMETRIC
    call matrix%initialize_from_graph(graph, props, status)
    call assert_true(status%is_ok(), "csr pattern")
    local = reshape([2.0_rk,-2.0_rk,-2.0_rk,2.0_rk],[2,2])
    call assemble_matrix_by_equation(matrix, maps(1)%equation_ids, local, status)
    call assert_true(status%is_ok(), "scatter by equation IDs")
    call assert_true(matrix%is_symmetric(1.0e-12_rk), "single element contribution symmetric")
    call assert_true(linear_solver_backend_available(LINEAR_SOLVER_DENSE_REFERENCE), "reference backend contract")
    call assert_true(linear_solver_backend_available(LINEAR_SOLVER_SPARSE_CG), "iterative backend contract")
    call assert_equal_index(matrix%row_count, 4_index_kind, "global equation count")

    write(*,'(A)') "PASS REG-V040-001 assembly contracts"
end program reg_v040_001_assembly_contracts
