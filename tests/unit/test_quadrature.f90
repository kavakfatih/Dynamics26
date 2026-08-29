program test_quadrature
    use fem_kinds, only : rk
    use fem_quadrature, only : quadrature_rule_t, create_gauss_rule
    use fem_status, only : status_t
    use test_support, only : assert_true, assert_close, assert_equal_int
    implicit none

    type(quadrature_rule_t) :: rule
    type(status_t) :: status
    real(rk) :: integral
    integer :: p

    call create_gauss_rule(1, 2, rule, status)
    call assert_true(status%is_ok(), "1D rule")
    call assert_equal_int(rule%point_count, 2, "1D point count")
    integral = 0.0_rk
    do p = 1, rule%point_count
        integral = integral + rule%weights(p) * rule%points(1,p)**2
    end do
    call assert_close(integral, 2.0_rk/3.0_rk, 1.0e-14_rk, 1.0e-14_rk, "integral xi^2")

    call create_gauss_rule(2, 2, rule, status)
    call assert_true(status%is_ok(), "2D rule")
    call assert_equal_int(rule%point_count, 4, "2D point count")
    integral = 0.0_rk
    do p = 1, rule%point_count
        integral = integral + rule%weights(p) * &
            (rule%points(1,p)**2 + rule%points(2,p)**2)
    end do
    call assert_close(integral, 8.0_rk/3.0_rk, 1.0e-14_rk, 1.0e-14_rk, "square polynomial integral")

    call create_gauss_rule(3, 2, rule, status)
    call assert_true(status%is_ok(), "3D rule")
    call assert_equal_int(rule%point_count, 8, "3D point count")
    integral = 0.0_rk
    do p = 1, rule%point_count
        integral = integral + rule%weights(p)
    end do
    call assert_close(integral, 8.0_rk, 1.0e-14_rk, 1.0e-14_rk, "reference cube volume")

    write(*,'(A)') "PASS unit_quadrature"
end program test_quadrature
