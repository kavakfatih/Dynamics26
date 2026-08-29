module fem_element_dof_map
    !! Element-local DOF adreslerini global DOF/equation uzayina baglar.
    !!
    !! Siralama sozlesmesi node-major'dir:
    !!   [node1:c1, node1:c2, ..., node2:c1, ...]
    !!
    !! Burada saklanan equation ID'ler 0-tabanlidir. Fortran array konumlari ise
    !! her zamanki gibi 1-tabanlidir. Constraint'li DOF'lar INVALID_ID equation
    !! degeri tasir ve prescribed_values dizisinde fiziksel sinir degeri tutulur.
    use fem_kinds,       only : rk, id_kind, index_kind
    use fem_ids,         only : INVALID_ID
    use fem_status,      only : status_t, FEM_STATUS_INVALID_ARGUMENT, FEM_STATUS_NOT_INITIALIZED
    use fem_mesh,        only : mesh_t
    use fem_dofs,        only : dof_set_t
    use fem_constraints, only : constraint_set_t
    use fem_numbering,   only : equation_map_t
    implicit none
    private

    type, public :: element_dof_map_t
        integer(id_kind), allocatable :: dof_ids(:)
        integer(id_kind), allocatable :: equation_ids(:)
        logical, allocatable :: constrained(:)
        real(rk), allocatable :: prescribed_values(:)
    contains
        procedure :: clear => element_dof_map_clear
        procedure :: size => element_dof_map_size
        procedure :: build_nodal_field => element_dof_map_build_nodal_field
        procedure :: build_nodal_fields => element_dof_map_build_nodal_fields
        procedure :: build_nodal_components => element_dof_map_build_nodal_components
        procedure :: build_mixed_up_p0 => element_dof_map_build_mixed_up_p0
    end type element_dof_map_t

