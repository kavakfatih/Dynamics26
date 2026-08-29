program fem_cli
    !! V1.0.x komut satiri smoke/demo uygulamasi.
    !!
    !! Qt GUI ayri C++ targetidir. CLI, cekirdegin sparse assembly +
    !! backend-bagimsiz linear solve zincirinin kurulu binary icinden
    !! calistigini gosteren kucuk bir assembled sistem cozer.
    use fem_kinds, only : rk, id_kind, index_kind
    use fem_logger, only : logger_t, LOG_INFO
    use fem_version, only : FEM_VERSION_STRING, FEM_PROJECT_SCHEMA_VERSION, &
                            FEM_RESULT_SCHEMA_VERSION, FEM_C_API_VERSION
    use fem_element_dof_map, only : element_dof_map_t
    use fem_sparsity_graph, only : sparsity_graph_t
    use fem_sparse_matrix, only : csr_matrix_t, matrix_properties_t, MATRIX_SYMMETRY_SYMMETRIC, &
                                  MATRIX_DEFINITENESS_SPD_EXPECTED
    use fem_linear_assembly, only : assemble_matrix_by_equation
    use fem_linear_solver, only : linear_solver_options_t, linear_solver_statistics_t, solve_linear_system, &
                                  linear_solver_backend_available, LINEAR_SOLVER_DENSE_REFERENCE, &
                                  LINEAR_SOLVER_ACCELERATE_SPARSE_DIRECT
    use fem_status, only : status_t
    implicit none

    type(logger_t) :: logger
    type(element_dof_map_t) :: maps(1)
    type(sparsity_graph_t) :: graph
    type(csr_matrix_t) :: matrix
    type(matrix_properties_t) :: properties
    type(linear_solver_options_t) :: solver_options
    type(linear_solver_statistics_t) :: solver_stats
    type(status_t) :: status
    real(rk) :: local_matrix(2,2), rhs(2)
    real(rk), allocatable :: solution(:)

    call logger%write(LOG_INFO, "FEMCAE Verified Engineering Release")
    write(*, '(A,A)') "Application version : ", FEM_VERSION_STRING
    write(*, '(A,I0)') "Project schema      : ", FEM_PROJECT_SCHEMA_VERSION
    write(*, '(A,I0)') "Result schema       : ", FEM_RESULT_SCHEMA_VERSION
    write(*, '(A,I0)') "C API version       : ", FEM_C_API_VERSION
    write(*, '(A)') "Residual convention : R = f_ext - f_int"
    write(*, '(A)') "Newton convention   : K_T * du = R"

    allocate(maps(1)%equation_ids(2))
    maps(1)%equation_ids = [0_id_kind, 1_id_kind]
    call graph%build(2_index_kind, maps, status)
    if (.not. status%is_ok()) error stop "CLI sparsity graph kurulumu basarisiz."
    properties%symmetry = MATRIX_SYMMETRY_SYMMETRIC
    properties%definiteness = MATRIX_DEFINITENESS_SPD_EXPECTED
    call matrix%initialize_from_graph(graph, properties, status)
    if (.not. status%is_ok()) error stop "CLI sparse matrix kurulumu basarisiz."

    local_matrix = reshape([4.0_rk, 1.0_rk, 1.0_rk, 3.0_rk], [2,2])
    call assemble_matrix_by_equation(matrix, maps(1)%equation_ids, local_matrix, status)
    if (.not. status%is_ok()) error stop "CLI assembly basarisiz."
    rhs = [1.0_rk, 2.0_rk]
    solver_options%backend = LINEAR_SOLVER_DENSE_REFERENCE
    call solve_linear_system(matrix, rhs, solution, solver_options, solver_stats, status)
    if (.not. status%is_ok()) error stop "CLI linear solve basarisiz."

    write(*, '(A,2(ES14.6,1X))') "Assembled demo x     : ", solution
    write(*, '(A,ES14.6)') "Demo residual norm  : ", solver_stats%residual_norm
    if (linear_solver_backend_available(LINEAR_SOLVER_ACCELERATE_SPARSE_DIRECT)) then
        write(*, '(A)') "Accelerate sparse   : available"
    else
        write(*, '(A)') "Accelerate sparse   : unavailable on this host"
    end if
end program fem_cli
