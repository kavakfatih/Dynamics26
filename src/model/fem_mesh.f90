module fem_mesh
    !! FEM mesh topolojisinin kalici kimlik tabanli depolama katmani.
    !!
    !! Kritik sozlesme:
    !!   * node_t%id kalici Node ID'dir.
    !!   * nodes(:) icindeki Fortran array konumu Node ID degildir.
    !!   * element connectivity, array konumu degil Node ID saklar.
    !!
    !! V0.2.0'da lookup islemleri bilincli olarak O(n) tutulur. Mesh yeniden
    !! siralama ve hizli hash/index tablolarini daha sonra ekleyebilmek icin
    !! semantik API bugunden ID tabanlidir.
    use fem_kinds,  only : rk, id_kind, index_kind
    use fem_ids,    only : INVALID_ID, id_is_valid
    use fem_status, only : status_t, FEM_STATUS_INVALID_ARGUMENT
    use fem_topology, only : TOPOLOGY_BAR2, TOPOLOGY_QUAD4, TOPOLOGY_HEX8
    implicit none
    private

    type, public :: node_t
        integer(id_kind) :: id = INVALID_ID
        real(rk) :: x(3) = 0.0_rk
    end type node_t

    type, public :: element_t
        integer(id_kind) :: id = INVALID_ID
        integer(id_kind) :: topology_id = INVALID_ID
        integer(id_kind) :: formulation_id = INVALID_ID
        integer(id_kind) :: material_id = INVALID_ID
        integer(id_kind) :: section_id = INVALID_ID
        integer(id_kind) :: orientation_frame_id = INVALID_ID
        integer(id_kind), allocatable :: node_ids(:)
    end type element_t

    type, public :: mesh_t
        type(node_t), allocatable :: nodes(:)
        type(element_t), allocatable :: elements(:)
    contains
        procedure :: clear => mesh_clear
        procedure :: node_count => mesh_node_count
        procedure :: element_count => mesh_element_count
        procedure :: add_node => mesh_add_node
        procedure :: add_element => mesh_add_element
        procedure :: find_node_position => mesh_find_node_position
        procedure :: find_element_position => mesh_find_element_position
        procedure :: validate_connectivity => mesh_validate_connectivity
        procedure :: assign_element_properties => mesh_assign_element_properties
        procedure :: assign_element_formulation => mesh_assign_element_formulation
        procedure :: assign_element_orientation => mesh_assign_element_orientation
    end type mesh_t

