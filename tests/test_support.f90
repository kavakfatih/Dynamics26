module test_support
    use fem_kinds, only : rk, id_kind, index_kind
    implicit none
    private

    public :: assert_true, assert_close, assert_equal_int, assert_equal_id, assert_equal_index, assert_equal_string

contains

    subroutine assert_true(condition, message)
        logical, intent(in) :: condition
        character(len=*), intent(in) :: message
        if (.not. condition) then
            write(*, '(A,A)') "FAIL: ", trim(message)
            error stop 1
        end if
    end subroutine assert_true

    subroutine assert_close(actual, expected, absolute_tolerance, relative_tolerance, message)
        real(rk), intent(in) :: actual, expected
        real(rk), intent(in) :: absolute_tolerance, relative_tolerance
        character(len=*), intent(in) :: message
        real(rk) :: limit

        limit = max(absolute_tolerance, relative_tolerance * abs(expected))
        if (abs(actual - expected) > limit) then
            write(*, '(A,A)') "FAIL: ", trim(message)
            write(*, '(A,ES24.16)') "  actual   = ", actual
            write(*, '(A,ES24.16)') "  expected = ", expected
            write(*, '(A,ES24.16)') "  limit    = ", limit
            error stop 1
        end if
    end subroutine assert_close

    subroutine assert_equal_int(actual, expected, message)
        integer, intent(in) :: actual, expected
        character(len=*), intent(in) :: message
        if (actual /= expected) then
            write(*, '(A,A)') "FAIL: ", trim(message)
            write(*, '(A,I0,A,I0)') "  actual=", actual, " expected=", expected
            error stop 1
        end if
    end subroutine assert_equal_int


    subroutine assert_equal_id(actual, expected, message)
        integer(id_kind), intent(in) :: actual, expected
        character(len=*), intent(in) :: message
        if (actual /= expected) then
            write(*, '(A,A)') "FAIL: ", trim(message)
            write(*, '(A,I0,A,I0)') "  actual=", actual, " expected=", expected
            error stop 1
        end if
    end subroutine assert_equal_id

    subroutine assert_equal_index(actual, expected, message)
        integer(index_kind), intent(in) :: actual, expected
        character(len=*), intent(in) :: message
        if (actual /= expected) then
            write(*, '(A,A)') "FAIL: ", trim(message)
            write(*, '(A,I0,A,I0)') "  actual=", actual, " expected=", expected
            error stop 1
        end if
    end subroutine assert_equal_index

    subroutine assert_equal_string(actual, expected, message)
        character(len=*), intent(in) :: actual, expected
        character(len=*), intent(in) :: message
        if (trim(actual) /= trim(expected)) then
            write(*, '(A,A)') "FAIL: ", trim(message)
            write(*, '(A,A,A,A)') "  actual='", trim(actual), "' expected='", trim(expected)//"'"
            error stop 1
        end if
    end subroutine assert_equal_string

end module test_support
