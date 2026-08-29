program test_v080_nonlinear_error_paths
    use fem_kinds, only : rk,id_kind
    use fem_model, only : model_t
    use fem_nonlinear_solver, only : nonlinear_solver_options_t, nonlinear_static_result_t, solve_nonlinear_static
    use fem_status, only : status_t,FEM_STATUS_INVALID_ARGUMENT,FEM_STATUS_NUMERICAL_FAILURE
    use nonlinear_test_models, only : build_uniaxial_stvk_hex8
    use test_support, only : assert_true
    implicit none
    type(model_t)::model
    type(nonlinear_solver_options_t)::options
    type(nonlinear_static_result_t)::result
    type(status_t)::status
    integer(id_kind)::dofs(4)

    options%use_residual_criterion=.false.;options%use_displacement_criterion=.false.;options%use_energy_criterion=.false.
    call options%validate(status)
    call assert_true(status%code==FEM_STATUS_INVALID_ARGUMENT,"at least one convergence criterion required")

    call build_uniaxial_stvk_hex8(model,3.e6_rk,.28_rk,1._rk,1._rk,5.e6_rk,dofs,status)
    call assert_true(status%is_ok(),"rollback model")
    options=nonlinear_solver_options_t()
    options%max_iterations=1
    options%initial_load_increment=1.0_rk
    options%minimum_load_increment=0.5_rk
    options%maximum_load_increment=1.0_rk
    options%cutback_factor=0.5_rk
    options%line_search=.false.
    call solve_nonlinear_static(model,options,result,status)
    call assert_true(status%code==FEM_STATUS_NUMERICAL_FAILURE,"failed increments end with numerical failure")
    call assert_true(.not.result%converged,"failed solve is not converged")
    call assert_true(abs(result%completed_load_factor)<1.e-15_rk,"failed step did not commit load factor")
    call assert_true(allocated(result%active_displacement),"rollback result displacement exists")
    call assert_true(maxval(abs(result%active_displacement))<1.e-15_rk,"failed step reverted to committed zero state")
    call assert_true(result%cutback_count>=1,"cutback was attempted")
end program test_v080_nonlinear_error_paths
