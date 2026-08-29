program test_state_buffer
    use fem_kinds, only : rk, index_kind
    use fem_state_buffer, only : state_buffer_t
    use fem_status, only : status_t
    use test_support, only : assert_true, assert_close
    implicit none

    type(state_buffer_t) :: state
    type(status_t) :: status
    real(rk) :: value

    call state%initialize(2_index_kind, 1.0_rk, status)
    call assert_true(status%is_ok(), "state initialize")

    call state%begin_trial(status)
    call state%set_trial_value(1_index_kind, 5.0_rk, status)
    value = state%committed_value(1_index_kind, status)
    call assert_close(value, 1.0_rk, 0.0_rk, 0.0_rk, &
        "trial degisikligi committed state'i degistirmemeli")

    call state%revert(status)
    value = state%trial_value(1_index_kind, status)
    call assert_close(value, 1.0_rk, 0.0_rk, 0.0_rk, "revert committed degeri geri getirmeli")

    call state%set_trial_value(2_index_kind, 7.0_rk, status)
    call state%commit(status)
    value = state%committed_value(2_index_kind, status)
    call assert_close(value, 7.0_rk, 0.0_rk, 0.0_rk, "commit trial degeri kalici yapmali")
    write(*, '(A)') "PASS unit_state_buffer"
end program test_state_buffer
