program test_constants
    use fem_kinds, only : rk
    use fem_constants, only : FEM_PI, FEM_ONE
    use test_support, only : assert_close
    implicit none

    call assert_close(FEM_PI, acos(-1.0_rk), 1.0e-15_rk, 1.0e-15_rk, "PI sabiti")
    call assert_close(FEM_ONE, 1.0_rk, 0.0_rk, 0.0_rk, "ONE sabiti")
    write(*, '(A)') "PASS unit_constants"
end program test_constants
