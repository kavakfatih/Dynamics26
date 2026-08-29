module fem_topology
    !! Element topolojisini formulation'dan ayiran metadata registry'si.
    !!
    !! V0.2.0'da topology yalnizca dugum sayisi ve topolojik boyut gibi temel
    !! bilgileri tasir. Shape function/Jacobian gibi matematik V0.3 element
    !! kernel'inde gelecektir.
    use fem_kinds,  only : id_kind, index_kind
    use fem_ids,    only : INVALID_ID, id_is_valid
    use fem_status, only : status_t, FEM_STATUS_INVALID_ARGUMENT
    implicit none
    private

    integer(id_kind), parameter, public :: TOPOLOGY_BAR2  = 101_id_kind
    integer(id_kind), parameter, public :: TOPOLOGY_QUAD4 = 201_id_kind
    integer(id_kind), parameter, public :: TOPOLOGY_HEX8  = 301_id_kind

    integer, parameter :: TOPOLOGY_NAME_LENGTH = 32

    type, public :: topology_definition_t
        integer(id_kind) :: id = INVALID_ID
        character(len=TOPOLOGY_NAME_LENGTH) :: name = ""
        integer :: topological_dimension = 0
        integer :: node_count = 0
    end type topology_definition_t

    type, public :: topology_registry_t
        type(topology_definition_t), allocatable :: topologies(:)
    contains
        procedure :: clear => topology_registry_clear
        procedure :: count => topology_registry_count
        procedure :: add => topology_registry_add
        procedure :: find_position => topology_registry_find_position
        procedure :: register_standard => topology_registry_register_standard
    end type topology_registry_t

contains

    subroutine topology_registry_clear(this)
        class(topology_registry_t), intent(inout) :: this
        if (allocated(this%topologies)) deallocate(this%topologies)
    end subroutine topology_registry_clear

    pure integer(index_kind) function topology_registry_count(this)
        class(topology_registry_t), intent(in) :: this
        if (allocated(this%topologies)) then
            topology_registry_count = int(size(this%topologies), index_kind)
        else
            topology_registry_count = 0_index_kind
        end if
    end function topology_registry_count

    pure integer(index_kind) function topology_registry_find_position(this, topology_id)
        class(topology_registry_t), intent(in) :: this
        integer(id_kind), intent(in) :: topology_id
        integer :: i

        topology_registry_find_position = 0_index_kind
        if (.not. allocated(this%topologies)) return
        do i = 1, size(this%topologies)
            if (this%topologies(i)%id == topology_id) then
                topology_registry_find_position = int(i, index_kind)
                return
            end if
        end do
    end function topology_registry_find_position

    subroutine topology_registry_add(this, topology_id, name, dimension, node_count, status)
        class(topology_registry_t), intent(inout) :: this
        integer(id_kind), intent(in) :: topology_id
        character(len=*), intent(in) :: name
        integer, intent(in) :: dimension, node_count
        type(status_t), intent(out) :: status
        type(topology_definition_t), allocatable :: tmp(:)
        integer :: old_size

        call status%clear()
        if (.not. id_is_valid(topology_id) .or. len_trim(name) == 0 .or. &
            dimension < 1 .or. dimension > 3 .or. node_count < 2) then
            call status%set_error(FEM_STATUS_INVALID_ARGUMENT, "Topology tanimi gecersiz.")
            return
        end if
        if (len_trim(name) > TOPOLOGY_NAME_LENGTH) then
            call status%set_error(FEM_STATUS_INVALID_ARGUMENT, "Topology adi izin verilen uzunluktan buyuk.")
            return
        end if
        if (this%find_position(topology_id) /= 0_index_kind) then
            call status%set_error(FEM_STATUS_INVALID_ARGUMENT, "Duplicate topology ID reddedildi.")
            return
        end if

        if (.not. allocated(this%topologies)) then
            allocate(this%topologies(1))
            old_size = 0
        else
            old_size = size(this%topologies)
            allocate(tmp(old_size + 1))
            tmp(1:old_size) = this%topologies
            call move_alloc(tmp, this%topologies)
        end if
        this%topologies(old_size + 1)%id = topology_id
        this%topologies(old_size + 1)%name = trim(name)
        this%topologies(old_size + 1)%topological_dimension = dimension
        this%topologies(old_size + 1)%node_count = node_count
    end subroutine topology_registry_add

    subroutine topology_registry_register_standard(this, status)
        class(topology_registry_t), intent(inout) :: this
        type(status_t), intent(out) :: status

        call this%add(TOPOLOGY_BAR2, "BAR2", 1, 2, status)
        if (.not. status%is_ok()) return
        call this%add(TOPOLOGY_QUAD4, "QUAD4", 2, 4, status)
        if (.not. status%is_ok()) return
        call this%add(TOPOLOGY_HEX8, "HEX8", 3, 8, status)
    end subroutine topology_registry_register_standard

end module fem_topology
