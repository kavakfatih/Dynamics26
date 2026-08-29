module fem_model
    !! V0.2.0 ana model aggregate'i.
    !!
    !! Bu type solver degildir. Mesh, field, DOF, constraint ve equation map
    !! yasam dongusunu tek yerde birlestirerek sonraki element/assembly katmanina
    !! temiz bir model sozlesmesi sunar.
    use fem_kinds,       only : id_kind, index_kind
    use fem_status,      only : status_t, FEM_STATUS_INVALID_ARGUMENT
    use fem_mesh,        only : mesh_t
    use fem_fields,      only : field_registry_t, FIELD_ASSOCIATION_NODE, FIELD_ASSOCIATION_ELEMENT
    use fem_dofs,        only : dof_set_t
    use fem_constraints, only : constraint_set_t
    use fem_numbering,   only : equation_map_t
    use fem_coordinate_frames, only : coordinate_frame_registry_t
    use fem_topology,     only : topology_registry_t
    use fem_sets,         only : set_registry_t, SET_KIND_NODE, SET_KIND_ELEMENT
    use fem_element_registry, only : element_registry_t
    use fem_linear_elastic_material, only : material_registry_t
    use fem_hyperelastic_material, only : hyperelastic_registry_t
    use fem_sections, only : section_registry_t
    use fem_loads, only : nodal_load_set_t
    use fem_contact_types, only : contact_registry_t
    implicit none
    private

    type, public :: model_t
        type(mesh_t) :: mesh
        type(field_registry_t) :: fields
        type(dof_set_t) :: dofs
        type(constraint_set_t) :: constraints
        type(equation_map_t) :: numbering
        type(coordinate_frame_registry_t) :: frames
        type(topology_registry_t) :: topologies
        type(set_registry_t) :: sets
        type(element_registry_t) :: element_formulations
        type(material_registry_t) :: materials
        type(hyperelastic_registry_t) :: hyperelastic_materials
        type(section_registry_t) :: sections
        type(nodal_load_set_t) :: loads
        type(contact_registry_t) :: contacts
    contains
        procedure :: clear => model_clear
        procedure :: initialize_standard_registries => model_initialize_standard_registries
        procedure :: add_node_set => model_add_node_set
        procedure :: add_element_set => model_add_element_set
        procedure :: build_nodal_field_dofs => model_build_nodal_field_dofs
        procedure :: build_element_field_dofs => model_build_element_field_dofs
        procedure :: renumber => model_renumber
        procedure :: assign_element_orientation => model_assign_element_orientation
    end type model_t

