program test_eigen_solvers
    use fem_kinds,only:rk
    use fem_eigen_solver,only:eigen_solver_options_t,EIGEN_SOLVER_DENSE_REFERENCE,EIGEN_SOLVER_ARPACK_NG, &
        solve_generalized_eigen,eigen_solver_backend_available
    use fem_status,only:status_t
    use test_support,only:assert_true,assert_close
    implicit none
    real(rk)::k(3,3),m(3,3);real(rk),allocatable::w(:),phi(:,:)
    type(eigen_solver_options_t)::opt;type(status_t)::status
    k=0.0_rk;m=0.0_rk;k(1,1)=4.0_rk;k(2,2)=9.0_rk;k(3,3)=16.0_rk
    m(1,1)=1.0_rk;m(2,2)=1.0_rk;m(3,3)=1.0_rk
    opt%backend=EIGEN_SOLVER_DENSE_REFERENCE;opt%requested_modes=2
    call solve_generalized_eigen(k,m,opt,w,phi,status)
    call assert_true(status%is_ok(),"Dense generalized eigen")
    call assert_close(w(1),4.0_rk,1e-10_rk,1e-10_rk,"lambda1")
    call assert_close(w(2),9.0_rk,1e-10_rk,1e-10_rk,"lambda2")
    call assert_close(dot_product(phi(:,1),matmul(m,phi(:,1))),1.0_rk,1e-10_rk,1e-10_rk,"mode1 M norm")
    if(eigen_solver_backend_available(EIGEN_SOLVER_ARPACK_NG))then
        opt%backend=EIGEN_SOLVER_ARPACK_NG
        call solve_generalized_eigen(k,m,opt,w,phi,status)
        call assert_true(status%is_ok(),"ARPACK generalized eigen")
        call assert_close(w(1),4.0_rk,1e-8_rk,1e-8_rk,"ARPACK lambda1")
        call assert_close(w(2),9.0_rk,1e-8_rk,1e-8_rk,"ARPACK lambda2")
    end if
end program
