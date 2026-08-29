module fem_reactions
    !! Constraint DOF'larindaki reaction kuvvetlerini DOF-ID bazinda toplar.
    use fem_kinds, only : rk, id_kind, index_kind
    use fem_status, only : status_t, FEM_STATUS_INVALID_ARGUMENT, FEM_STATUS_SIZE_MISMATCH
    use fem_constraints, only : constraint_set_t
    use fem_element_dof_map, only : element_dof_map_t
    implicit none
    private

    type, public :: reaction_vector_t
        integer(id_kind), allocatable :: dof_ids(:)
        real(rk), allocatable :: values(:)
    contains
        procedure :: clear => reaction_vector_clear
        procedure :: initialize => reaction_vector_initialize
        procedure :: value_of => reaction_vector_value_of
        procedure :: add_constrained_external_load => reaction_add_external_load
        procedure :: accumulate_element => reaction_accumulate_element
    end type reaction_vector_t

contains

    subroutine reaction_vector_clear(this)
        class(reaction_vector_t), intent(inout) :: this
        if (allocated(this%dof_ids)) deallocate(this%dof_ids)
        if (allocated(this%values)) deallocate(this%values)
    end subroutine reaction_vector_clear

    subroutine reaction_vector_initialize(this, constraints)
        class(reaction_vector_t), intent(inout) :: this
        type(constraint_set_t), intent(in) :: constraints
        integer :: i

        call this%clear()
        if (.not. allocated(constraints%constraints)) then
            allocate(this%dof_ids(0), this%values(0))
            return
        end if
        allocate(this%dof_ids(size(constraints%constraints)), this%values(size(constraints%constraints)))
        do i = 1, size(constraints%constraints)
            this%dof_ids(i) = constraints%constraints(i)%dof_id
        end do
        this%values = 0.0_rk
    end subroutine reaction_vector_initialize

    pure real(rk) function reaction_vector_value_of(this, dof_id)
        class(reaction_vector_t), intent(in) :: this
        integer(id_kind), intent(in) :: dof_id
        integer :: i
        reaction_vector_value_of = 0.0_rk
        if (.not. allocated(this%dof_ids)) return
        do i = 1, size(this%dof_ids)
            if (this%dof_ids(i) == dof_id) then
                reaction_vector_value_of = this%values(i)
                return
            end if
        end do
    end function reaction_vector_value_of

    subroutine reaction_add_external_load(this, dof_id, load_value, status)
        class(reaction_vector_t), intent(inout) :: this
        integer(id_kind), intent(in) :: dof_id
        real(rk), intent(in) :: load_value
        type(status_t), intent(out) :: status
        integer(index_kind) :: pos

        call status%clear()
        pos = find_dof(this, dof_id)
        if (pos == 0_index_kind) then
            call status%set_error(FEM_STATUS_INVALID_ARGUMENT, "Reaction external load yalnizca constraint'li DOF'a eklenebilir.")
            return
        end if
        ! R_support = f_int - f_ext. Constraint DOF'taki harici yuk reaction'dan
        ! cikarilir; element internal katkisi accumulate_element ile eklenir.
        this%values(int(pos)) = this%values(int(pos)) - load_value
    end subroutine reaction_add_external_load

    subroutine reaction_accumulate_element(this, map, local_matrix, local_load, active_solution, status)
        class(reaction_vector_t), intent(inout) :: this
        type(element_dof_map_t), intent(in) :: map
        real(rk), intent(in) :: local_matrix(:, :)
        real(rk), intent(in) :: local_load(:)
        real(rk), intent(in) :: active_solution(:)
        type(status_t), intent(out) :: status
        real(rk), allocatable :: local_u(:), local_balance(:)
        integer(index_kind) :: pos
        integer :: a

        call status%clear()
        if (.not. allocated(map%dof_ids) .or. size(local_matrix,1) /= size(map%dof_ids) .or. &
            size(local_matrix,2) /= size(map%dof_ids) .or. size(local_load) /= size(map%dof_ids)) then
            call status%set_error(FEM_STATUS_SIZE_MISMATCH, "Reaction recovery local matrix/vector boyutu DOF map ile uyusmuyor.")
            return
        end if
        allocate(local_u(size(map%dof_ids)), local_balance(size(map%dof_ids)))
        do a = 1, size(map%dof_ids)
            if (map%constrained(a)) then
                local_u(a) = map%prescribed_values(a)
            else
                if (map%equation_ids(a) < 0_id_kind .or. map%equation_ids(a) >= int(size(active_solution), id_kind)) then
                    call status%set_error(FEM_STATUS_INVALID_ARGUMENT, "Reaction recovery active equation ID solution araligi disinda.")
                    return
                end if
                local_u(a) = active_solution(int(map%equation_ids(a)) + 1)
            end if
        end do
        local_balance = matmul(local_matrix, local_u) - local_load
        do a = 1, size(map%dof_ids)
            if (.not. map%constrained(a)) cycle
            pos = find_dof(this, map%dof_ids(a))
            if (pos == 0_index_kind) then
                call status%set_error(FEM_STATUS_INVALID_ARGUMENT, "Reaction vector constraint DOF'u icermiyor.")
                return
            end if
            this%values(int(pos)) = this%values(int(pos)) + local_balance(a)
        end do
    end subroutine reaction_accumulate_element

    pure integer(index_kind) function find_dof(this, dof_id)
        class(reaction_vector_t), intent(in) :: this
        integer(id_kind), intent(in) :: dof_id
        integer :: i
        find_dof = 0_index_kind
        if (.not. allocated(this%dof_ids)) return
        do i = 1, size(this%dof_ids)
            if (this%dof_ids(i) == dof_id) then
                find_dof = int(i, index_kind)
                return
            end if
        end do
    end function find_dof

end module fem_reactions
