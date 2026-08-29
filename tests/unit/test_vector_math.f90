program test_vector_math
    use fem_kinds, only : rk
    use fem_vector_math, only : vector_dot, vector_norm2, normalize_vector
    use fem_status, only : status_t
    use test_support, only : assert_true, assert_close
    implicit none

    real(rk) :: a(3), b(3), dot_value
    real(rk), allocatable :: normalized(:)
    type(status_t) :: status

    a = [1.0_rk, 2.0_rk, 2.0_rk]
    b = [3.0_rk, 4.0_rk, 5.0_rk]

    call vector_dot(a, b, dot_value, status)
    call assert_true(status%is_ok(), "dot product status")
    call assert_close(dot_value, 21.0_rk, 1.0e-14_rk, 1.0e-14_rk, "dot product")
    call assert_close(vector_norm2(a), 3.0_rk, 1.0e-14_rk, 1.0e-14_rk, "vector norm")

    call normalize_vector(a, normalized, status)
    call assert_true(status%is_ok(), "normalize status")
    call assert_close(vector_norm2(normalized), 1.0_rk, 1.0e-14_rk, 1.0e-14_rk, "normalized norm")
    write(*, '(A)') "PASS unit_vector_math"
end program test_vector_math
