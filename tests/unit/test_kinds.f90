program test_kinds
    use, intrinsic :: iso_fortran_env, only : real64, int64
    use fem_kinds, only : rk, id_kind, index_kind
    use test_support, only : assert_equal_int
    implicit none

    call assert_equal_int(rk, real64, "FEM hesaplari real64 olmalidir.")
    call assert_equal_int(id_kind, int64, "Kalici kimlikler int64 olmalidir.")
    call assert_equal_int(index_kind, int64, "V0.1.0 solver index kind int64 olmalidir.")
    write(*, '(A)') "PASS unit_kinds"
end program test_kinds