contains

    subroutine model_clear(this)
        class(model_t), intent(inout) :: this
        call this%mesh%clear()
        call this%fields%clear()
        call this%dofs%clear()
        call this%constraints%clear()
        call this%numbering%clear()
        call this%frames%clear()
        call this%topologies%clear()
        call this%sets%clear()
        call this%element_formulations%clear()
        call this%materials%clear()
        call this%hyperelastic_materials%clear()
        call this%sections%clear()
        call this%loads%clear()
        call this%contacts%clear()
    end subroutine model_clear

    subroutine model_initialize_standard_registries(this, status)
        class(model_t), intent(inout) :: this
        type(status_t), intent(out) :: status

        call status%clear()
        call this%topologies%register_standard(status)
        if (.not. status%is_ok()) return
        call this%fields%register_standard_structural(status)
        if (.not. status%is_ok()) return
        call this%element_formulations%register_standard(status)
    end subroutine model_initialize_standard_registries

    subroutine model_add_node_set(this, set_id, name, member_ids, status)
        class(model_t), intent(inout) :: this
        integer(id_kind), intent(in) :: set_id
        character(len=*), intent(in) :: name
        integer(id_kind), intent(in) :: member_ids(:)
        type(status_t), intent(out) :: status
        integer :: i

        call status%clear()
        do i = 1, size(member_ids)
            if (this%mesh%find_node_position(member_ids(i)) == 0_index_kind) then
                call status%set_error(FEM_STATUS_INVALID_ARGUMENT, "Node set mesh'te bulunmayan Node ID iceriyor.")
                return
            end if
        end do
        call this%sets%add(set_id, name, SET_KIND_NODE, member_ids, status)
    end subroutine model_add_node_set

    subroutine model_add_element_set(this, set_id, name, member_ids, status)
        class(model_t), intent(inout) :: this
        integer(id_kind), intent(in) :: set_id
        character(len=*), intent(in) :: name
        integer(id_kind), intent(in) :: member_ids(:)
        type(status_t), intent(out) :: status
        integer :: i

        call status%clear()
        do i = 1, size(member_ids)
            if (this%mesh%find_element_position(member_ids(i)) == 0_index_kind) then
                call status%set_error(FEM_STATUS_INVALID_ARGUMENT, "Element set mesh'te bulunmayan Element ID iceriyor.")
                return
            end if
        end do
        call this%sets%add(set_id, name, SET_KIND_ELEMENT, member_ids, status)
    end subroutine model_add_element_set

    subroutine model_build_nodal_field_dofs(this, field_id, status)
        class(model_t), intent(inout) :: this
        integer(id_kind), intent(in) :: field_id
        type(status_t), intent(out) :: status
        integer :: components, i, c
        integer(id_kind) :: dof_id
        integer(index_kind) :: field_pos

        call status%clear()
        field_pos = this%fields%find_position(field_id)
        if (field_pos == 0) then
            call status%set_error(FEM_STATUS_INVALID_ARGUMENT, "DOF olusturmak icin Field ID registry'de bulunamadi.")
            return
        end if
        if (this%fields%fields(field_pos)%association /= FIELD_ASSOCIATION_NODE) then
            call status%set_error(FEM_STATUS_INVALID_ARGUMENT, "Bu yordam yalnizca nodal field icin DOF olusturur.")
            return
        end if
        if (.not. allocated(this%mesh%nodes)) return

        components = this%fields%fields(field_pos)%component_count
        do i = 1, size(this%mesh%nodes)
            do c = 1, components
                call this%dofs%add(this%mesh%nodes(i)%id, field_id, c, dof_id, status)
                if (.not. status%is_ok()) return
            end do
        end do
    end subroutine model_build_nodal_field_dofs


    subroutine model_build_element_field_dofs(this, field_id, status)
        class(model_t), intent(inout) :: this
        integer(id_kind), intent(in) :: field_id
        type(status_t), intent(out) :: status
        integer :: components, i, c
        integer(id_kind) :: dof_id
        integer(index_kind) :: field_pos

        call status%clear()
        field_pos = this%fields%find_position(field_id)
        if (field_pos == 0_index_kind) then
            call status%set_error(FEM_STATUS_INVALID_ARGUMENT, "Element field DOF icin Field ID registry'de bulunamadi.")
            return
        end if
        if (this%fields%fields(field_pos)%association /= FIELD_ASSOCIATION_ELEMENT) then
            call status%set_error(FEM_STATUS_INVALID_ARGUMENT, "Bu yordam yalnizca element-associated field icin DOF olusturur.")
            return
        end if
        if (.not. allocated(this%mesh%elements)) return

        components = this%fields%fields(field_pos)%component_count
        do i = 1, size(this%mesh%elements)
            do c = 1, components
                call this%dofs%add(this%mesh%elements(i)%id, field_id, c, dof_id, status)
                if (.not. status%is_ok()) return
            end do
        end do
    end subroutine model_build_element_field_dofs

    subroutine model_renumber(this, status)
        class(model_t), intent(inout) :: this
        type(status_t), intent(out) :: status
        call this%numbering%build(this%dofs, this%constraints, status)
    end subroutine model_renumber

    subroutine model_assign_element_orientation(this, element_id, frame_id, status)
        class(model_t), intent(inout) :: this
        integer(id_kind), intent(in) :: element_id, frame_id
        type(status_t), intent(out) :: status
        if(this%frames%find_position(frame_id)==0_index_kind)then
            call status%set_error(FEM_STATUS_INVALID_ARGUMENT,"Element orientation icin coordinate frame registry'de bulunamadi.")
            return
        end if
        call this%mesh%assign_element_orientation(element_id,frame_id,status)
    end subroutine model_assign_element_orientation

end module fem_model
