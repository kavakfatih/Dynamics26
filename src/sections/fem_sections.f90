module fem_sections
    !! Lineer yapisal elemanlar icin kesit/property registry'si.
    use fem_kinds, only : rk, id_kind, index_kind
    use fem_ids, only : INVALID_ID, id_is_valid
    use fem_status, only : status_t, FEM_STATUS_INVALID_ARGUMENT
    implicit none
    private

    integer, parameter, public :: SECTION_TRUSS = 1
    integer, parameter, public :: SECTION_PLANE = 2
    integer, parameter, public :: SECTION_BEAM = 3
    integer, parameter :: SECTION_NAME_LENGTH = 64

    type, public :: section_t
        integer(id_kind) :: id = INVALID_ID
        character(len=SECTION_NAME_LENGTH) :: name = ""
        integer :: kind = 0
        real(rk) :: area = 0.0_rk
        real(rk) :: thickness = 0.0_rk
        real(rk) :: iy = 0.0_rk
        real(rk) :: iz = 0.0_rk
        real(rk) :: torsion_j = 0.0_rk
    contains
        procedure :: validate => section_validate
    end type section_t

    type, public :: section_registry_t
        type(section_t), allocatable :: sections(:)
    contains
        procedure :: clear => section_registry_clear
        procedure :: count => section_registry_count
        procedure :: find_position => section_registry_find_position
        procedure :: add => section_registry_add
    end type section_registry_t

contains

    subroutine section_validate(this, status)
        class(section_t), intent(in) :: this
        type(status_t), intent(out) :: status
        call status%clear()
        if (.not. id_is_valid(this%id) .or. len_trim(this%name) == 0) then
            call status%set_error(FEM_STATUS_INVALID_ARGUMENT, "Section ID/name gecersiz.")
            return
        end if
        select case (this%kind)
        case (SECTION_TRUSS)
            if (this%area <= 0.0_rk) call status%set_error(FEM_STATUS_INVALID_ARGUMENT, "Truss alani pozitif olmali.")
        case (SECTION_PLANE)
            if (this%thickness <= 0.0_rk) call status%set_error(FEM_STATUS_INVALID_ARGUMENT, "Plane thickness pozitif olmali.")
        case (SECTION_BEAM)
            if (this%area <= 0.0_rk .or. this%iz <= 0.0_rk) then
                call status%set_error(FEM_STATUS_INVALID_ARGUMENT, "2B beam icin A ve Iz pozitif olmali.")
            end if
        case default
            call status%set_error(FEM_STATUS_INVALID_ARGUMENT, "Bilinmeyen section kind.")
        end select
    end subroutine section_validate

    subroutine section_registry_clear(this)
        class(section_registry_t), intent(inout) :: this
        if (allocated(this%sections)) deallocate(this%sections)
    end subroutine section_registry_clear

    pure integer(index_kind) function section_registry_count(this)
        class(section_registry_t), intent(in) :: this
        if (allocated(this%sections)) then
            section_registry_count = int(size(this%sections), index_kind)
        else
            section_registry_count = 0_index_kind
        end if
    end function section_registry_count

    pure integer(index_kind) function section_registry_find_position(this, section_id)
        class(section_registry_t), intent(in) :: this
        integer(id_kind), intent(in) :: section_id
        integer :: i
        section_registry_find_position = 0_index_kind
        if (.not. allocated(this%sections)) return
        do i = 1, size(this%sections)
            if (this%sections(i)%id == section_id) then
                section_registry_find_position = int(i, index_kind)
                return
            end if
        end do
    end function section_registry_find_position

    subroutine section_registry_add(this, section, status)
        class(section_registry_t), intent(inout) :: this
        type(section_t), intent(in) :: section
        type(status_t), intent(out) :: status
        type(section_t), allocatable :: tmp(:)
        integer :: n
        call section%validate(status)
        if (.not. status%is_ok()) return
        if (this%find_position(section%id) /= 0_index_kind) then
            call status%set_error(FEM_STATUS_INVALID_ARGUMENT, "Duplicate Section ID reddedildi.")
            return
        end if
        if (.not. allocated(this%sections)) then
            allocate(this%sections(1)); n=0
        else
            n=size(this%sections); allocate(tmp(n+1)); tmp(1:n)=this%sections
            call move_alloc(tmp, this%sections)
        end if
        this%sections(n+1)=section
    end subroutine section_registry_add

end module fem_sections
