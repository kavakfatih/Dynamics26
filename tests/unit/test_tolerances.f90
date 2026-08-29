program test_tolerances
    !! Toleranslar makine epsilon'u ile aynı kavram değildir. Bu test varsayılan
    !! solver/geometry toleranslarının pozitif ve birbirinden bağımsız metadata
    !! olarak kullanılabilir kaldığını kontrol eder.
    use fem_kinds, only : rk
    use fem_tolerances, only : tolerance_set_t
    use test_support, only : assert_true
    implicit none

    type(tolerance_set_t) :: tol

    call assert_true(tol%absolute > 0.0_rk, "absolute tolerance pozitif olmalı")
    call assert_true(tol%relative > 0.0_rk, "relative tolerance pozitif olmalı")
    call assert_true(tol%geometry > 0.0_rk, "geometry tolerance pozitif olmalı")
    call assert_true(tol%singular > 0.0_rk, "singular tolerance pozitif olmalı")
    call assert_true(tol%singular < tol%absolute, &
        "singular eşiği varsayılan absolute toleranstan daha küçük olmalı")

    write(*, '(A)') "PASS unit_tolerances"
end program test_tolerances
