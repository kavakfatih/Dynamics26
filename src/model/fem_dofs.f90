module fem_dofs
    !! Degree-of-freedom (DOF) kimlik katmani.
    !!
    !! DOF ID, Node ID'den veya equation ID'den turetilmis bir formulle
    !! hesaplanmaz. Her DOF kendi kalici kimligine sahiptir ve fiziksel adresi
    !! (entity + field + component) ayri metadata olarak tutulur.
    use fem_kinds,  only : id_kind, index_kind
    use fem_ids,    only : INVALID_ID, id_is_valid
    use fem_status, only : status_t, FEM_STATUS_INVALID_ARGUMENT
    implicit none
    private

    type, public :: dof_t
        integer(id_kind) :: id = INVALID_ID
        integer(id_kind) :: entity_id = INVALID_ID
        integer(id_kind) :: field_id = INVALID_ID
        integer :: component = 0
    end type dof_t

    type, public :: dof_set_t
        type(dof_t), allocatable :: dofs(:)
        integer(id_kind) :: next_id = 0_id_kind
    contains
        procedure :: clear => dof_set_clear
        procedure :: count => dof_set_count
        procedure :: add => dof_set_add
        procedure :: find_position => dof_set_find_position
        procedure :: find_by_address => dof_set_find_by_address
    end type dof_set_t

contains

    subroutine dof_set_clear(this)
        class(dof_set_t), intent(inout) :: this
        if (allocated(this%dofs)) deallocate(this%dofs)
        this%next_id = 0_id_kind
    end subroutine dof_set_clear

    pure integer(index_kind) function dof_set_count(this)
        class(dof_set_t), intent(in) :: this
        if (allocated(this%dofs)) then
            dof_set_count = int(size(this%dofs), index_kind)
        else
            dof_set_count = 0_index_kind
        end if
    end function dof_set_count

    pure integer(index_kind) function dof_set_find_position(this, dof_id)
        class(dof_set_t), intent(in) :: this
        integer(id_kind), intent(in) :: dof_id
        integer :: i

        dof_set_find_position = 0_index_kind
        if (.not. allocated(this%dofs)) return
        do i = 1, size(this%dofs)
            if (this%dofs(i)%id == dof_id) then
                dof_set_find_position = int(i, index_kind)
                return
            end if
        end do
    end function dof_set_find_position

    pure integer(index_kind) function dof_set_find_by_address(this, entity_id, field_id, component)
        class(dof_set_t), intent(in) :: this
        integer(id_kind), intent(in) :: entity_id, field_id
        integer, intent(in) :: component
        integer :: i

        dof_set_find_by_address = 0_index_kind
        if (.not. allocated(this%dofs)) return
        do i = 1, size(this%dofs)
            if (this%dofs(i)%entity_id == entity_id .and. &
                this%dofs(i)%field_id == field_id .and. &
                this%dofs(i)%component == component) then
                dof_set_find_by_address = int(i, index_kind)
                return
            end if
        end do
    end function dof_set_find_by_address

    subroutine dof_set_add(this, entity_id, field_id, component, dof_id, status)
        class(dof_set_t), intent(inout) :: this
        integer(id_kind), intent(in) :: entity_id, field_id
        integer, intent(in) :: component
        integer(id_kind), intent(out) :: dof_id
        type(status_t), intent(out) :: status
        type(dof_t), allocatable :: tmp(:)
        integer :: old_size

        call status%clear()
        dof_id = INVALID_ID
        if (.not. id_is_valid(entity_id) .or. .not. id_is_valid(field_id) .or. component < 1) then
            call status%set_error(FEM_STATUS_INVALID_ARGUMENT, "DOF fiziksel adresi gecersiz.")
            return
        end if
        if (this%find_by_address(entity_id, field_id, component) /= 0_index_kind) then
            call status%set_error(FEM_STATUS_INVALID_ARGUMENT, "Ayni entity/field/component icin ikinci DOF olusturulamaz.")
            return
        end if

        if (.not. allocated(this%dofs)) then
            allocate(this%dofs(1))
            old_size = 0
        else
            old_size = size(this%dofs)
            allocate(tmp(old_size + 1))
            tmp(1:old_size) = this%dofs
            call move_alloc(tmp, this%dofs)
        end if

        dof_id = this%next_id
        this%next_id = this%next_id + 1_id_kind
        this%dofs(old_size + 1)%id = dof_id
        this%dofs(old_size + 1)%entity_id = entity_id
        this%dofs(old_size + 1)%field_id = field_id
        this%dofs(old_size + 1)%component = component
    end subroutine dof_set_add

end module fem_dofs
