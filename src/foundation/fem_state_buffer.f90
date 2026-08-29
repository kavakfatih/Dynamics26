module fem_state_buffer
    !! Nonlinear malzeme ve temas gecmis degiskenleri icin trial/commit/revert
    !! sozlesmesinin V0.1.0 seviyesindeki gercek implementasyonu.
    !!
    !! Newton iterasyonu sirasinda committed state DEGISTIRILMEZ. Constitutive
    !! model trial state uzerinde calisir. Yuk adimi yakinserse trial commit edilir;
    !! adim reddedilirse revert ile committed state geri yuklenir.
    use fem_kinds, only : rk, index_kind
    use fem_status, only : status_t, FEM_STATUS_INVALID_ARGUMENT, &
                           FEM_STATUS_NOT_INITIALIZED
    implicit none
    private

    type, public :: state_buffer_t
        real(rk), allocatable, private :: committed(:)
        real(rk), allocatable, private :: trial(:)
    contains
        procedure :: initialize => state_initialize
        procedure :: begin_trial => state_begin_trial
        procedure :: set_trial_value => state_set_trial_value
        procedure :: committed_value => state_committed_value
        procedure :: trial_value => state_trial_value
        procedure :: commit => state_commit
        procedure :: revert => state_revert
        procedure :: size => state_size
        procedure :: is_initialized => state_is_initialized
    end type state_buffer_t

contains

    subroutine state_initialize(this, number_of_variables, initial_value, status)
        class(state_buffer_t), intent(inout) :: this
        integer(index_kind), intent(in) :: number_of_variables
        real(rk), intent(in), optional :: initial_value
        type(status_t), intent(out) :: status
        real(rk) :: value

        call status%clear()
        if (number_of_variables < 0_index_kind) then
            call status%set_error(FEM_STATUS_INVALID_ARGUMENT, &
                "State degiskeni sayisi negatif olamaz.")
            return
        end if

        value = 0.0_rk
        if (present(initial_value)) value = initial_value

        if (allocated(this%committed)) deallocate(this%committed)
        if (allocated(this%trial)) deallocate(this%trial)
        allocate(this%committed(number_of_variables), this%trial(number_of_variables))
        this%committed = value
        this%trial = value
    end subroutine state_initialize

    subroutine state_begin_trial(this, status)
        class(state_buffer_t), intent(inout) :: this
        type(status_t), intent(out) :: status

        call status%clear()
        if (.not. this%is_initialized()) then
            call status%set_error(FEM_STATUS_NOT_INITIALIZED, &
                "State buffer initialize edilmeden trial baslatilamaz.")
            return
        end if
        this%trial = this%committed
    end subroutine state_begin_trial

    subroutine state_set_trial_value(this, position, value, status)
        class(state_buffer_t), intent(inout) :: this
        integer(index_kind), intent(in) :: position
        real(rk), intent(in) :: value
        type(status_t), intent(out) :: status

        call status%clear()
        if (.not. this%is_initialized()) then
            call status%set_error(FEM_STATUS_NOT_INITIALIZED, &
                "State buffer initialize edilmedi.")
            return
        end if
        if (position < 1_index_kind .or. position > size(this%trial, kind=index_kind)) then
            call status%set_error(FEM_STATUS_INVALID_ARGUMENT, &
                "Trial state indeksi gecerli araligin disinda.")
            return
        end if
        this%trial(position) = value
    end subroutine state_set_trial_value

    real(rk) function state_committed_value(this, position, status) result(value)
        class(state_buffer_t), intent(in) :: this
        integer(index_kind), intent(in) :: position
        type(status_t), intent(out) :: status

        value = 0.0_rk
        call status%clear()
        if (.not. this%is_initialized()) then
            call status%set_error(FEM_STATUS_NOT_INITIALIZED, &
                "State buffer initialize edilmedi.")
            return
        end if
        if (position < 1_index_kind .or. position > size(this%committed, kind=index_kind)) then
            call status%set_error(FEM_STATUS_INVALID_ARGUMENT, &
                "Committed state indeksi gecerli araligin disinda.")
            return
        end if
        value = this%committed(position)
    end function state_committed_value

    real(rk) function state_trial_value(this, position, status) result(value)
        class(state_buffer_t), intent(in) :: this
        integer(index_kind), intent(in) :: position
        type(status_t), intent(out) :: status

        value = 0.0_rk
        call status%clear()
        if (.not. this%is_initialized()) then
            call status%set_error(FEM_STATUS_NOT_INITIALIZED, &
                "State buffer initialize edilmedi.")
            return
        end if
        if (position < 1_index_kind .or. position > size(this%trial, kind=index_kind)) then
            call status%set_error(FEM_STATUS_INVALID_ARGUMENT, &
                "Trial state indeksi gecerli araligin disinda.")
            return
        end if
        value = this%trial(position)
    end function state_trial_value

    subroutine state_commit(this, status)
        class(state_buffer_t), intent(inout) :: this
        type(status_t), intent(out) :: status

        call status%clear()
        if (.not. this%is_initialized()) then
            call status%set_error(FEM_STATUS_NOT_INITIALIZED, &
                "State buffer initialize edilmeden commit yapilamaz.")
            return
        end if
        this%committed = this%trial
    end subroutine state_commit

    subroutine state_revert(this, status)
        class(state_buffer_t), intent(inout) :: this
        type(status_t), intent(out) :: status

        call status%clear()
        if (.not. this%is_initialized()) then
            call status%set_error(FEM_STATUS_NOT_INITIALIZED, &
                "State buffer initialize edilmeden revert yapilamaz.")
            return
        end if
        this%trial = this%committed
    end subroutine state_revert

    pure integer(index_kind) function state_size(this) result(number_of_variables)
        class(state_buffer_t), intent(in) :: this
        if (allocated(this%committed)) then
            number_of_variables = size(this%committed, kind=index_kind)
        else
            number_of_variables = 0_index_kind
        end if
    end function state_size

    pure logical function state_is_initialized(this)
        class(state_buffer_t), intent(in) :: this
        state_is_initialized = allocated(this%committed) .and. allocated(this%trial)
    end function state_is_initialized

end module fem_state_buffer
