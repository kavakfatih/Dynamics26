module fem_fields
    !! FEM alan (field) tanimlarinin merkezi kaydi.
    !!
    !! Field, bir fiziksel bilinmeyen/sonuc ailesini tanimlar. Displacement'in
    !! uc component'i tek field altindadir; pressure tek component'tir. Bu yapi
    !! rotational ve thermal alanlarin solver mimarisini bozmadan eklenmesini
    !! saglar.
    use fem_kinds,  only : id_kind, index_kind
    use fem_ids,    only : INVALID_ID, id_is_valid
    use fem_status, only : status_t, FEM_STATUS_INVALID_ARGUMENT
    implicit none
    private

    integer, parameter, public :: FIELD_ASSOCIATION_NODE = 1
    integer, parameter, public :: FIELD_ASSOCIATION_ELEMENT = 2

    integer(id_kind), parameter, public :: FIELD_ID_DISPLACEMENT = 0_id_kind
    integer(id_kind), parameter, public :: FIELD_ID_PRESSURE     = 1_id_kind
    integer(id_kind), parameter, public :: FIELD_ID_ROTATION     = 2_id_kind
    integer(id_kind), parameter, public :: FIELD_ID_TEMPERATURE  = 3_id_kind
    ! V0.10 mixed u-p HEX8/P0: pressure element bazli tek sabit unknown olarak
    ! tutulur. FIELD_ID_PRESSURE nodal pressure altyapisini geriye donuk korur.
    integer(id_kind), parameter, public :: FIELD_ID_PRESSURE_P0  = 4_id_kind

    integer, parameter :: FIELD_NAME_LENGTH = 64

    type, public :: field_definition_t
        integer(id_kind) :: id = INVALID_ID
        character(len=FIELD_NAME_LENGTH) :: name = ""
        integer :: component_count = 0
        integer :: association = FIELD_ASSOCIATION_NODE
    end type field_definition_t

    type, public :: field_registry_t
        type(field_definition_t), allocatable :: fields(:)
    contains
        procedure :: clear => registry_clear
        procedure :: count => registry_count
        procedure :: add => registry_add
        procedure :: find_position => registry_find_position
        procedure :: get_component_count => registry_get_component_count
        procedure :: register_standard_structural => registry_register_standard_structural
    end type field_registry_t

contains

    subroutine registry_clear(this)
        class(field_registry_t), intent(inout) :: this
        if (allocated(this%fields)) deallocate(this%fields)
    end subroutine registry_clear

    pure integer(index_kind) function registry_count(this)
        class(field_registry_t), intent(in) :: this
        if (allocated(this%fields)) then
            registry_count = int(size(this%fields), index_kind)
        else
            registry_count = 0_index_kind
        end if
    end function registry_count

    pure integer(index_kind) function registry_find_position(this, field_id)
        class(field_registry_t), intent(in) :: this
        integer(id_kind), intent(in) :: field_id
        integer :: i

        registry_find_position = 0_index_kind
        if (.not. allocated(this%fields)) return
        do i = 1, size(this%fields)
            if (this%fields(i)%id == field_id) then
                registry_find_position = int(i, index_kind)
                return
            end if
        end do
    end function registry_find_position

    pure integer function registry_get_component_count(this, field_id)
        class(field_registry_t), intent(in) :: this
        integer(id_kind), intent(in) :: field_id
        integer(index_kind) :: pos

        registry_get_component_count = 0
        pos = this%find_position(field_id)
        if (pos > 0_index_kind) registry_get_component_count = this%fields(pos)%component_count
    end function registry_get_component_count

    subroutine registry_add(this, field_id, name, component_count, association, status)
        class(field_registry_t), intent(inout) :: this
        integer(id_kind), intent(in) :: field_id
        character(len=*), intent(in) :: name
        integer, intent(in) :: component_count
        integer, intent(in) :: association
        type(status_t), intent(out) :: status
        type(field_definition_t), allocatable :: tmp(:)
        integer :: old_size

        call status%clear()
        if (.not. id_is_valid(field_id)) then
            call status%set_error(FEM_STATUS_INVALID_ARGUMENT, "Field ID gecersiz.")
            return
        end if
        if (len_trim(name) == 0 .or. len_trim(name) > FIELD_NAME_LENGTH) then
            call status%set_error(FEM_STATUS_INVALID_ARGUMENT, "Field adi bos veya izin verilen uzunluktan buyuk.")
            return
        end if
        if (component_count < 1) then
            call status%set_error(FEM_STATUS_INVALID_ARGUMENT, "Field component sayisi en az bir olmali.")
            return
        end if
        if (association /= FIELD_ASSOCIATION_NODE .and. association /= FIELD_ASSOCIATION_ELEMENT) then
            call status%set_error(FEM_STATUS_INVALID_ARGUMENT, "Field association gecersiz.")
            return
        end if
        if (this%find_position(field_id) /= 0_index_kind) then
            call status%set_error(FEM_STATUS_INVALID_ARGUMENT, "Ayni Field ID ikinci kez kaydedilemez.")
            return
        end if

        if (.not. allocated(this%fields)) then
            allocate(this%fields(1))
            old_size = 0
        else
            old_size = size(this%fields)
            allocate(tmp(old_size + 1))
            tmp(1:old_size) = this%fields
            call move_alloc(tmp, this%fields)
        end if

        this%fields(old_size + 1)%id = field_id
        this%fields(old_size + 1)%name = trim(name)
        this%fields(old_size + 1)%component_count = component_count
        this%fields(old_size + 1)%association = association
    end subroutine registry_add

    subroutine registry_register_standard_structural(this, status)
        class(field_registry_t), intent(inout) :: this
        type(status_t), intent(out) :: status

        call this%add(FIELD_ID_DISPLACEMENT, "displacement", 3, FIELD_ASSOCIATION_NODE, status)
        if (.not. status%is_ok()) return
        call this%add(FIELD_ID_PRESSURE, "pressure", 1, FIELD_ASSOCIATION_NODE, status)
        if (.not. status%is_ok()) return
        call this%add(FIELD_ID_ROTATION, "rotation", 3, FIELD_ASSOCIATION_NODE, status)
        if (.not. status%is_ok()) return
        call this%add(FIELD_ID_PRESSURE_P0, "pressure_p0", 1, FIELD_ASSOCIATION_ELEMENT, status)
    end subroutine registry_register_standard_structural

end module fem_fields
