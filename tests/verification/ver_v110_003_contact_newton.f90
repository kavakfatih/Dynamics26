program ver_v110_003_contact_newton
    use fem_kinds, only : rk
    use fem_model, only : model_t
    use fem_nonlinear_solver, only : nonlinear_solver_options_t, nonlinear_static_result_t, solve_nonlinear_static
    use fem_nonlinear_assembly, only : nonlinear_system_t, evaluate_nonlinear_system
    use fem_linear_solver, only : LINEAR_SOLVER_DENSE_REFERENCE
    use fem_status, only : status_t
    use nonlinear_test_models, only : build_contact_stvk_hex8
    use test_support, only : assert_true, assert_close
    implicit none
    type(model_t)::model
    type(nonlinear_solver_options_t)::opt
    type(nonlinear_static_result_t)::result
    type(nonlinear_system_t)::system
    type(status_t)::status
    real(rk),parameter::load=1000.0_rk
    call build_contact_stvk_hex8(model,load,1.0e8_rk,status=status)
    call assert_true(status%is_ok(),"contact model build")
    opt%linear%backend=LINEAR_SOLVER_DENSE_REFERENCE
    opt%initial_load_increment=0.25_rk;opt%maximum_load_increment=0.5_rk;opt%minimum_load_increment=1.0e-5_rk
    opt%max_iterations=20;opt%line_search=.true.;opt%residual_relative_tolerance=1.0e-9_rk
    call solve_nonlinear_static(model,opt,result,status)
    call assert_true(status%is_ok().and.result%converged,"contact Newton convergence")
    call assert_true(result%final_active_contact_count==4,"four bottom slave nodes active")
    call assert_true(result%maximum_penetration<1.0e-4_rk,"penalty penetration controlled")
    call evaluate_nonlinear_system(model,result%active_displacement,system,status,1.0_rk)
    call assert_true(status%is_ok(),"final contact system evaluation")
    call assert_close(system%total_contact_normal_force,load,1.0e-3_rk,2.0e-6_rk,"contact normal force balances applied load")
    call assert_true(system%displacement_residual_norm<1.0e-5_rk,"global equilibrium residual")
    write(*,'(A)')'PASS VER-V110-003 contact Newton equilibrium'
    write(*,'(A,ES14.6)')'normal_force=',system%total_contact_normal_force
    write(*,'(A,ES14.6)')'max_penetration=',system%maximum_penetration
end program
