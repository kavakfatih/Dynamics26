module fem_status
    !! Library kodunda kontrolsuz STOP/ERROR STOP kullanmak yerine hata bilgisini
    !! cagiran katmana tasimak icin ortak status nesnesi.
    implicit none
    private

    integer, parameter, public :: FEM_STATUS_OK = 0
    integer, parameter, public :: FEM_STATUS_INVALID_ARGUMENT = 10
    integer, parameter, public :: FEM_STATUS_SIZE_MISMATCH = 20
    integer, parameter, public :: FEM_STATUS_NUMERICAL_FAILURE = 30
    integer, parameter, public :: FEM_STATUS_NOT_INITIALIZED = 40

    type, public :: status_t
        integer :: code = FEM_STATUS_OK
        character(len=:), allocatable :: message
    contains
        procedure :: is_ok => status_is_ok
        procedure :: clear => status_clear
        procedure :: set_error => status_set_error
    end type status_t

contains

    pure logical function status_is_ok(this)
        class(status_t), intent(in) :: this
        status_is_ok = this%code == FEM_STATUS_OK
    end function status_is_ok

    subroutine status_clear(this)
        class(status_t), intent(inout) :: this
        this%code = FEM_STATUS_OK
        this%message = "OK"
    end subroutine status_clear

    subroutine status_set_error(this, code, message)
        class(status_t), intent(inout) :: this
        integer, intent(in) :: code
        character(len=*), intent(in) :: message

        this%code = code
        this%message = trim(message)
    end subroutine status_set_error

end module fem_status
