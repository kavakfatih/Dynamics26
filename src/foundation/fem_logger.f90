module fem_logger
    !! Global mutable logger yerine nesne bazli basit loglama arayuzu.
    !!
    !! Solver cekirdegi GUI'yi bilmez. Ileride Qt tarafina callback eklenirse bu
    !! tipin arayuzu korunarak farkli sink/adapter uygulanabilir.
    use, intrinsic :: iso_fortran_env, only : output_unit, error_unit
    implicit none
    private

    integer, parameter, public :: LOG_DEBUG = 10
    integer, parameter, public :: LOG_INFO  = 20
    integer, parameter, public :: LOG_WARN  = 30
    integer, parameter, public :: LOG_ERROR = 40

    type, public :: logger_t
        integer :: minimum_level = LOG_INFO
    contains
        procedure :: write => logger_write
    end type logger_t

contains

    subroutine logger_write(this, level, message)
        class(logger_t), intent(in) :: this
        integer, intent(in) :: level
        character(len=*), intent(in) :: message
        integer :: unit

        if (level < this%minimum_level) return

        if (level >= LOG_ERROR) then
            unit = error_unit
        else
            unit = output_unit
        end if

        write(unit, '(A,1X,A)') trim(level_name(level)), trim(message)
    end subroutine logger_write

    pure function level_name(level) result(name)
        integer, intent(in) :: level
        character(len=7) :: name

        select case (level)
        case (LOG_DEBUG)
            name = '[DEBUG]'
        case (LOG_INFO)
            name = '[INFO] '
        case (LOG_WARN)
            name = '[WARN] '
        case (LOG_ERROR)
            name = '[ERROR]'
        case default
            name = '[LOG]  '
        end select
    end function level_name

end module fem_logger
