module fem_nonlinear_assembly
    !! Newton-Raphson algoritmasindan BAGIMSIZ nonlinear system evaluator.
    !!
    !! Verilen bir trial active-displacement vectoru icin:
    !!
    !!   f_int(u), K_T(u), f_ext ve R(u)=f_ext-f_int
    !!
    !! assemble edilir. V0.7 burada iterasyon yapmaz; V0.8 Newton/line-search
    !! bu saf evaluator'u cagiracaktir. Bu ayrim rollback ve tangent testlerini
    !! solver algoritmasindan ayirir.
    use fem_kinds, only : rk, id_kind, index_kind
    use fem_ids, only : INVALID_ID
    use fem_model, only : model_t
    use fem_element_registry, only : ELEMENT_TOTAL_LAGRANGIAN_HEX8, ELEMENT_MIXED_UP_HEX8_P0
    use fem_fields, only : FIELD_ID_DISPLACEMENT, FIELD_ID_PRESSURE_P0
    use fem_linear_static_analysis, only : build_element_map
    use fem_element_dof_map, only : element_dof_map_t
    use fem_sparsity_graph, only : sparsity_graph_t
    use fem_sparse_matrix, only : csr_matrix_t, matrix_properties_t, &
        MATRIX_SYMMETRY_SYMMETRIC, MATRIX_SYMMETRY_GENERAL, MATRIX_DEFINITENESS_UNKNOWN
    use fem_contact_assembly, only : contact_assembly_summary_t, assemble_contact_contributions
    use fem_linear_assembly, only : assemble_matrix_by_equation, assemble_vector_by_equation
    use fem_total_lagrangian_hex8, only : total_lagrangian_hex8_result_t, evaluate_total_lagrangian_hex8
    use fem_mixed_up_hex8, only : mixed_up_hex8_result_t, evaluate_mixed_up_hex8
    use fem_status, only : status_t, FEM_STATUS_INVALID_ARGUMENT, FEM_STATUS_SIZE_MISMATCH, &
                           FEM_STATUS_NOT_INITIALIZED
    implicit none
    private

    type, public :: nonlinear_system_t
        type(csr_matrix_t) :: tangent
        real(rk), allocatable :: internal_force(:)
        real(rk), allocatable :: external_force(:)
        real(rk), allocatable :: residual(:)
        real(rk) :: minimum_j = huge(1.0_rk)
        real(rk) :: strain_energy = 0.0_rk
        integer :: displacement_unknown_count = 0
        integer :: pressure_unknown_count = 0
        logical :: has_mixed_pressure = .false.
        real(rk) :: displacement_residual_norm = 0.0_rk
        real(rk) :: pressure_residual_norm = 0.0_rk
        integer :: active_contact_count = 0
        integer :: stick_contact_count = 0
        integer :: slip_contact_count = 0
        real(rk) :: maximum_penetration = 0.0_rk
        real(rk) :: total_contact_normal_force = 0.0_rk
        real(rk) :: total_contact_tangential_force = 0.0_rk
    contains
        procedure :: clear => nonlinear_system_clear
    end type nonlinear_system_t

    public :: evaluate_nonlinear_system

