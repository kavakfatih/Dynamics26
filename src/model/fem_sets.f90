module fem_sets
    !! Node/element selection set'leri icin kalici ID tabanli veri modeli.
    !! GUI selection, load/BC assignment ve post-processing gruplari ileride bu
    !! katmani kullanabilir. Set uyeleri array position degil entity ID'dir.
    use fem_kinds,  only : id_kind, index_kind
    use fem_ids,    only : INVALID_ID, id_is_valid
    use fem_status, only : status_t, FEM_STATUS_INVALID_ARGUMENT
    implicit none
    private

    integer, parameter, public :: SET_KIND_NODE = 1
    integer, parameter, public :: SET_KIND_ELEMENT = 2
    integer, parameter :: SET_NAME_LENGTH = 64

    type, public :: entity_set_t
        integer(id_kind) :: id = INVALID_ID
        character(len=SET_NAME_LENGTH) :: name = ""
        integer :: kind = 0
        integer(id_kind), allocatable :: member_ids(:)
    contains
        procedure :: contains => entity_set_contains
    end type entity_set_t

    type, public :: set_registry_t
        type(entity_set_t), allocatable :: sets(:)
    contains
        procedure :: clear => set_registry_clear
        procedure :: count => set_registry_count
        procedure :: add => set_registry_add
        procedure :: find_position => set_registry_find_position
    end type set_registry_t

contains

    pure logical function entity_set_contains(this, entity_id)
        class(entity_set_t), intent(in) :: this
        integer(id_kind), intent(in) :: entity_id
        entity_set_contains = .false.
        if (.not. allocated(this%member_ids)) return
        entity_set_contains = any(this%member_ids == entity_id)
    end function entity_set_contains

    subroutine set_registry_clear(this)
        class(set_registry_t), intent(inout) :: this
        if (allocated(this%sets)) deallocate(this%sets)
    end subroutine set_registry_clear

    pure integer(index_kind) function set_registry_count(this)
        class(set_registry_t), intent(in) :: this
        if (allocated(this%sets)) then
            set_registry_count = int(size(this%sets), index_kind)
        else
            set_registry_count = 0_index_kind
        end if
    end function set_registry_count

    pure integer(index_kind) function set_registry_find_position(this, set_id)
        class(set_registry_t), intent(in) :: this
        integer(id_kind), intent(in) :: set_id
        integer :: i

        set_registry_find_position = 0_index_kind
        if (.not. allocated(this%sets)) return
        do i = 1, size(this%sets)
            if (this%sets(i)%id == set_id) then
                set_registry_find_position = int(i, index_kind)
                return
            end if
        end do
    end function set_registry_find_position

    subroutine set_registry_add(this, set_id, name, kind, member_ids, status)
        class(set_registry_t), intent(inout) :: this
        integer(id_kind), intent(in) :: set_id
        character(len=*), intent(in) :: name
        integer, intent(in) :: kind
        integer(id_kind), intent(in) :: member_ids(:)
        type(status_t), intent(out) :: status
        type(entity_set_t), allocatable :: tmp(:)
        integer :: old_size, i

        call status%clear()
        if (.not. id_is_valid(set_id) .or. len_trim(name) == 0 .or. &
            (kind /= SET_KIND_NODE .and. kind /= SET_KIND_ELEMENT)) then
            call status%set_error(FEM_STATUS_INVALID_ARGUMENT, "Entity set tanimi gecersiz.")
            return
        end if
        if (len_trim(name) > SET_NAME_LENGTH .or. size(member_ids) < 1) then
            call status%set_error(FEM_STATUS_INVALID_ARGUMENT, "Entity set adi veya uye listesi gecersiz.")
            return
        end if
        if (this%find_position(set_id) /= 0_index_kind) then
            call status%set_error(FEM_STATUS_INVALID_ARGUMENT, "Duplicate set ID reddedildi.")
            return
        end if
        do i = 1, size(member_ids)
            if (.not. id_is_valid(member_ids(i))) then
                call status%set_error(FEM_STATUS_INVALID_ARGUMENT, "Set icinde gecersiz entity ID var.")
                return
            end if
            if (i > 1) then
                if (any(member_ids(1:i-1) == member_ids(i))) then
                    call status%set_error(FEM_STATUS_INVALID_ARGUMENT, "Set icinde duplicate entity ID reddedildi.")
                    return
                end if
            end if
        end do

        if (.not. allocated(this%sets)) then
            allocate(this%sets(1))
            old_size = 0
        else
            old_size = size(this%sets)
            allocate(tmp(old_size + 1))
            tmp(1:old_size) = this%sets
            call move_alloc(tmp, this%sets)
        end if
        this%sets(old_size + 1)%id = set_id
        this%sets(old_size + 1)%name = trim(name)
        this%sets(old_size + 1)%kind = kind
        allocate(this%sets(old_size + 1)%member_ids(size(member_ids)))
        this%sets(old_size + 1)%member_ids = member_ids
    end subroutine set_registry_add

end module fem_sets
