program ver_v110_004_contact_rollback
    use fem_kinds, only : rk
    use fem_model, only : model_t
    use fem_contact_types, only : CONTACT_ENFORCEMENT_AUGMENTED_LAGRANGIAN
    use fem_nonlinear_solver, only : nonlinear_solver_options_t, nonlinear_static_result_t, solve_nonlinear_static
    use fem_linear_solver, only : LINEAR_SOLVER_DENSE_REFERENCE
    use fem_status, only : status_t
    use nonlinear_test_models, only : build_contact_stvk_hex8
    use test_support, only : assert_true, assert_close
    implicit none
    type(model_t)::model
    type(nonlinear_solver_options_t)::opt
    type(nonlinear_static_result_t)::result
    type(status_t)::status
    integer::i
    call build_contact_stvk_hex8(model,1.0e7_rk,1.0e7_rk,CONTACT_ENFORCEMENT_AUGMENTED_LAGRANGIAN,status)
    call assert_true(status%is_ok(),"AL contact model build")
    opt%linear%backend=LINEAR_SOLVER_DENSE_REFERENCE
    opt%initial_load_increment=1.0_rk;opt%minimum_load_increment=0.9_rk;opt%maximum_load_increment=1.0_rk
    opt%max_iterations=1;opt%adaptive_stepping=.true.;opt%cutback_factor=0.5_rk;opt%line_search=.false.
    call solve_nonlinear_static(model,opt,result,status)
    call assert_true(.not.status%is_ok(),"forced nonlinear failure expected")
    do i=1,size(model%contacts%pairs(1)%states)
        call assert_close(model%contacts%pairs(1)%states(i)%committed_normal_multiplier,0.0_rk,1e-14_rk,1e-14_rk, &
            "failed step cannot commit AL multiplier")
        call assert_close(model%contacts%pairs(1)%states(i)%trial_normal_multiplier,0.0_rk,1e-14_rk,1e-14_rk, &
            "failed step reverts contact trial multiplier")
    end do
    write(*,'(A)')'PASS VER-V110-004 contact rollback semantics'
end program
