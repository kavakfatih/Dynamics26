program test_status
    use fem_status, only : status_t, FEM_STATUS_INVALID_ARGUMENT
    use test_support, only : assert_true, assert_equal_int, assert_equal_string
    implicit none

    type(status_t) :: status

    call status%clear()
    call assert_true(status%is_ok(), "clear sonrasi status OK olmali")
    call status%set_error(FEM_STATUS_INVALID_ARGUMENT, "ornek hata")
    call assert_true(.not. status%is_ok(), "hata sonrasi status OK olmamali")
    call assert_equal_int(status%code, FEM_STATUS_INVALID_ARGUMENT, "status code")
    call assert_equal_string(status%message, "ornek hata", "status message")
    write(*, '(A)') "PASS unit_status"
end program test_status
