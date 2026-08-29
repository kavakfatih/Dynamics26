program test_matrix_math
    use fem_kinds, only : rk
    use fem_matrix_math, only : matrix_vector_product, determinant_2x2, &
                                determinant_3x3, identity_matrix_3x3
    use fem_status, only : status_t
    use test_support, only : assert_true, assert_close
    implicit none

    real(rk) :: a2(2,2), a3(3,3), identity(3,3), x(2)
    real(rk), allocatable :: y(:)
    type(status_t) :: status

    a2(1,:) = [2.0_rk, 1.0_rk]
    a2(2,:) = [3.0_rk, 4.0_rk]
    call assert_close(determinant_2x2(a2), 5.0_rk, 1.0e-14_rk, 1.0e-14_rk, "det 2x2")

    a3 = reshape([1.0_rk, 0.0_rk, 0.0_rk, &
                  2.0_rk, 3.0_rk, 0.0_rk, &
                  4.0_rk, 5.0_rk, 6.0_rk], [3,3])
    call assert_close(determinant_3x3(a3), 18.0_rk, 1.0e-14_rk, 1.0e-14_rk, "det 3x3")

    x = [1.0_rk, 2.0_rk]
    call matrix_vector_product(a2, x, y, status)
    call assert_true(status%is_ok(), "matvec status")
    call assert_close(y(1), 4.0_rk, 1.0e-14_rk, 1.0e-14_rk, "matvec y1")
    call assert_close(y(2), 11.0_rk, 1.0e-14_rk, 1.0e-14_rk, "matvec y2")

    identity = identity_matrix_3x3()
    call assert_close(sum(identity), 3.0_rk, 1.0e-14_rk, 1.0e-14_rk, "identity trace sum")
    write(*, '(A)') "PASS unit_matrix_math"
end program test_matrix_math
