program ver_v080_002_modified_newton
    use fem_kinds, only : rk, id_kind
    use fem_model, only : model_t
    use fem_nonlinear_solver, only : nonlinear_solver_options_t, nonlinear_static_result_t, &
        NONLINEAR_MODIFIED_NEWTON, solve_nonlinear_static
    use fem_status, only : status_t
    use nonlinear_test_models, only : build_uniaxial_stvk_hex8
    use test_support, only : assert_true, assert_close
    implicit none
    type(model_t) :: model
    type(nonlinear_solver_options_t) :: options
    type(nonlinear_static_result_t) :: result
    type(status_t) :: status
    integer(id_kind) :: right_dofs(4),eq
    real(rk) :: e,nu,l,area,stretch,g,lambda_lame,e11,total_force,expected_u
    integer :: i
    e=4.e6_rk;nu=0.25_rk;l=0.8_rk;area=0.6_rk;stretch=1.08_rk
    g=e/(2._rk*(1._rk+nu));lambda_lame=e*nu/((1._rk+nu)*(1._rk-2._rk*nu));e11=.5_rk*(stretch**2-1._rk)
    total_force=area*stretch*(lambda_lame+2._rk*g)*e11;expected_u=(stretch-1._rk)*l
    call build_uniaxial_stvk_hex8(model,e,nu,l,area,total_force,right_dofs,status);call assert_true(status%is_ok(),"modified model")
    options%method=NONLINEAR_MODIFIED_NEWTON
    options%initial_load_increment=.25_rk;options%maximum_load_increment=.25_rk
    options%line_search=.true.;options%max_iterations=40
    options%residual_relative_tolerance=1.e-9_rk;options%displacement_relative_tolerance=1.e-9_rk
    call solve_nonlinear_static(model,options,result,status);call assert_true(status%is_ok(),"modified Newton solve")
    call assert_true(result%converged,"modified Newton converged")
    do i=1,4
        eq=model%numbering%equation_of(right_dofs(i))
        call assert_close(result%active_displacement(int(eq)+1),expected_u,2.e-8_rk,2.e-8_rk,"modified Newton exact state")
    end do
    call assert_true(result%accepted_steps==4,"fixed quarter load stepping")
end program ver_v080_002_modified_newton
