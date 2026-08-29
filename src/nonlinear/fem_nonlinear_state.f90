module fem_nonlinear_state
    !! V0.8 Newton/load-step solver'inin kullanacagi displacement state baseline'i.
    !! V0.7 bu type'i yalnızca trial/commit/revert semantigini global DOF vectorune
    !! tasimak icin kurar; Newton algoritmasi burada uygulanmaz.
    use fem_kinds, only : rk, index_kind
    use fem_status, only : status_t, FEM_STATUS_INVALID_ARGUMENT, FEM_STATUS_NOT_INITIALIZED
    implicit none
    private

    type, public :: nonlinear_displacement_state_t
        real(rk), allocatable :: committed(:)
        real(rk), allocatable :: trial(:)
    contains
        procedure :: initialize => nonlinear_state_initialize
        procedure :: begin_trial => nonlinear_state_begin_trial
        procedure :: set_trial => nonlinear_state_set_trial
        procedure :: commit => nonlinear_state_commit
        procedure :: revert => nonlinear_state_revert
        procedure :: is_initialized => nonlinear_state_is_initialized
    end type nonlinear_displacement_state_t

contains

    subroutine nonlinear_state_initialize(this, ndof, status)
        class(nonlinear_displacement_state_t), intent(inout) :: this
        integer(index_kind), intent(in) :: ndof
        type(status_t), intent(out) :: status
        call status%clear()
        if(ndof<0_index_kind)then
            call status%set_error(FEM_STATUS_INVALID_ARGUMENT,"Nonlinear displacement DOF sayisi negatif olamaz.");return
        end if
        if(allocated(this%committed))deallocate(this%committed)
        if(allocated(this%trial))deallocate(this%trial)
        allocate(this%committed(ndof),this%trial(ndof));this%committed=0.0_rk;this%trial=0.0_rk
    end subroutine nonlinear_state_initialize

    subroutine nonlinear_state_begin_trial(this,status)
        class(nonlinear_displacement_state_t),intent(inout)::this
        type(status_t),intent(out)::status
        call status%clear();if(.not.this%is_initialized())then
            call status%set_error(FEM_STATUS_NOT_INITIALIZED,"Nonlinear state initialize edilmedi.");return
        end if
        this%trial=this%committed
    end subroutine nonlinear_state_begin_trial

    subroutine nonlinear_state_set_trial(this,values,status)
        class(nonlinear_displacement_state_t),intent(inout)::this
        real(rk),intent(in)::values(:)
        type(status_t),intent(out)::status
        call status%clear();if(.not.this%is_initialized())then
            call status%set_error(FEM_STATUS_NOT_INITIALIZED,"Nonlinear state initialize edilmedi.");return
        end if
        if(size(values)/=size(this%trial))then
            call status%set_error(FEM_STATUS_INVALID_ARGUMENT,"Trial displacement vector boyutu uyusmuyor.");return
        end if
        this%trial=values
    end subroutine nonlinear_state_set_trial

    subroutine nonlinear_state_commit(this,status)
        class(nonlinear_displacement_state_t),intent(inout)::this
        type(status_t),intent(out)::status
        call status%clear();if(.not.this%is_initialized())then
            call status%set_error(FEM_STATUS_NOT_INITIALIZED,"Nonlinear state initialize edilmedi.");return
        end if
        this%committed=this%trial
    end subroutine nonlinear_state_commit

    subroutine nonlinear_state_revert(this,status)
        class(nonlinear_displacement_state_t),intent(inout)::this
        type(status_t),intent(out)::status
        call status%clear();if(.not.this%is_initialized())then
            call status%set_error(FEM_STATUS_NOT_INITIALIZED,"Nonlinear state initialize edilmedi.");return
        end if
        this%trial=this%committed
    end subroutine nonlinear_state_revert

    pure logical function nonlinear_state_is_initialized(this)
        class(nonlinear_displacement_state_t),intent(in)::this
        nonlinear_state_is_initialized=allocated(this%committed).and.allocated(this%trial)
    end function nonlinear_state_is_initialized

end module fem_nonlinear_state
