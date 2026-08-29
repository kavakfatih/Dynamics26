program test_linear_solvers
    use fem_kinds, only : rk, id_kind, index_kind
    use fem_element_dof_map, only : element_dof_map_t
    use fem_sparsity_graph, only : sparsity_graph_t
    use fem_sparse_matrix, only : csr_matrix_t, matrix_properties_t, MATRIX_SYMMETRY_SYMMETRIC, &
                                  MATRIX_DEFINITENESS_SPD_EXPECTED
    use fem_linear_assembly, only : assemble_matrix_by_equation
    use fem_linear_solver, only : linear_solver_options_t, linear_solver_statistics_t, solve_linear_system, &
                                  linear_solver_backend_available, LINEAR_SOLVER_DENSE_REFERENCE, &
                                  LINEAR_SOLVER_SPARSE_CG, LINEAR_SOLVER_ACCELERATE_SPARSE_DIRECT
    use fem_status, only : status_t
    use test_support, only : assert_true, assert_close
    implicit none

    type(element_dof_map_t) :: maps(1)
    type(sparsity_graph_t) :: graph
    type(csr_matrix_t) :: matrix
    type(matrix_properties_t) :: props
    type(linear_solver_options_t) :: options
    type(linear_solver_statistics_t) :: stats
    type(status_t) :: status
    real(rk) :: local(2,2), rhs(2)
    real(rk), allocatable :: x(:)

    allocate(maps(1)%equation_ids(2))
    maps(1)%equation_ids = [0_id_kind, 1_id_kind]
    call graph%build(2_index_kind, maps, status)
    call assert_true(status%is_ok(), "full 2x2 graph")
    props%symmetry = MATRIX_SYMMETRY_SYMMETRIC
    props%definiteness = MATRIX_DEFINITENESS_SPD_EXPECTED
    call matrix%initialize_from_graph(graph, props, status)
    call assert_true(status%is_ok(), "matrix init")
    local = reshape([4.0_rk, 1.0_rk, 1.0_rk, 3.0_rk], [2,2])
    call assemble_matrix_by_equation(matrix, maps(1)%equation_ids, local, status)
    call assert_true(status%is_ok(), "matrix assembly")
    rhs = [1.0_rk, 2.0_rk]

    call assert_true(linear_solver_backend_available(LINEAR_SOLVER_DENSE_REFERENCE), "dense backend available")
    options%backend = LINEAR_SOLVER_DENSE_REFERENCE
    call solve_linear_system(matrix, rhs, x, options, stats, status)
    call assert_true(status%is_ok() .and. stats%converged, "dense solve")
    call assert_close(x(1), 1.0_rk/11.0_rk, 1e-12_rk, 1e-12_rk, "dense x1")
    call assert_close(x(2), 7.0_rk/11.0_rk, 1e-12_rk, 1e-12_rk, "dense x2")

    call assert_true(linear_solver_backend_available(LINEAR_SOLVER_SPARSE_CG), "CG backend available")
    options%backend = LINEAR_SOLVER_SPARSE_CG
    options%cg%relative_tolerance = 1.0e-12_rk
    options%cg%absolute_tolerance = 1.0e-14_rk
    call solve_linear_system(matrix, rhs, x, options, stats, status)
    call assert_true(status%is_ok() .and. stats%converged, "CG solve")
    call assert_close(x(1), 1.0_rk/11.0_rk, 1e-11_rk, 1e-11_rk, "CG x1")
    call assert_close(x(2), 7.0_rk/11.0_rk, 1e-11_rk, 1e-11_rk, "CG x2")

    ! macOS CI'da bu blok gerçek Accelerate SparseFactor/SparseSolve yolunu test eder.
    if (linear_solver_backend_available(LINEAR_SOLVER_ACCELERATE_SPARSE_DIRECT)) then
        options%backend = LINEAR_SOLVER_ACCELERATE_SPARSE_DIRECT
        call solve_linear_system(matrix, rhs, x, options, stats, status)
        call assert_true(status%is_ok() .and. stats%converged, "Accelerate sparse direct solve")
        call assert_close(x(1), 1.0_rk/11.0_rk, 1e-11_rk, 1e-11_rk, "Accelerate x1")
        call assert_close(x(2), 7.0_rk/11.0_rk, 1e-11_rk, 1e-11_rk, "Accelerate x2")
    end if

    write(*,'(A)') "PASS unit_linear_solvers"
end program test_linear_solvers
