program test_finite_strain_kinematics
    use fem_kinds, only : rk
    use fem_finite_strain_kinematics
    use fem_status, only : status_t
    use test_support, only : assert_true, assert_close
    implicit none
    real(rk) :: grad_u(3,3), f(3,3), e(3,3), s(3,3), sigma(3,3), p(3,3), tau(3,3)
    real(rk) :: theta, q(3,3), expected
    type(status_t) :: status
    integer :: i,j

    grad_u=0.0_rk; grad_u(1,1)=0.2_rk
    call deformation_gradient_from_displacement_gradient(grad_u,f)
    call green_lagrange_strain(f,e)
    expected=0.5_rk*((1.2_rk)**2-1.0_rk)
    call assert_close(e(1,1), expected, 1.0e-14_rk, 1.0e-14_rk, "uniaxial Green-Lagrange")
    call assert_close(e(2,2), 0.0_rk, 1.0e-14_rk, 1.0e-14_rk, "transverse E22")

    theta=0.71_rk
    q=0.0_rk; q(1,1)=cos(theta);q(1,2)=-sin(theta);q(2,1)=sin(theta);q(2,2)=cos(theta);q(3,3)=1.0_rk
    call green_lagrange_strain(q,e)
    call assert_true(maxval(abs(e))<1.0e-14_rk, "rigid rotation E zero")

    s=0.0_rk;s(1,1)=12.0_rk;s(2,2)=3.0_rk;s(1,2)=2.0_rk;s(2,1)=2.0_rk
    f=0.0_rk;f(1,1)=1.2_rk;f(2,2)=0.9_rk;f(3,3)=1.1_rk
    call second_pk_to_first_pk(f,s,p)
    call second_pk_to_kirchhoff(f,s,tau)
    call second_pk_to_cauchy(f,s,sigma,status)
    call assert_true(status%is_ok(), "cauchy status")
    call assert_close(p(1,1), 14.4_rk, 1.0e-13_rk, 1.0e-13_rk, "P11")
    call assert_close(sigma(1,1), tau(1,1)/(1.2_rk*0.9_rk*1.1_rk), 1.0e-13_rk, 1.0e-13_rk, "tau/J cauchy")
    do i=1,3;do j=1,3
        call assert_close(sigma(i,j), sigma(j,i), 1.0e-13_rk, 1.0e-13_rk, "cauchy symmetric")
    end do;end do
end program test_finite_strain_kinematics
