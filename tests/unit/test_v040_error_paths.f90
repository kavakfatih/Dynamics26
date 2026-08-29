program test_v040_error_paths
    !! V0.4 global assembly/solver hata yollarinin sessizce gecmedigini dogrular.
    use fem_kinds, only : rk, id_kind, index_kind
    use fem_status, only : status_t, FEM_STATUS_INVALID_ARGUMENT, FEM_STATUS_NUMERICAL_FAILURE
    use fem_element_dof_map, only : element_dof_map_t
    use fem_sparsity_graph, only : sparsity_graph_t
    use fem_sparse_matrix, only : csr_matrix_t
    use fem_linear_solver, only : linear_solver_options_t, linear_solver_statistics_t, solve_linear_system, &
        LINEAR_SOLVER_DENSE_REFERENCE
    use test_support, only : assert_true, assert_equal_int
    implicit none

    type(status_t) :: status
    type(sparsity_graph_t) :: graph
    type(csr_matrix_t) :: matrix
    type(element_dof_map_t), allocatable :: maps(:)
    type(linear_solver_options_t) :: options
    type(linear_solver_statistics_t) :: statistics
    real(rk), allocatable :: solution(:)
    real(rk) :: rhs(2)

    ! Pattern disi scatter reddedilmelidir. Bos map listesinde graph yalnizca
    ! yapisal diagonalleri barindirir; (0,1) girdisi dolayisiyla gecersizdir.
    allocate(maps(0))
    call graph%build(2_index_kind, maps, status)
    call assert_true(status%is_ok(), "Diagonal-only sparsity graph kurulabilmeli.")
    call matrix%initialize_from_graph(graph, status=status)
    call assert_true(status%is_ok(), "CSR matrix graph'tan kurulabilmeli.")
    call matrix%add_value(0_id_kind, 1_id_kind, 1.0_rk, status)
    call assert_equal_int(status%code, FEM_STATUS_INVALID_ARGUMENT, &
        "Sparse pattern disi assembly girdisi INVALID_ARGUMENT vermeli.")

    ! Singular 2x2 sistem dense referans solver tarafindan numerical failure
    ! olarak raporlanmalidir; bos/yanlis solution sessiz basari sayilmamalidir.
    call matrix%clear()
    matrix%row_count = 2_index_kind
    matrix%column_count = 2_index_kind
    allocate(matrix%row_ptr(3), matrix%col_ind(4), matrix%values(4))
    matrix%row_ptr = [1_index_kind, 3_index_kind, 5_index_kind]
    matrix%col_ind = [0_id_kind, 1_id_kind, 0_id_kind, 1_id_kind]
    matrix%values = [1.0_rk, 1.0_rk, 1.0_rk, 1.0_rk]
    rhs = [1.0_rk, 1.0_rk]
    options%backend = LINEAR_SOLVER_DENSE_REFERENCE
    call solve_linear_system(matrix, rhs, solution, options, statistics, status)
    call assert_equal_int(status%code, FEM_STATUS_NUMERICAL_FAILURE, &
        "Singular sistem numerical failure vermeli.")
    call assert_true(size(solution) == 0, "Singular solve bos solution dondurmeli.")
    call assert_true(.not. statistics%converged, "Singular solve converged olmamali.")

    ! Tanimsiz backend ID de acik hata vermelidir.
    options%backend = 999
    call solve_linear_system(matrix, rhs, solution, options, statistics, status)
    call assert_equal_int(status%code, FEM_STATUS_INVALID_ARGUMENT, &
        "Bilinmeyen solver backend INVALID_ARGUMENT vermeli.")
    call assert_true(size(solution) == 0, "Bilinmeyen backend bos solution dondurmeli.")

    write(*,'(A)') "PASS: V0.4 assembly/solver error paths"
end program test_v040_error_paths
