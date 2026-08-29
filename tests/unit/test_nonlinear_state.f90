program test_nonlinear_state
    use fem_kinds, only : rk, index_kind
    use fem_nonlinear_state, only : nonlinear_displacement_state_t
    use fem_status, only : status_t
    use test_support, only : assert_true, assert_close
    implicit none
    type(nonlinear_displacement_state_t) :: state
    type(status_t) :: status
    real(rk) :: values(3)

    call state%initialize(int(3,index_kind), status)
    call assert_true(status%is_ok(), "nonlinear state initialize")
    call state%begin_trial(status)
    call assert_true(status%is_ok(), "nonlinear state begin trial")
    values = [1.0_rk, 2.0_rk, 3.0_rk]
    call state%set_trial(values, status)
    call assert_true(status%is_ok(), "nonlinear state set trial")
    call state%commit(status)
    call assert_true(status%is_ok(), "nonlinear state commit")
    values = [9.0_rk, 9.0_rk, 9.0_rk]
    call state%set_trial(values, status)
    call assert_true(status%is_ok(), "nonlinear state second trial")
    call state%revert(status)
    call assert_true(status%is_ok(), "nonlinear state revert")
    call assert_close(state%trial(2), 2.0_rk, 1.0e-14_rk, 1.0e-14_rk, "revert restores committed")
end program test_nonlinear_state
