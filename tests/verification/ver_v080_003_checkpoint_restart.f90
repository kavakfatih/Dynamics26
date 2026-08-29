program ver_v080_003_checkpoint_restart
    use fem_kinds,only:rk,id_kind
    use fem_model,only:model_t
    use fem_nonlinear_solver,only:nonlinear_solver_options_t,nonlinear_static_result_t,nonlinear_checkpoint_t,solve_nonlinear_static
    use fem_status,only:status_t,FEM_STATUS_NUMERICAL_FAILURE
    use nonlinear_test_models,only:build_uniaxial_stvk_hex8
    use test_support,only:assert_true,assert_close
    implicit none
    type(model_t)::model
    type(nonlinear_solver_options_t)::options
    type(nonlinear_static_result_t)::partial,restarted
    type(nonlinear_checkpoint_t)::checkpoint
    type(status_t)::status
    integer(id_kind)::dofs(4),eq
    real(rk)::e,nu,l,area,stretch,g,lam,e11,force,expected
    integer::i
    e=5.e6_rk;nu=.27_rk;l=1._rk;area=1._rk;stretch=1.12_rk
    g=e/(2._rk*(1._rk+nu));lam=e*nu/((1._rk+nu)*(1._rk-2._rk*nu));e11=.5_rk*(stretch**2-1._rk)
    force=area*stretch*(lam+2._rk*g)*e11;expected=(stretch-1._rk)*l
    call build_uniaxial_stvk_hex8(model,e,nu,l,area,force,dofs,status);call assert_true(status%is_ok(),"restart model")
    options%initial_load_increment=.25_rk;options%minimum_load_increment=.01_rk;options%maximum_load_increment=.25_rk
    options%adaptive_stepping=.false.;options%max_step_attempts=1
    call solve_nonlinear_static(model,options,partial,status)
    call assert_true(status%code==FEM_STATUS_NUMERICAL_FAILURE,"partial run intentionally stops after one accepted step")
    call assert_close(partial%completed_load_factor,.25_rk,1.e-14_rk,1.e-14_rk,"checkpoint load factor")
    call checkpoint%capture(partial,status);call assert_true(status%is_ok(),"capture checkpoint")
    options%max_step_attempts=20
    call solve_nonlinear_static(model,options,restarted,status,checkpoint);call assert_true(status%is_ok(),"restart solve")
    call assert_true(restarted%converged,"restart reached final load")
    call assert_true(restarted%accepted_steps==4,"accepted step count continued across checkpoint")
    do i=1,4
        eq=model%numbering%equation_of(dofs(i))
        call assert_close(restarted%active_displacement(int(eq)+1),expected,2.e-9_rk,2.e-9_rk,"restart final displacement")
    end do
end program ver_v080_003_checkpoint_restart
