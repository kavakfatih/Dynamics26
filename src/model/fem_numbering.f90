module fem_numbering
    !! Deterministic equation numbering.
    !!
    !! Equation ID yalnizca solver'in aktif bilinmeyen uzayini tanimlar.
    !! Constraint'li DOF'lara equation ID verilmez (INVALID_ID). Numbering,
    !! DOF storage sirasi uzerinden deterministik olarak 0'dan baslar.
    use fem_kinds,       only : id_kind, index_kind
    use fem_ids,         only : INVALID_ID
    use fem_status,      only : status_t, FEM_STATUS_INVALID_ARGUMENT
    use fem_dofs,        only : dof_set_t
    use fem_constraints, only : constraint_set_t
    implicit none
    private

    type, public :: equation_map_t
        integer(id_kind), allocatable :: dof_ids(:)
        integer(id_kind), allocatable :: equation_ids(:)
        integer(index_kind) :: active_equation_count = 0_index_kind
    contains
        procedure :: clear => equation_map_clear
        procedure :: build => equation_map_build
        procedure :: equation_of => equation_map_equation_of
    end type equation_map_t

contains

    subroutine equation_map_clear(this)
        class(equation_map_t), intent(inout) :: this
        if (allocated(this%dof_ids)) deallocate(this%dof_ids)
        if (allocated(this%equation_ids)) deallocate(this%equation_ids)
        this%active_equation_count = 0_index_kind
    end subroutine equation_map_clear

    subroutine equation_map_build(this, dofs, constraints, status)
        class(equation_map_t), intent(inout) :: this
        type(dof_set_t), intent(in) :: dofs
        type(constraint_set_t), intent(in) :: constraints
        type(status_t), intent(out) :: status
        integer(id_kind) :: next_equation
        integer :: i

        call status%clear()
        call this%clear()

        if (.not. allocated(dofs%dofs)) return
        allocate(this%dof_ids(size(dofs%dofs)))
        allocate(this%equation_ids(size(dofs%dofs)))

        next_equation = 0_id_kind
        do i = 1, size(dofs%dofs)
            this%dof_ids(i) = dofs%dofs(i)%id
            if (constraints%is_constrained(dofs%dofs(i)%id)) then
                this%equation_ids(i) = INVALID_ID
            else
                this%equation_ids(i) = next_equation
                next_equation = next_equation + 1_id_kind
            end if
        end do
        this%active_equation_count = int(next_equation, index_kind)

        ! Constraint set icindeki her DOF gercekten mevcut olmali. Bu kontrol,
        ! dangling constraint'in sessizce numbering disinda kalmasini engeller.
        if (allocated(constraints%constraints)) then
            do i = 1, size(constraints%constraints)
                if (dofs%find_position(constraints%constraints(i)%dof_id) == 0_index_kind) then
                    call this%clear()
                    call status%set_error(FEM_STATUS_INVALID_ARGUMENT, "Constraint mevcut olmayan DOF ID'ye bagli.")
                    return
                end if
            end do
        end if
    end subroutine equation_map_build

    pure integer(id_kind) function equation_map_equation_of(this, dof_id)
        class(equation_map_t), intent(in) :: this
        integer(id_kind), intent(in) :: dof_id
        integer :: i

        equation_map_equation_of = INVALID_ID
        if (.not. allocated(this%dof_ids)) return
        do i = 1, size(this%dof_ids)
            if (this%dof_ids(i) == dof_id) then
                equation_map_equation_of = this%equation_ids(i)
                return
            end if
        end do
    end function equation_map_equation_of

end module fem_numbering