contains

    subroutine nonlinear_system_clear(this)
        class(nonlinear_system_t),intent(inout)::this
        call this%tangent%clear()
        if(allocated(this%internal_force))deallocate(this%internal_force)
        if(allocated(this%external_force))deallocate(this%external_force)
        if(allocated(this%residual))deallocate(this%residual)
        this%minimum_j=huge(1.0_rk)
        this%strain_energy=0.0_rk
        this%displacement_unknown_count=0
        this%pressure_unknown_count=0
        this%has_mixed_pressure=.false.
        this%displacement_residual_norm=0.0_rk
        this%pressure_residual_norm=0.0_rk
        this%active_contact_count=0;this%stick_contact_count=0;this%slip_contact_count=0
        this%maximum_penetration=0.0_rk;this%total_contact_normal_force=0.0_rk
        this%total_contact_tangential_force=0.0_rk
    end subroutine nonlinear_system_clear

    subroutine evaluate_nonlinear_system(model,active_displacement,system,status,load_factor)
        type(model_t),intent(inout)::model
        real(rk),intent(in)::active_displacement(:)
        type(nonlinear_system_t),intent(inout)::system
        type(status_t),intent(out)::status
        real(rk),intent(in),optional::load_factor
        type(element_dof_map_t),allocatable::maps(:)
        type(sparsity_graph_t)::graph
        type(matrix_properties_t)::properties
        type(total_lagrangian_hex8_result_t)::element_result
        type(mixed_up_hex8_result_t)::mixed_result
        real(rk)::reference_coords(3,8),local_u(3,8),local_pressure
        integer::e,a,local_index,component,i
        real(rk)::lambda_load
        integer(index_kind)::node_pos,material_pos,hyper_material_pos
        integer(id_kind)::equation_id
        type(contact_assembly_summary_t)::contact_summary

        call status%clear()
        call system%clear()
        lambda_load=1.0_rk
        if(present(load_factor))lambda_load=load_factor
        if(lambda_load<0.0_rk)then
            call status%set_error(FEM_STATUS_INVALID_ARGUMENT,"Nonlinear load factor negatif olamaz.")
            return
        end if
        if(.not.allocated(model%mesh%elements).or..not.allocated(model%dofs%dofs))then
            call status%set_error(FEM_STATUS_NOT_INITIALIZED,"Nonlinear assembly mesh/DOF sistemi hazir degil.")
            return
        end if
        call model%renumber(status)
        if(.not.status%is_ok())return
        if(size(active_displacement)/=int(model%numbering%active_equation_count))then
            call status%set_error(FEM_STATUS_SIZE_MISMATCH,"Trial active displacement boyutu equation count ile uyusmuyor.")
            return
        end if

        allocate(maps(size(model%mesh%elements)))
        do e=1,size(maps)
            select case(model%mesh%elements(e)%formulation_id)
            case(ELEMENT_TOTAL_LAGRANGIAN_HEX8)
                call build_element_map(model,e,maps(e),status)
            case(ELEMENT_MIXED_UP_HEX8_P0)
                call maps(e)%build_mixed_up_p0(model%mesh,model%mesh%elements(e)%id, &
                    FIELD_ID_DISPLACEMENT,FIELD_ID_PRESSURE_P0,model%dofs,model%constraints,model%numbering,status)
            case default
                call status%set_error(FEM_STATUS_INVALID_ARGUMENT, &
                    "Nonlinear assembly TL_HEX8 veya MIXED_UP_HEX8_P0 destekler; contact ayri subsystem olarak assemble edilir.")
            end select
            if(.not.status%is_ok())return
        end do
        call validate_active_dof_coverage(model,maps,status)
        if(.not.status%is_ok())return
        call classify_active_unknowns(model,system)
        call graph%build(model%numbering%active_equation_count,maps,status)
        if(.not.status%is_ok())return
        properties%symmetry=MATRIX_SYMMETRY_SYMMETRIC
        if(model%contacts%has_friction())properties%symmetry=MATRIX_SYMMETRY_GENERAL
        properties%definiteness=MATRIX_DEFINITENESS_UNKNOWN
        call system%tangent%initialize_from_graph(graph,properties,status)
        if(.not.status%is_ok())return
        allocate(system%internal_force(size(active_displacement)), &
                 system%external_force(size(active_displacement)), &
                 system%residual(size(active_displacement)))
        system%internal_force=0.0_rk
        system%external_force=0.0_rk
        system%residual=0.0_rk

        do e=1,size(maps)
            if(size(model%mesh%elements(e)%node_ids)/=8)then
                call status%set_error(FEM_STATUS_INVALID_ARGUMENT,"Nonlinear HEX8 connectivity 8 node gerektirir.")
                return
            end if
            reference_coords=0.0_rk
            local_u=0.0_rk
            do a=1,8
                node_pos=model%mesh%find_node_position(model%mesh%elements(e)%node_ids(a))
                if(node_pos==0_index_kind)then
                    call status%set_error(FEM_STATUS_INVALID_ARGUMENT,"Nonlinear element Node ID mesh icinde bulunamadi.")
                    return
                end if
                reference_coords(:,a)=model%mesh%nodes(node_pos)%x
                do component=1,3
                    local_index=3*(a-1)+component
                    if(maps(e)%constrained(local_index))then
                        local_u(component,a)=maps(e)%prescribed_values(local_index)
                    else
                        equation_id=maps(e)%equation_ids(local_index)
                        local_u(component,a)=active_displacement(int(equation_id)+1)
                    end if
                end do
            end do
            hyper_material_pos=model%hyperelastic_materials%find_position(model%mesh%elements(e)%material_id)

            select case(model%mesh%elements(e)%formulation_id)
            case(ELEMENT_TOTAL_LAGRANGIAN_HEX8)
                if(hyper_material_pos/=0_index_kind)then
                    call evaluate_total_lagrangian_hex8(reference_coords,local_u, &
                        model%hyperelastic_materials%materials(hyper_material_pos),element_result,status)
                else
                    material_pos=model%materials%find_position(model%mesh%elements(e)%material_id)
                    if(material_pos==0_index_kind)then
                        call status%set_error(FEM_STATUS_INVALID_ARGUMENT,"Nonlinear element Material ID linear/hyperelastic registry'de bulunamadi.")
                        return
                    end if
                    call evaluate_total_lagrangian_hex8(reference_coords,local_u, &
                        model%materials%materials(material_pos),element_result,status)
                end if
                if(.not.status%is_ok())return
                call assemble_matrix_by_equation(system%tangent,maps(e)%equation_ids,element_result%tangent,status)
                if(.not.status%is_ok())return
                call assemble_vector_by_equation(system%internal_force,maps(e)%equation_ids,element_result%internal_force,status)
                if(.not.status%is_ok())return
                system%minimum_j=min(system%minimum_j,element_result%min_j)
                system%strain_energy=system%strain_energy+element_result%strain_energy

            case(ELEMENT_MIXED_UP_HEX8_P0)
                if(hyper_material_pos==0_index_kind)then
                    call status%set_error(FEM_STATUS_INVALID_ARGUMENT,"Mixed u-p HEX8 hyperelastic material gerektirir.")
                    return
                end if
                if(maps(e)%constrained(25))then
                    local_pressure=maps(e)%prescribed_values(25)
                else
                    equation_id=maps(e)%equation_ids(25)
                    local_pressure=active_displacement(int(equation_id)+1)
                end if
                call evaluate_mixed_up_hex8(reference_coords,local_u,local_pressure, &
                    model%hyperelastic_materials%materials(hyper_material_pos),mixed_result,status)
                if(.not.status%is_ok())return
                call assemble_matrix_by_equation(system%tangent,maps(e)%equation_ids,mixed_result%tangent,status)
                if(.not.status%is_ok())return
                call assemble_vector_by_equation(system%internal_force,maps(e)%equation_ids,mixed_result%internal_force,status)
                if(.not.status%is_ok())return
                system%minimum_j=min(system%minimum_j,mixed_result%minimum_j)
                system%strain_energy=system%strain_energy+mixed_result%mixed_energy
            end select
        end do

        if(allocated(model%loads%loads))then
            do i=1,size(model%loads%loads)
                equation_id=model%numbering%equation_of(model%loads%loads(i)%dof_id)
                if(equation_id/=INVALID_ID)then
                    system%external_force(int(equation_id)+1)=system%external_force(int(equation_id)+1)+lambda_load*model%loads%loads(i)%value
                end if
            end do
        end if
        system%residual=system%external_force-system%internal_force
        call assemble_contact_contributions(model,active_displacement,system%tangent,system%residual,contact_summary,status)
        if(.not.status%is_ok())return
        system%active_contact_count=contact_summary%active_count
        system%stick_contact_count=contact_summary%stick_count
        system%slip_contact_count=contact_summary%slip_count
        system%maximum_penetration=contact_summary%maximum_penetration
        system%total_contact_normal_force=contact_summary%total_normal_force
        system%total_contact_tangential_force=contact_summary%total_tangential_force
        call compute_residual_block_norms(model,system)
    end subroutine evaluate_nonlinear_system



    subroutine compute_residual_block_norms(model,system)
        type(model_t),intent(in)::model
        type(nonlinear_system_t),intent(inout)::system
        integer::i,eq_index
        integer(index_kind)::dof_pos
        real(rk)::u2,p2
        u2=0.0_rk;p2=0.0_rk
        do i=1,size(model%numbering%dof_ids)
            if(model%numbering%equation_ids(i)==INVALID_ID)cycle
            eq_index=int(model%numbering%equation_ids(i))+1
            dof_pos=model%dofs%find_position(model%numbering%dof_ids(i))
            if(dof_pos==0_index_kind)cycle
            if(model%dofs%dofs(dof_pos)%field_id==FIELD_ID_PRESSURE_P0)then
                p2=p2+system%residual(eq_index)*system%residual(eq_index)
            else
                u2=u2+system%residual(eq_index)*system%residual(eq_index)
            end if
        end do
        system%displacement_residual_norm=sqrt(max(0.0_rk,u2))
        system%pressure_residual_norm=sqrt(max(0.0_rk,p2))
    end subroutine compute_residual_block_norms

    subroutine validate_active_dof_coverage(model,maps,status)
        type(model_t),intent(in)::model
        type(element_dof_map_t),intent(in)::maps(:)
        type(status_t),intent(out)::status
        integer::i,e
        logical::found
        call status%clear()
        do i=1,size(model%numbering%dof_ids)
            if(model%numbering%equation_ids(i)==INVALID_ID)cycle
            found=.false.
            do e=1,size(maps)
                if(any(maps(e)%dof_ids==model%numbering%dof_ids(i)))then
                    found=.true.;exit
                end if
            end do
            if(.not.found)then
                call status%set_error(FEM_STATUS_INVALID_ARGUMENT, &
                    "Aktif nonlinear DOF hicbir element formulation tarafindan kullanilmiyor.")
                return
            end if
        end do
    end subroutine validate_active_dof_coverage

    subroutine classify_active_unknowns(model,system)
        type(model_t),intent(in)::model
        type(nonlinear_system_t),intent(inout)::system
        integer::i
        integer(index_kind)::dof_pos
        system%displacement_unknown_count=0
        system%pressure_unknown_count=0
        do i=1,size(model%numbering%dof_ids)
            if(model%numbering%equation_ids(i)==INVALID_ID)cycle
            dof_pos=model%dofs%find_position(model%numbering%dof_ids(i))
            if(dof_pos==0_index_kind)cycle
            select case(model%dofs%dofs(dof_pos)%field_id)
            case(FIELD_ID_DISPLACEMENT)
                system%displacement_unknown_count=system%displacement_unknown_count+1
            case(FIELD_ID_PRESSURE_P0)
                system%pressure_unknown_count=system%pressure_unknown_count+1
            end select
        end do
        system%has_mixed_pressure=system%pressure_unknown_count>0
    end subroutine classify_active_unknowns

end module fem_nonlinear_assembly