contains

    subroutine element_dof_map_clear(this)
        class(element_dof_map_t), intent(inout) :: this
        if (allocated(this%dof_ids)) deallocate(this%dof_ids)
        if (allocated(this%equation_ids)) deallocate(this%equation_ids)
        if (allocated(this%constrained)) deallocate(this%constrained)
        if (allocated(this%prescribed_values)) deallocate(this%prescribed_values)
    end subroutine element_dof_map_clear

    pure integer(index_kind) function element_dof_map_size(this)
        class(element_dof_map_t), intent(in) :: this
        if (allocated(this%dof_ids)) then
            element_dof_map_size = int(size(this%dof_ids), index_kind)
        else
            element_dof_map_size = 0_index_kind
        end if
    end function element_dof_map_size

    subroutine element_dof_map_build_nodal_field(this, mesh, element_id, field_id, component_count, &
                                                  dofs, constraints, numbering, status)
        class(element_dof_map_t), intent(inout) :: this
        type(mesh_t), intent(in) :: mesh
        integer(id_kind), intent(in) :: element_id, field_id
        integer, intent(in) :: component_count
        type(dof_set_t), intent(in) :: dofs
        type(constraint_set_t), intent(in) :: constraints
        type(equation_map_t), intent(in) :: numbering
        type(status_t), intent(out) :: status
        integer(index_kind) :: element_pos, dof_pos, constraint_pos
        integer :: a, c, local_index, local_count
        integer(id_kind) :: dof_id

        call status%clear()
        call this%clear()

        if (component_count < 1) then
            call status%set_error(FEM_STATUS_INVALID_ARGUMENT, "Element DOF map component_count en az 1 olmali.")
            return
        end if
        if (.not. allocated(numbering%dof_ids)) then
            call status%set_error(FEM_STATUS_NOT_INITIALIZED, "Equation numbering element DOF map'ten once kurulmalidir.")
            return
        end if

        element_pos = mesh%find_element_position(element_id)
        if (element_pos == 0_index_kind) then
            call status%set_error(FEM_STATUS_INVALID_ARGUMENT, "Element DOF map icin Element ID bulunamadi.")
            return
        end if
        if (.not. allocated(mesh%elements(element_pos)%node_ids)) then
            call status%set_error(FEM_STATUS_INVALID_ARGUMENT, "Element connectivity allocate edilmemis.")
            return
        end if

        local_count = size(mesh%elements(element_pos)%node_ids) * component_count
        allocate(this%dof_ids(local_count), this%equation_ids(local_count), &
                 this%constrained(local_count), this%prescribed_values(local_count))
        this%dof_ids = INVALID_ID
        this%equation_ids = INVALID_ID
        this%constrained = .false.
        this%prescribed_values = 0.0_rk

        local_index = 0
        do a = 1, size(mesh%elements(element_pos)%node_ids)
            do c = 1, component_count
                local_index = local_index + 1
                dof_pos = dofs%find_by_address(mesh%elements(element_pos)%node_ids(a), field_id, c)
                if (dof_pos == 0_index_kind) then
                    call this%clear()
                    call status%set_error(FEM_STATUS_INVALID_ARGUMENT, &
                        "Element node/field/component adresi icin DOF bulunamadi.")
                    return
                end if
                dof_id = dofs%dofs(dof_pos)%id
                this%dof_ids(local_index) = dof_id
                this%equation_ids(local_index) = numbering%equation_of(dof_id)

                constraint_pos = constraints%find_position_by_dof(dof_id)
                if (constraint_pos /= 0_index_kind) then
                    this%constrained(local_index) = .true.
                    this%prescribed_values(local_index) = constraints%constraints(constraint_pos)%prescribed_value
                    if (this%equation_ids(local_index) /= INVALID_ID) then
                        call this%clear()
                        call status%set_error(FEM_STATUS_INVALID_ARGUMENT, &
                            "Constraint'li DOF aktif equation ID tasiyor; numbering sozlesmesi bozuk.")
                        return
                    end if
                else
                    if (this%equation_ids(local_index) == INVALID_ID) then
                        call this%clear()
                        call status%set_error(FEM_STATUS_INVALID_ARGUMENT, &
                            "Serbest DOF equation ID almamis; numbering sozlesmesi bozuk.")
                        return
                    end if
                end if
            end do
        end do
    end subroutine element_dof_map_build_nodal_field


    subroutine element_dof_map_build_nodal_fields(this, mesh, element_id, field_ids, component_counts, &
                                                   dofs, constraints, numbering, status)
        class(element_dof_map_t), intent(inout) :: this
        type(mesh_t), intent(in) :: mesh
        integer(id_kind), intent(in) :: element_id
        integer(id_kind), intent(in) :: field_ids(:)
        integer, intent(in) :: component_counts(:)
        type(dof_set_t), intent(in) :: dofs
        type(constraint_set_t), intent(in) :: constraints
        type(equation_map_t), intent(in) :: numbering
        type(status_t), intent(out) :: status
        integer(index_kind) :: element_pos, dof_pos, constraint_pos
        integer :: a, f, c, local_index, per_node, local_count
        integer(id_kind) :: dof_id

        call status%clear()
        call this%clear()
        if (size(field_ids) < 1 .or. size(field_ids) /= size(component_counts) .or. any(component_counts < 1)) then
            call status%set_error(FEM_STATUS_INVALID_ARGUMENT, "Mixed element DOF map field/component listesi gecersiz.")
            return
        end if
        if (.not. allocated(numbering%dof_ids)) then
            call status%set_error(FEM_STATUS_NOT_INITIALIZED, "Equation numbering element DOF map'ten once kurulmalidir.")
            return
        end if
        element_pos = mesh%find_element_position(element_id)
        if (element_pos == 0_index_kind) then
            call status%set_error(FEM_STATUS_INVALID_ARGUMENT, "Mixed element DOF map icin Element ID bulunamadi.")
            return
        end if
        per_node = sum(component_counts)
        local_count = size(mesh%elements(element_pos)%node_ids) * per_node
        allocate(this%dof_ids(local_count), this%equation_ids(local_count), &
                 this%constrained(local_count), this%prescribed_values(local_count))
        this%dof_ids=INVALID_ID; this%equation_ids=INVALID_ID
        this%constrained=.false.; this%prescribed_values=0.0_rk
        local_index=0
        do a=1,size(mesh%elements(element_pos)%node_ids)
            do f=1,size(field_ids)
                do c=1,component_counts(f)
                    local_index=local_index+1
                    dof_pos=dofs%find_by_address(mesh%elements(element_pos)%node_ids(a),field_ids(f),c)
                    if (dof_pos == 0_index_kind) then
                        call status%set_error(FEM_STATUS_INVALID_ARGUMENT,"Mixed element adresinde gerekli DOF bulunamadi.")
                        return
                    end if
                    dof_id=dofs%dofs(dof_pos)%id
                    this%dof_ids(local_index)=dof_id
                    constraint_pos=constraints%find_position_by_dof(dof_id)
                    if (constraint_pos /= 0_index_kind) then
                        this%constrained(local_index)=.true.
                        this%prescribed_values(local_index)=constraints%constraints(constraint_pos)%prescribed_value
                    else
                        this%equation_ids(local_index)=numbering%equation_of(dof_id)
                        if (this%equation_ids(local_index) == INVALID_ID) then
                            call status%set_error(FEM_STATUS_INVALID_ARGUMENT,"Aktif mixed DOF icin equation ID bulunamadi.")
                            return
                        end if
                    end if
                end do
            end do
        end do
    end subroutine element_dof_map_build_nodal_fields


    subroutine element_dof_map_build_nodal_components(this, mesh, element_id, local_field_ids, &
                                                       local_component_ids, dofs, constraints, numbering, status)
        class(element_dof_map_t), intent(inout) :: this
        type(mesh_t), intent(in) :: mesh
        integer(id_kind), intent(in) :: element_id
        integer(id_kind), intent(in) :: local_field_ids(:)
        integer, intent(in) :: local_component_ids(:)
        type(dof_set_t), intent(in) :: dofs
        type(constraint_set_t), intent(in) :: constraints
        type(equation_map_t), intent(in) :: numbering
        type(status_t), intent(out) :: status
        integer(index_kind) :: element_pos, dof_pos, constraint_pos
        integer :: a, q, local_index, per_node, local_count
        integer(id_kind) :: dof_id

        call status%clear(); call this%clear()
        if (size(local_field_ids) < 1 .or. size(local_field_ids) /= size(local_component_ids) .or. &
            any(local_component_ids < 1)) then
            call status%set_error(FEM_STATUS_INVALID_ARGUMENT, "Local field/component adres listesi gecersiz.")
            return
        end if
        if (.not. allocated(numbering%dof_ids)) then
            call status%set_error(FEM_STATUS_NOT_INITIALIZED, "Equation numbering element DOF map'ten once kurulmalidir.")
            return
        end if
        element_pos=mesh%find_element_position(element_id)
        if (element_pos == 0_index_kind) then
            call status%set_error(FEM_STATUS_INVALID_ARGUMENT, "Component element DOF map icin Element ID bulunamadi.")
            return
        end if
        per_node=size(local_field_ids)
        local_count=size(mesh%elements(element_pos)%node_ids)*per_node
        allocate(this%dof_ids(local_count),this%equation_ids(local_count), &
                 this%constrained(local_count),this%prescribed_values(local_count))
        this%dof_ids=INVALID_ID; this%equation_ids=INVALID_ID
        this%constrained=.false.; this%prescribed_values=0.0_rk
        local_index=0
        do a=1,size(mesh%elements(element_pos)%node_ids)
            do q=1,per_node
                local_index=local_index+1
                dof_pos=dofs%find_by_address(mesh%elements(element_pos)%node_ids(a),local_field_ids(q),local_component_ids(q))
                if (dof_pos == 0_index_kind) then
                    call status%set_error(FEM_STATUS_INVALID_ARGUMENT,"Element local component adresinde DOF bulunamadi.")
                    return
                end if
                dof_id=dofs%dofs(dof_pos)%id; this%dof_ids(local_index)=dof_id
                constraint_pos=constraints%find_position_by_dof(dof_id)
                if (constraint_pos /= 0_index_kind) then
                    this%constrained(local_index)=.true.
                    this%prescribed_values(local_index)=constraints%constraints(constraint_pos)%prescribed_value
                else
                    this%equation_ids(local_index)=numbering%equation_of(dof_id)
                    if (this%equation_ids(local_index) == INVALID_ID) then
                        call status%set_error(FEM_STATUS_INVALID_ARGUMENT,"Aktif local component DOF icin equation ID bulunamadi.")
                        return
                    end if
                end if
            end do
        end do
    end subroutine element_dof_map_build_nodal_components


    subroutine element_dof_map_build_mixed_up_p0(this, mesh, element_id, displacement_field_id, pressure_field_id, &
                                                  dofs, constraints, numbering, status)
        !! HEX8/Q1-P0 local ordering contract:
        !!   [u1x,u1y,u1z,...,u8x,u8y,u8z,p_element]
        !! Pressure DOF element entity ID'si uzerinden adreslenir; Node ID ile
        !! karistirilmaz. Bu explicit siralama mixed block assembly ve testler
        !! icin sabit bir ABI-benzeri element sozlesmesidir.
        class(element_dof_map_t), intent(inout) :: this
        type(mesh_t), intent(in) :: mesh
        integer(id_kind), intent(in) :: element_id, displacement_field_id, pressure_field_id
        type(dof_set_t), intent(in) :: dofs
        type(constraint_set_t), intent(in) :: constraints
        type(equation_map_t), intent(in) :: numbering
        type(status_t), intent(out) :: status
        integer(index_kind) :: element_pos, dof_pos
        integer :: a, c, local_index, local_count
        integer(id_kind) :: dof_id

        call status%clear(); call this%clear()
        if (.not. allocated(numbering%dof_ids)) then
            call status%set_error(FEM_STATUS_NOT_INITIALIZED, "Equation numbering mixed u-p DOF map'ten once kurulmalidir.")
            return
        end if
        element_pos = mesh%find_element_position(element_id)
        if (element_pos == 0_index_kind) then
            call status%set_error(FEM_STATUS_INVALID_ARGUMENT, "Mixed u-p DOF map icin Element ID bulunamadi.")
            return
        end if
        if (.not. allocated(mesh%elements(element_pos)%node_ids)) then
            call status%set_error(FEM_STATUS_INVALID_ARGUMENT, "Mixed u-p element connectivity allocate edilmemis.")
            return
        end if
        if (size(mesh%elements(element_pos)%node_ids) /= 8) then
            call status%set_error(FEM_STATUS_INVALID_ARGUMENT, "Mixed HEX8/P0 tam olarak 8 node gerektirir.")
            return
        end if

        local_count = 8*3 + 1
        allocate(this%dof_ids(local_count), this%equation_ids(local_count), &
                 this%constrained(local_count), this%prescribed_values(local_count))
        this%dof_ids=INVALID_ID; this%equation_ids=INVALID_ID
        this%constrained=.false.; this%prescribed_values=0.0_rk

        local_index = 0
        do a=1,8
            do c=1,3
                local_index=local_index+1
                dof_pos=dofs%find_by_address(mesh%elements(element_pos)%node_ids(a), displacement_field_id, c)
                if (dof_pos == 0_index_kind) then
                    call this%clear()
                    call status%set_error(FEM_STATUS_INVALID_ARGUMENT,"Mixed u-p displacement DOF bulunamadi.")
                    return
                end if
                dof_id=dofs%dofs(dof_pos)%id
                call assign_local_dof(this,local_index,dof_id,dofs,constraints,numbering,status)
                if(.not.status%is_ok())return
            end do
        end do

        dof_pos=dofs%find_by_address(element_id, pressure_field_id, 1)
        if(dof_pos==0_index_kind)then
            call this%clear()
            call status%set_error(FEM_STATUS_INVALID_ARGUMENT,"Mixed u-p element pressure P0 DOF bulunamadi.")
            return
        end if
        dof_id=dofs%dofs(dof_pos)%id
        call assign_local_dof(this,25,dof_id,dofs,constraints,numbering,status)
    end subroutine element_dof_map_build_mixed_up_p0

    subroutine assign_local_dof(this,local_index,dof_id,dofs,constraints,numbering,status)
        class(element_dof_map_t),intent(inout)::this
        integer,intent(in)::local_index
        integer(id_kind),intent(in)::dof_id
        type(dof_set_t),intent(in)::dofs
        type(constraint_set_t),intent(in)::constraints
        type(equation_map_t),intent(in)::numbering
        type(status_t),intent(out)::status
        integer(index_kind)::constraint_pos,dof_pos
        call status%clear()
        dof_pos=dofs%find_position(dof_id)
        if(dof_pos==0_index_kind)then
            call status%set_error(FEM_STATUS_INVALID_ARGUMENT,"Local DOF ID global DOF set icinde bulunamadi.")
            return
        end if
        this%dof_ids(local_index)=dof_id
        constraint_pos=constraints%find_position_by_dof(dof_id)
        if(constraint_pos/=0_index_kind)then
            this%constrained(local_index)=.true.
            this%prescribed_values(local_index)=constraints%constraints(constraint_pos)%prescribed_value
            if(numbering%equation_of(dof_id)/=INVALID_ID)then
                call status%set_error(FEM_STATUS_INVALID_ARGUMENT,"Constraint'li mixed DOF aktif equation tasiyor.")
                return
            end if
        else
            this%equation_ids(local_index)=numbering%equation_of(dof_id)
            if(this%equation_ids(local_index)==INVALID_ID)then
                call status%set_error(FEM_STATUS_INVALID_ARGUMENT,"Aktif mixed DOF equation ID almamis.")
                return
            end if
        end if
    end subroutine assign_local_dof

end module fem_element_dof_map