contains

    subroutine mesh_clear(this)
        class(mesh_t), intent(inout) :: this
        if (allocated(this%nodes)) deallocate(this%nodes)
        if (allocated(this%elements)) deallocate(this%elements)
    end subroutine mesh_clear

    pure integer(index_kind) function mesh_node_count(this)
        class(mesh_t), intent(in) :: this
        if (allocated(this%nodes)) then
            mesh_node_count = int(size(this%nodes), index_kind)
        else
            mesh_node_count = 0_index_kind
        end if
    end function mesh_node_count

    pure integer(index_kind) function mesh_element_count(this)
        class(mesh_t), intent(in) :: this
        if (allocated(this%elements)) then
            mesh_element_count = int(size(this%elements), index_kind)
        else
            mesh_element_count = 0_index_kind
        end if
    end function mesh_element_count

    pure integer(index_kind) function mesh_find_node_position(this, node_id)
        class(mesh_t), intent(in) :: this
        integer(id_kind), intent(in) :: node_id
        integer :: i

        mesh_find_node_position = 0_index_kind
        if (.not. allocated(this%nodes)) return

        do i = 1, size(this%nodes)
            if (this%nodes(i)%id == node_id) then
                mesh_find_node_position = int(i, index_kind)
                return
            end if
        end do
    end function mesh_find_node_position

    pure integer(index_kind) function mesh_find_element_position(this, element_id)
        class(mesh_t), intent(in) :: this
        integer(id_kind), intent(in) :: element_id
        integer :: i

        mesh_find_element_position = 0_index_kind
        if (.not. allocated(this%elements)) return

        do i = 1, size(this%elements)
            if (this%elements(i)%id == element_id) then
                mesh_find_element_position = int(i, index_kind)
                return
            end if
        end do
    end function mesh_find_element_position

    subroutine mesh_add_node(this, node_id, coordinates, status)
        class(mesh_t), intent(inout) :: this
        integer(id_kind), intent(in) :: node_id
        real(rk), intent(in) :: coordinates(3)
        type(status_t), intent(out) :: status
        type(node_t), allocatable :: tmp(:)
        integer :: old_size

        call status%clear()
        if (.not. id_is_valid(node_id)) then
            call status%set_error(FEM_STATUS_INVALID_ARGUMENT, "Node ID gecersiz.")
            return
        end if
        if (this%find_node_position(node_id) /= 0_index_kind) then
            call status%set_error(FEM_STATUS_INVALID_ARGUMENT, "Ayni Node ID mesh icinde ikinci kez kullanilamaz.")
            return
        end if

        if (.not. allocated(this%nodes)) then
            allocate(this%nodes(1))
            old_size = 0
        else
            old_size = size(this%nodes)
            allocate(tmp(old_size + 1))
            tmp(1:old_size) = this%nodes
            call move_alloc(tmp, this%nodes)
        end if

        this%nodes(old_size + 1)%id = node_id
        this%nodes(old_size + 1)%x = coordinates
    end subroutine mesh_add_node

    subroutine mesh_add_element(this, element_id, topology_id, node_ids, status)
        class(mesh_t), intent(inout) :: this
        integer(id_kind), intent(in) :: element_id
        integer(id_kind), intent(in) :: topology_id
        integer(id_kind), intent(in) :: node_ids(:)
        type(status_t), intent(out) :: status
        type(element_t), allocatable :: tmp(:)
        integer :: old_size, i

        call status%clear()
        if (.not. id_is_valid(element_id)) then
            call status%set_error(FEM_STATUS_INVALID_ARGUMENT, "Element ID gecersiz.")
            return
        end if
        if (this%find_element_position(element_id) /= 0_index_kind) then
            call status%set_error(FEM_STATUS_INVALID_ARGUMENT, "Ayni Element ID mesh icinde ikinci kez kullanilamaz.")
            return
        end if
        if (.not. id_is_valid(topology_id)) then
            call status%set_error(FEM_STATUS_INVALID_ARGUMENT, "Element type tanimli olmali.")
            return
        end if
        if (size(node_ids) < 2) then
            call status%set_error(FEM_STATUS_INVALID_ARGUMENT, "Element en az iki Node ID icermeli.")
            return
        end if
        select case (topology_id)
        case (TOPOLOGY_BAR2)
            if (size(node_ids) /= 2) then
                call status%set_error(FEM_STATUS_INVALID_ARGUMENT, "BAR2 tam olarak iki Node ID gerektirir.")
                return
            end if
        case (TOPOLOGY_QUAD4)
            if (size(node_ids) /= 4) then
                call status%set_error(FEM_STATUS_INVALID_ARGUMENT, "QUAD4 tam olarak dort Node ID gerektirir.")
                return
            end if
        case (TOPOLOGY_HEX8)
            if (size(node_ids) /= 8) then
                call status%set_error(FEM_STATUS_INVALID_ARGUMENT, "HEX8 tam olarak sekiz Node ID gerektirir.")
                return
            end if
        end select

        do i = 1, size(node_ids)
            if (this%find_node_position(node_ids(i)) == 0_index_kind) then
                call status%set_error(FEM_STATUS_INVALID_ARGUMENT, "Element connectivity mesh'te bulunmayan Node ID iceriyor.")
                return
            end if
            if (i > 1) then
                if (any(node_ids(1:i-1) == node_ids(i))) then
                    call status%set_error(FEM_STATUS_INVALID_ARGUMENT, "Bir element connectivity'sinde ayni Node ID tekrarlanamaz.")
                    return
                end if
            end if
        end do

        if (.not. allocated(this%elements)) then
            allocate(this%elements(1))
            old_size = 0
        else
            old_size = size(this%elements)
            allocate(tmp(old_size + 1))
            tmp(1:old_size) = this%elements
            call move_alloc(tmp, this%elements)
        end if

        this%elements(old_size + 1)%id = element_id
        this%elements(old_size + 1)%topology_id = topology_id
        allocate(this%elements(old_size + 1)%node_ids(size(node_ids)))
        this%elements(old_size + 1)%node_ids = node_ids
    end subroutine mesh_add_element

    subroutine mesh_assign_element_properties(this, element_id, material_id, section_id, status)
        class(mesh_t), intent(inout) :: this
        integer(id_kind), intent(in) :: element_id, material_id, section_id
        type(status_t), intent(out) :: status
        integer(index_kind) :: pos

        call status%clear()
        pos = this%find_element_position(element_id)
        if (pos == 0_index_kind) then
            call status%set_error(FEM_STATUS_INVALID_ARGUMENT, "Property assignment icin Element ID bulunamadi.")
            return
        end if
        if (.not. id_is_valid(material_id)) then
            call status%set_error(FEM_STATUS_INVALID_ARGUMENT, "Material ID gecersiz.")
            return
        end if
        ! Section her element ailesi icin zorunlu degildir. INVALID_ID, section
        ! baglantisinin bu element icin kullanilmadigini ifade eder.
        if (section_id /= INVALID_ID .and. .not. id_is_valid(section_id)) then
            call status%set_error(FEM_STATUS_INVALID_ARGUMENT, "Section ID gecersiz.")
            return
        end if

        this%elements(pos)%material_id = material_id
        this%elements(pos)%section_id = section_id
    end subroutine mesh_assign_element_properties


    subroutine mesh_assign_element_formulation(this, element_id, formulation_id, status)
        class(mesh_t), intent(inout) :: this
        integer(id_kind), intent(in) :: element_id, formulation_id
        type(status_t), intent(out) :: status
        integer(index_kind) :: pos
        call status%clear()
        pos=this%find_element_position(element_id)
        if (pos == 0_index_kind) then
            call status%set_error(FEM_STATUS_INVALID_ARGUMENT,"Formulation assignment icin Element ID bulunamadi.")
            return
        end if
        if (.not. id_is_valid(formulation_id)) then
            call status%set_error(FEM_STATUS_INVALID_ARGUMENT,"Element formulation ID gecersiz.")
            return
        end if
        this%elements(pos)%formulation_id=formulation_id
    end subroutine mesh_assign_element_formulation

    subroutine mesh_validate_connectivity(this, status)
        class(mesh_t), intent(in) :: this
        type(status_t), intent(out) :: status
        integer :: e, a

        call status%clear()
        if (.not. allocated(this%elements)) return

        do e = 1, size(this%elements)
            if (.not. allocated(this%elements(e)%node_ids)) then
                call status%set_error(FEM_STATUS_INVALID_ARGUMENT, "Element connectivity allocate edilmemis.")
                return
            end if
            do a = 1, size(this%elements(e)%node_ids)
                if (this%find_node_position(this%elements(e)%node_ids(a)) == 0_index_kind) then
                    call status%set_error(FEM_STATUS_INVALID_ARGUMENT, "Mesh connectivity gecersiz Node ID iceriyor.")
                    return
                end if
            end do
        end do
    end subroutine mesh_validate_connectivity

    subroutine mesh_assign_element_orientation(this, element_id, frame_id, status)
        class(mesh_t), intent(inout) :: this
        integer(id_kind), intent(in) :: element_id, frame_id
        type(status_t), intent(out) :: status
        integer(index_kind) :: pos
        call status%clear()
        pos=this%find_element_position(element_id)
        if(pos==0_index_kind)then
            call status%set_error(FEM_STATUS_INVALID_ARGUMENT,"Orientation assignment icin Element ID bulunamadi.");return
        end if
        if(.not.id_is_valid(frame_id))then
            call status%set_error(FEM_STATUS_INVALID_ARGUMENT,"Orientation frame ID gecersiz.");return
        end if
        this%elements(pos)%orientation_frame_id=frame_id
    end subroutine mesh_assign_element_orientation

end module fem_mesh
