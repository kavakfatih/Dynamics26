program ver_v080_001_full_newton
    use fem_kinds, only : rk, id_kind
    use fem_model, only : model_t
    use fem_nonlinear_solver, only : nonlinear_solver_options_t, nonlinear_static_result_t, &
        NONLINEAR_FULL_NEWTON, solve_nonlinear_static
    use fem_status, only : status_t
    use nonlinear_test_models, only : build_uniaxial_stvk_hex8
    use test_support, only : assert_true, assert_close
    implicit none
    type(model_t) :: model
    type(nonlinear_solver_options_t) :: options
    type(nonlinear_static_result_t) :: result
    type(status_t) :: status
    integer(id_kind) :: right_dofs(4), eq
    real(rk) :: e,nu,l,area,stretch,g,lambda_lame,e11,p11,total_force,expected_u
    integer :: i

    e=6.0e6_rk; nu=0.29_rk; l=1.0_rk; area=1.0_rk; stretch=1.20_rk
    g=e/(2.0_rk*(1.0_rk+nu))
    lambda_lame=e*nu/((1.0_rk+nu)*(1.0_rk-2.0_rk*nu))
    e11=0.5_rk*(stretch*stretch-1.0_rk)
    p11=stretch*(lambda_lame+2.0_rk*g)*e11
    total_force=area*p11
    expected_u=(stretch-1.0_rk)*l
    call build_uniaxial_stvk_hex8(model,e,nu,l,area,total_force,right_dofs,status)
    call assert_true(status%is_ok(),"V0.8 uniaxial model build")

    options%method=NONLINEAR_FULL_NEWTON
    options%initial_load_increment=0.20_rk
    options%minimum_load_increment=1.0e-4_rk
    options%maximum_load_increment=0.40_rk
    options%line_search=.true.
    options%residual_relative_tolerance=1.0e-10_rk
    options%displacement_relative_tolerance=1.0e-10_rk
    call solve_nonlinear_static(model,options,result,status)
    call assert_true(status%is_ok(),"full Newton solve status")
    call assert_true(result%converged,"full Newton converged")
    call assert_close(result%completed_load_factor,1.0_rk,1.e-14_rk,1.e-14_rk,"load factor one")
    call assert_true(result%accepted_steps>=3,"multiple load steps accepted")
    call assert_true(result%total_iterations>0,"Newton corrections executed")
    call assert_true(allocated(result%history),"convergence history allocated")
    do i=1,4
        eq=model%numbering%equation_of(right_dofs(i))
        call assert_close(result%active_displacement(int(eq)+1),expected_u,2.e-9_rk,2.e-9_rk,"StVK exact finite stretch")
    end do
    call assert_true(result%minimum_j>=1.0_rk,"positive finite-strain J")
end program ver_v080_001_full_newton
