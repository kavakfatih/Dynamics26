module fem_constraints
    !! Essential/Dirichlet constraint verisinin DOF-ID tabanli depolanmasi.
    !! Constraint equation numbering'den once tanimlanir; dolayisiyla equation
    !! ID saklamaz ve renumbering sonrasi gecersiz hale gelmez.
    use fem_kinds,  only : rk, id_kind, index_kind
    use fem_ids,    only : INVALID_ID, id_is_valid
    use fem_status, only : status_t, FEM_STATUS_INVALID_ARGUMENT
    implicit none
    private

    type, public :: constraint_t
        integer(id_kind) :: id = INVALID_ID
        integer(id_kind) :: dof_id = INVALID_ID
        real(rk) :: prescribed_value = 0.0_rk
    end type constraint_t

    type, public :: constraint_set_t
        type(constraint_t), allocatable :: constraints(:)
        integer(id_kind) :: next_id = 0_id_kind
    contains
        procedure :: clear => constraint_set_clear
        procedure :: count => constraint_set_count
        procedure :: add => constraint_set_add
        procedure :: is_constrained => constraint_set_is_constrained
        procedure :: find_position_by_dof => constraint_find_position_by_dof
    end type constraint_set_t

contains

    subroutine constraint_set_clear(this)
        class(constraint_set_t), intent(inout) :: this
        if (allocated(this%constraints)) deallocate(this%constraints)
        this%next_id = 0_id_kind
    end subroutine constraint_set_clear

    pure integer(index_kind) function constraint_set_count(this)
        class(constraint_set_t), intent(in) :: this
        if (allocated(this%constraints)) then
            constraint_set_count = int(size(this%constraints), index_kind)
        else
            constraint_set_count = 0_index_kind
        end if
    end function constraint_set_count

    pure integer(index_kind) function constraint_find_position_by_dof(this, dof_id)
        class(constraint_set_t), intent(in) :: this
        integer(id_kind), intent(in) :: dof_id
        integer :: i

        constraint_find_position_by_dof = 0_index_kind
        if (.not. allocated(this%constraints)) return
        do i = 1, size(this%constraints)
            if (this%constraints(i)%dof_id == dof_id) then
                constraint_find_position_by_dof = int(i, index_kind)
                return
            end if
        end do
    end function constraint_find_position_by_dof

    pure logical function constraint_set_is_constrained(this, dof_id)
        class(constraint_set_t), intent(in) :: this
        integer(id_kind), intent(in) :: dof_id
        constraint_set_is_constrained = this%find_position_by_dof(dof_id) /= 0_index_kind
    end function constraint_set_is_constrained

    subroutine constraint_set_add(this, dof_id, prescribed_value, constraint_id, status)
        class(constraint_set_t), intent(inout) :: this
        integer(id_kind), intent(in) :: dof_id
        real(rk), intent(in) :: prescribed_value
        integer(id_kind), intent(out) :: constraint_id
        type(status_t), intent(out) :: status
        type(constraint_t), allocatable :: tmp(:)
        integer :: old_size

        call status%clear()
        constraint_id = INVALID_ID
        if (.not. id_is_valid(dof_id)) then
            call status%set_error(FEM_STATUS_INVALID_ARGUMENT, "Constraint icin DOF ID gecersiz.")
            return
        end if
        if (this%is_constrained(dof_id)) then
            call status%set_error(FEM_STATUS_INVALID_ARGUMENT, "Bir DOF icin birden fazla essential constraint tanimlanamaz.")
            return
        end if

        if (.not. allocated(this%constraints)) then
            allocate(this%constraints(1))
            old_size = 0
        else
            old_size = size(this%constraints)
            allocate(tmp(old_size + 1))
            tmp(1:old_size) = this%constraints
            call move_alloc(tmp, this%constraints)
        end if

        constraint_id = this%next_id
        this%next_id = this%next_id + 1_id_kind
        this%constraints(old_size + 1)%id = constraint_id
        this%constraints(old_size + 1)%dof_id = dof_id
        this%constraints(old_size + 1)%prescribed_value = prescribed_value
    end subroutine constraint_set_add

end module fem_constraints
