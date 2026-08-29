program test_v080_convergence_history
    use fem_kinds,only:rk,id_kind
    use fem_model,only:model_t
    use fem_nonlinear_solver,only:nonlinear_solver_options_t,nonlinear_static_result_t,solve_nonlinear_static
    use fem_status,only:status_t
    use nonlinear_test_models,only:build_uniaxial_stvk_hex8
    use test_support,only:assert_true
    implicit none
    type(model_t)::model
    type(nonlinear_solver_options_t)::options
    type(nonlinear_static_result_t)::result
    type(status_t)::status
    integer(id_kind)::dofs(4)
    integer::i
    call build_uniaxial_stvk_hex8(model,5.e6_rk,.3_rk,1._rk,1._rk,8.e5_rk,dofs,status);call assert_true(status%is_ok(),"history model")
    options%initial_load_increment=.5_rk;options%maximum_load_increment=.5_rk
    options%use_energy_criterion=.true.;options%energy_relative_tolerance=1.e-8_rk
    options%residual_relative_tolerance=1.e-9_rk;options%displacement_relative_tolerance=1.e-9_rk
    call solve_nonlinear_static(model,options,result,status);call assert_true(status%is_ok(),"history solve")
    call assert_true(result%converged,"history solve converged")
    call assert_true(size(result%history)>=result%accepted_steps,"history contains iteration records")
    do i=1,size(result%history)
        call assert_true(result%history(i)%line_search_alpha>0._rk .and. result%history(i)%line_search_alpha<=1._rk, &
            "line search alpha bounded")
        call assert_true(result%history(i)%relative_residual>=0._rk,"relative residual nonnegative")
        call assert_true(result%history(i)%relative_energy>=0._rk,"relative energy nonnegative")
    end do
end program test_v080_convergence_history
