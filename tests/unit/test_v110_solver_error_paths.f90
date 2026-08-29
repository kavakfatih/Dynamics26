program test_v110_solver_error_paths
    use fem_kinds, only : rk
    use fem_model, only : model_t
    use fem_nonlinear_solver, only : nonlinear_solver_options_t, nonlinear_static_result_t, nonlinear_checkpoint_t, solve_nonlinear_static
    use fem_linear_solver, only : LINEAR_SOLVER_SPARSE_CG, LINEAR_SOLVER_DENSE_REFERENCE
    use fem_status, only : status_t
    use nonlinear_test_models, only : build_contact_stvk_hex8
    use test_support, only : assert_true
    implicit none
    type(model_t)::model
    type(nonlinear_solver_options_t)::opt
    type(nonlinear_static_result_t)::result
    type(nonlinear_checkpoint_t)::checkpoint
    type(status_t)::status
    call build_contact_stvk_hex8(model,1000.0_rk,1.0e8_rk,status=status);call assert_true(status%is_ok(),"contact model")
    opt%linear%backend=LINEAR_SOLVER_SPARSE_CG
    call solve_nonlinear_static(model,opt,result,status)
    call assert_true(.not.status%is_ok(),"CG rejected for contact tangent")
    opt%linear%backend=LINEAR_SOLVER_DENSE_REFERENCE
    allocate(checkpoint%active_displacement(int(model%numbering%active_equation_count)));checkpoint%active_displacement=0.0_rk
    checkpoint%load_factor=0.25_rk
    call solve_nonlinear_static(model,opt,result,status,checkpoint)
    call assert_true(.not.status%is_ok(),"contact checkpoint rejected until history snapshot exists")
    write(*,'(A)')'PASS V0.11 contact solver error paths'
end program
