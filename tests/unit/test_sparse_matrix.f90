program test_sparse_matrix
    use fem_kinds, only : rk, id_kind, index_kind
    use fem_element_dof_map, only : element_dof_map_t
    use fem_sparsity_graph, only : sparsity_graph_t
    use fem_sparse_matrix, only : csr_matrix_t, matrix_properties_t, MATRIX_SYMMETRY_SYMMETRIC
    use fem_linear_assembly, only : assemble_matrix_by_equation, assemble_vector_by_equation
    use fem_status, only : status_t
    use test_support, only : assert_true, assert_close, assert_equal_index
    implicit none

    type(element_dof_map_t) :: maps(2)
    type(sparsity_graph_t) :: graph
    type(csr_matrix_t) :: matrix
    type(matrix_properties_t) :: props
    type(status_t) :: status
    real(rk) :: local(2,2), local_vec(2), global_vec(3)
    real(rk), allocatable :: dense(:,:), y(:)

    allocate(maps(1)%equation_ids(2), maps(2)%equation_ids(2))
    maps(1)%equation_ids = [0_id_kind, 1_id_kind]
    maps(2)%equation_ids = [1_id_kind, 2_id_kind]
    call graph%build(3_index_kind, maps, status)
    call assert_true(status%is_ok(), "graph")
    props%symmetry = MATRIX_SYMMETRY_SYMMETRIC
    call matrix%initialize_from_graph(graph, props, status)
    call assert_true(status%is_ok(), "csr init")

    local = reshape([1.0_rk, -1.0_rk, -1.0_rk, 1.0_rk], [2,2])
    call assemble_matrix_by_equation(matrix, maps(1)%equation_ids, local, status)
    call assert_true(status%is_ok(), "element1 scatter")
    call assemble_matrix_by_equation(matrix, maps(2)%equation_ids, local, status)
    call assert_true(status%is_ok(), "element2 scatter")
    call assert_equal_index(matrix%nnz(), 7_index_kind, "csr nnz preserved")
    call assert_true(matrix%is_symmetric(1.0e-12_rk), "assembled matrix symmetric")

    call matrix%to_dense(dense, status)
    call assert_true(status%is_ok(), "dense inspection")
    call assert_close(dense(1,1), 1.0_rk, 1e-14_rk, 1e-14_rk, "K11")
    call assert_close(dense(2,2), 2.0_rk, 1e-14_rk, 1e-14_rk, "K22")
    call assert_close(dense(3,3), 1.0_rk, 1e-14_rk, 1e-14_rk, "K33")
    call assert_close(dense(1,3), 0.0_rk, 1e-14_rk, 1e-14_rk, "structural zero")

    call matrix%matvec([1.0_rk, 2.0_rk, 4.0_rk], y, status)
    call assert_true(status%is_ok(), "csr matvec")
    call assert_close(y(1), -1.0_rk, 1e-14_rk, 1e-14_rk, "y1")
    call assert_close(y(2), -1.0_rk, 1e-14_rk, 1e-14_rk, "y2")
    call assert_close(y(3), 2.0_rk, 1e-14_rk, 1e-14_rk, "y3")

    ! Aynı scatter API stiffness, tangent veya mass matrix için kullanılabilir.
    global_vec = 0.0_rk
    local_vec = [2.0_rk, 3.0_rk]
    call assemble_vector_by_equation(global_vec, maps(1)%equation_ids, local_vec, status)
    call assert_true(status%is_ok(), "residual/load vector scatter")
    call assert_close(global_vec(1), 2.0_rk, 1e-14_rk, 1e-14_rk, "vector scatter eq0")
    call assert_close(global_vec(2), 3.0_rk, 1e-14_rk, 1e-14_rk, "vector scatter eq1")

    write(*,'(A)') "PASS unit_sparse_matrix"
end program test_sparse_matrix
