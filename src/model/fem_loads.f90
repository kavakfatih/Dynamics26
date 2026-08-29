module fem_loads
    !! Solver-independent nodal load container. Loads DOF ID ile adreslenir;
    !! equation numbering degisse bile fiziksel yuk tanimi gecerliligini korur.
    use fem_kinds, only : rk, id_kind, index_kind
    use fem_ids, only : INVALID_ID, id_is_valid
    use fem_status, only : status_t, FEM_STATUS_INVALID_ARGUMENT
    implicit none
    private

    type, public :: nodal_load_t
        integer(id_kind) :: id = INVALID_ID
        integer(id_kind) :: dof_id = INVALID_ID
        real(rk) :: value = 0.0_rk
    end type nodal_load_t

    type, public :: nodal_load_set_t
        type(nodal_load_t), allocatable :: loads(:)
        integer(id_kind) :: next_id = 0_id_kind
    contains
        procedure :: clear => nodal_load_set_clear
        procedure :: count => nodal_load_set_count
        procedure :: add => nodal_load_set_add
    end type nodal_load_set_t
contains
    subroutine nodal_load_set_clear(this)
        class(nodal_load_set_t), intent(inout) :: this
        if (allocated(this%loads)) deallocate(this%loads)
        this%next_id=0_id_kind
    end subroutine nodal_load_set_clear

    pure integer(index_kind) function nodal_load_set_count(this)
        class(nodal_load_set_t), intent(in) :: this
        if (allocated(this%loads)) then
            nodal_load_set_count=int(size(this%loads),index_kind)
        else
            nodal_load_set_count=0_index_kind
        end if
    end function nodal_load_set_count

    subroutine nodal_load_set_add(this,dof_id,value,load_id,status)
        class(nodal_load_set_t), intent(inout) :: this
        integer(id_kind), intent(in) :: dof_id
        real(rk), intent(in) :: value
        integer(id_kind), intent(out) :: load_id
        type(status_t), intent(out) :: status
        type(nodal_load_t), allocatable :: tmp(:)
        integer :: n
        call status%clear(); load_id=INVALID_ID
        if (.not. id_is_valid(dof_id)) then
            call status%set_error(FEM_STATUS_INVALID_ARGUMENT,"Nodal load DOF ID gecersiz."); return
        end if
        load_id=this%next_id; this%next_id=this%next_id+1_id_kind
        if (.not. allocated(this%loads)) then
            allocate(this%loads(1)); n=0
        else
            n=size(this%loads); allocate(tmp(n+1)); tmp(1:n)=this%loads
            call move_alloc(tmp,this%loads)
        end if
        this%loads(n+1)=nodal_load_t(load_id,dof_id,value)
    end subroutine nodal_load_set_add
end module fem_loads
