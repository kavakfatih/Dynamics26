module fem_linear_static_analysis
    !! V0.5 genel lineer statik analiz surucusu.
    !!
    !! Element formulation dispatch, sparse assembly, nodal load, solve ve
    !! reaction recovery tek akista birlesir. GUI/C API katmani bu modulu
    !! gelecekte opaque project handle uzerinden cagirabilir.
    use fem_kinds, only : rk,id_kind,index_kind
    use fem_ids, only : INVALID_ID
    use fem_model, only : model_t
    use fem_fields, only : FIELD_ID_DISPLACEMENT,FIELD_ID_ROTATION
    use fem_element_registry, only : ELEMENT_TRUSS2,ELEMENT_BEAM2_LINEAR_2D, &
        ELEMENT_PLANE_STRESS_QUAD4,ELEMENT_PLANE_STRAIN_QUAD4,ELEMENT_AXISYM_QUAD4,ELEMENT_SOLID_HEX8, &
        ELEMENT_TOTAL_LAGRANGIAN_HEX8
    use fem_element_dof_map, only : element_dof_map_t
    use fem_sparsity_graph, only : sparsity_graph_t
    use fem_sparse_matrix, only : csr_matrix_t,matrix_properties_t,MATRIX_SYMMETRY_SYMMETRIC,MATRIX_DEFINITENESS_SPD_EXPECTED
    use fem_linear_assembly, only : assemble_stiffness_with_constraints,add_active_equation_load
    use fem_linear_solver, only : linear_solver_options_t,linear_solver_statistics_t,solve_linear_system
    use fem_reactions, only : reaction_vector_t
    use fem_linear_truss, only : truss2_stiffness_3d
    use fem_linear_beam, only : beam2_frame_stiffness_2d
    use fem_linear_continuum, only : quad4_stiffness_plane,quad4_stiffness_axisymmetric,hex8_stiffness, &
        PLANE_MODE_STRESS,PLANE_MODE_STRAIN
    use fem_sections, only : SECTION_TRUSS,SECTION_PLANE,SECTION_BEAM
    use fem_status, only : status_t,FEM_STATUS_INVALID_ARGUMENT,FEM_STATUS_NOT_INITIALIZED
    implicit none
    private

    type, public :: linear_static_result_t
        integer(id_kind), allocatable :: dof_ids(:)
        real(rk), allocatable :: dof_values(:)
        real(rk), allocatable :: active_solution(:)
        type(reaction_vector_t) :: reactions
        type(linear_solver_statistics_t) :: solver_statistics
    contains
        procedure :: clear => linear_static_result_clear
        procedure :: value_of_dof => linear_static_result_value_of_dof
    end type linear_static_result_t

    public :: solve_linear_static
    public :: build_element_map, element_stiffness, validate_active_dof_coverage

contains

    subroutine linear_static_result_clear(this)
        class(linear_static_result_t), intent(inout) :: this
        if (allocated(this%dof_ids)) deallocate(this%dof_ids)
        if (allocated(this%dof_values)) deallocate(this%dof_values)
        if (allocated(this%active_solution)) deallocate(this%active_solution)
        call this%reactions%clear()
        this%solver_statistics=linear_solver_statistics_t()
    end subroutine linear_static_result_clear

    pure real(rk) function linear_static_result_value_of_dof(this,dof_id) result(value)
        class(linear_static_result_t), intent(in) :: this
        integer(id_kind), intent(in) :: dof_id
        integer :: i
        value=0.0_rk
        if (.not. allocated(this%dof_ids)) return
        do i=1,size(this%dof_ids)
            if (this%dof_ids(i)==dof_id) then
                value=this%dof_values(i); return
            end if
        end do
    end function linear_static_result_value_of_dof

    subroutine solve_linear_static(model,options,result,status)
        type(model_t), intent(inout) :: model
        type(linear_solver_options_t), intent(in) :: options
        type(linear_static_result_t), intent(inout) :: result
        type(status_t), intent(out) :: status
        type(element_dof_map_t), allocatable :: maps(:)
        type(sparsity_graph_t) :: graph
        type(csr_matrix_t) :: stiffness
        type(matrix_properties_t) :: props
        real(rk), allocatable :: rhs(:),ke(:,:),fe(:)
        integer :: e,i
        integer(id_kind) :: eq
        integer(index_kind) :: cpos

        call status%clear(); call result%clear()
        if (.not. allocated(model%mesh%elements) .or. .not. allocated(model%dofs%dofs)) then
            call status%set_error(FEM_STATUS_NOT_INITIALIZED,"Linear analysis mesh/DOF sistemi hazir degil."); return
        end if
        call model%renumber(status); if (.not.status%is_ok()) return
        allocate(maps(size(model%mesh%elements)))
        do e=1,size(maps)
            call build_element_map(model,e,maps(e),status)
            if (.not.status%is_ok()) return
        end do
        call validate_active_dof_coverage(model,maps,status); if (.not.status%is_ok()) return
        call graph%build(model%numbering%active_equation_count,maps,status); if (.not.status%is_ok()) return
        props%symmetry=MATRIX_SYMMETRY_SYMMETRIC; props%definiteness=MATRIX_DEFINITENESS_SPD_EXPECTED
        call stiffness%initialize_from_graph(graph,props,status); if (.not.status%is_ok()) return
        allocate(rhs(int(model%numbering%active_equation_count))); rhs=0.0_rk

        do e=1,size(maps)
            call element_stiffness(model,e,ke,status); if (.not.status%is_ok()) return
            allocate(fe(size(ke,1))); fe=0.0_rk
            call assemble_stiffness_with_constraints(stiffness,rhs,maps(e),ke,fe,status)
            if (.not.status%is_ok()) return
            deallocate(ke,fe)
        end do

        if (allocated(model%loads%loads)) then
            do i=1,size(model%loads%loads)
                eq=model%numbering%equation_of(model%loads%loads(i)%dof_id)
                if (eq /= INVALID_ID) then
                    call add_active_equation_load(rhs,eq,model%loads%loads(i)%value,status)
                    if (.not.status%is_ok()) return
                else
                    cpos=model%constraints%find_position_by_dof(model%loads%loads(i)%dof_id)
                    if (cpos == 0_index_kind) then
                        call status%set_error(FEM_STATUS_INVALID_ARGUMENT,"Load DOF ne active equation ne de constraint olarak bulundu.")
                        return
                    end if
                end if
            end do
        end if

        if (size(rhs) == 0) then
            allocate(result%active_solution(0))
            result%solver_statistics%converged=.true.
        else
            call solve_linear_system(stiffness,rhs,result%active_solution,options,result%solver_statistics,status)
            if (.not.status%is_ok()) return
        end if
        call reconstruct_dof_values(model,result,status); if (.not.status%is_ok()) return
        call recover_reactions(model,maps,result,status)
    end subroutine solve_linear_static

    subroutine build_element_map(model,e,map,status)
        type(model_t), intent(in) :: model
        integer, intent(in) :: e
        type(element_dof_map_t), intent(inout) :: map
        type(status_t), intent(out) :: status
        integer(id_kind),parameter :: beam_fields(3)=[FIELD_ID_DISPLACEMENT,FIELD_ID_DISPLACEMENT,FIELD_ID_ROTATION]
        integer,parameter :: beam_comps(3)=[1,2,3]
        integer(index_kind) :: formulation_pos
        formulation_pos=model%element_formulations%find_position(model%mesh%elements(e)%formulation_id)
        if (formulation_pos==0_index_kind) then
            call status%set_error(FEM_STATUS_INVALID_ARGUMENT,"Element formulation ID registry'de bulunamadi.")
            return
        end if
        if (model%element_formulations%elements(formulation_pos)%topology_id /= model%mesh%elements(e)%topology_id) then
            call status%set_error(FEM_STATUS_INVALID_ARGUMENT,"Element formulation ile mesh topology uyusmuyor.")
            return
        end if
        select case(model%mesh%elements(e)%formulation_id)
        case(ELEMENT_TRUSS2)
            call map%build_nodal_field(model%mesh,model%mesh%elements(e)%id,FIELD_ID_DISPLACEMENT,3, &
                model%dofs,model%constraints,model%numbering,status)
        case(ELEMENT_PLANE_STRESS_QUAD4,ELEMENT_PLANE_STRAIN_QUAD4,ELEMENT_AXISYM_QUAD4)
            call map%build_nodal_field(model%mesh,model%mesh%elements(e)%id,FIELD_ID_DISPLACEMENT,2, &
                model%dofs,model%constraints,model%numbering,status)
        case(ELEMENT_SOLID_HEX8,ELEMENT_TOTAL_LAGRANGIAN_HEX8)
            call map%build_nodal_field(model%mesh,model%mesh%elements(e)%id,FIELD_ID_DISPLACEMENT,3, &
                model%dofs,model%constraints,model%numbering,status)
        case(ELEMENT_BEAM2_LINEAR_2D)
            call map%build_nodal_components(model%mesh,model%mesh%elements(e)%id,beam_fields,beam_comps, &
                model%dofs,model%constraints,model%numbering,status)
        case default
            call status%set_error(FEM_STATUS_INVALID_ARGUMENT,"Element formulation lineer statik solver tarafindan desteklenmiyor.")
        end select
    end subroutine build_element_map

    subroutine element_stiffness(model,e,ke,status)
        type(model_t), intent(in) :: model
        integer, intent(in) :: e
        real(rk), allocatable, intent(out) :: ke(:,:)
        type(status_t), intent(out) :: status
        integer(index_kind) :: mpos,spos,npos
        integer :: a
        real(rk) :: x3(3,8),x2(2,8),thickness
        integer(id_kind) :: fid
        fid=model%mesh%elements(e)%formulation_id
        mpos=model%materials%find_position(model%mesh%elements(e)%material_id)
        if (mpos==0_index_kind) then
            allocate(ke(0,0)); call status%set_error(FEM_STATUS_INVALID_ARGUMENT,"Element Material ID registry'de bulunamadi."); return
        end if
        x3=0.0_rk; x2=0.0_rk
        do a=1,size(model%mesh%elements(e)%node_ids)
            npos=model%mesh%find_node_position(model%mesh%elements(e)%node_ids(a))
            x3(:,a)=model%mesh%nodes(npos)%x; x2(:,a)=model%mesh%nodes(npos)%x(1:2)
        end do
        select case(fid)
        case(ELEMENT_TRUSS2)
            spos=model%sections%find_position(model%mesh%elements(e)%section_id)
            if (spos==0_index_kind) then
                allocate(ke(0,0)); call status%set_error(FEM_STATUS_INVALID_ARGUMENT,"TRUSS2 section bulunamadi."); return
            end if
            if (model%sections%sections(spos)%kind /= SECTION_TRUSS) then
                allocate(ke(0,0)); call status%set_error(FEM_STATUS_INVALID_ARGUMENT,"TRUSS2 section kind uyusmuyor."); return
            end if
            allocate(ke(6,6)); call truss2_stiffness_3d(x3(:,1),x3(:,2),model%materials%materials(mpos)%young_modulus, &
                model%sections%sections(spos)%area,ke,status)
        case(ELEMENT_BEAM2_LINEAR_2D)
            spos=model%sections%find_position(model%mesh%elements(e)%section_id)
            if (spos==0_index_kind) then
                allocate(ke(0,0)); call status%set_error(FEM_STATUS_INVALID_ARGUMENT,"BEAM2 section bulunamadi."); return
            end if
            if (model%sections%sections(spos)%kind /= SECTION_BEAM) then
                allocate(ke(0,0)); call status%set_error(FEM_STATUS_INVALID_ARGUMENT,"BEAM2 section kind uyusmuyor."); return
            end if
            allocate(ke(6,6)); call beam2_frame_stiffness_2d(x2(:,1),x2(:,2),model%materials%materials(mpos)%young_modulus, &
                model%sections%sections(spos)%area,model%sections%sections(spos)%iz,ke,status)
        case(ELEMENT_PLANE_STRESS_QUAD4,ELEMENT_PLANE_STRAIN_QUAD4)
            spos=model%sections%find_position(model%mesh%elements(e)%section_id)
            if (spos==0_index_kind) then
                allocate(ke(0,0)); call status%set_error(FEM_STATUS_INVALID_ARGUMENT,"Plane QUAD4 thickness section bulunamadi."); return
            end if
            if (model%sections%sections(spos)%kind /= SECTION_PLANE) then
                allocate(ke(0,0)); call status%set_error(FEM_STATUS_INVALID_ARGUMENT,"Plane QUAD4 section kind uyusmuyor."); return
            end if
            thickness=model%sections%sections(spos)%thickness; allocate(ke(8,8))
            if (fid==ELEMENT_PLANE_STRESS_QUAD4) then
                call quad4_stiffness_plane(x2(:,1:4),model%materials%materials(mpos),thickness,PLANE_MODE_STRESS,ke,status)
            else
                call quad4_stiffness_plane(x2(:,1:4),model%materials%materials(mpos),thickness,PLANE_MODE_STRAIN,ke,status)
            end if
        case(ELEMENT_AXISYM_QUAD4)
            allocate(ke(8,8)); call quad4_stiffness_axisymmetric(x2(:,1:4),model%materials%materials(mpos),ke,status)
        case(ELEMENT_SOLID_HEX8)
            allocate(ke(24,24)); call hex8_stiffness(x3(:,1:8),model%materials%materials(mpos),ke,status)
        case default
            allocate(ke(0,0)); call status%set_error(FEM_STATUS_INVALID_ARGUMENT,"Stiffness formulation desteklenmiyor.")
        end select
    end subroutine element_stiffness

    subroutine validate_active_dof_coverage(model,maps,status)
        type(model_t), intent(in) :: model
        type(element_dof_map_t), intent(in) :: maps(:)
        type(status_t), intent(out) :: status
        integer :: i,e
        logical :: found
        call status%clear()
        do i=1,size(model%numbering%dof_ids)
            if (model%numbering%equation_ids(i)==INVALID_ID) cycle
            found=.false.
            do e=1,size(maps)
                if (any(maps(e)%dof_ids==model%numbering%dof_ids(i))) then
                    found=.true.; exit
                end if
            end do
            if (.not.found) then
                call status%set_error(FEM_STATUS_INVALID_ARGUMENT, &
                    "Aktif bir DOF hicbir element formulation tarafindan kullanilmiyor; unused DOF constrain edilmelidir.")
                return
            end if
        end do
    end subroutine validate_active_dof_coverage

    subroutine reconstruct_dof_values(model,result,status)
        type(model_t), intent(in) :: model
        type(linear_static_result_t), intent(inout) :: result
        type(status_t), intent(out) :: status
        integer :: i
        integer(index_kind) :: cpos
        integer(id_kind) :: eq
        call status%clear()
        allocate(result%dof_ids(size(model%dofs%dofs)),result%dof_values(size(model%dofs%dofs)))
        do i=1,size(model%dofs%dofs)
            result%dof_ids(i)=model%dofs%dofs(i)%id
            cpos=model%constraints%find_position_by_dof(result%dof_ids(i))
            if (cpos/=0_index_kind) then
                result%dof_values(i)=model%constraints%constraints(cpos)%prescribed_value
            else
                eq=model%numbering%equation_of(result%dof_ids(i))
                if (eq==INVALID_ID) then
                    call status%set_error(FEM_STATUS_INVALID_ARGUMENT,"DOF reconstruction equation ID bulamadi."); return
                end if
                result%dof_values(i)=result%active_solution(int(eq)+1)
            end if
        end do
    end subroutine reconstruct_dof_values

    subroutine recover_reactions(model,maps,result,status)
        type(model_t), intent(in) :: model
        type(element_dof_map_t), intent(in) :: maps(:)
        type(linear_static_result_t), intent(inout) :: result
        type(status_t), intent(out) :: status
        real(rk),allocatable :: ke(:,:),fe(:)
        integer :: e,i
        integer(index_kind) :: cpos
        call status%clear(); call result%reactions%initialize(model%constraints)
        do e=1,size(maps)
            call element_stiffness(model,e,ke,status); if (.not.status%is_ok()) return
            allocate(fe(size(ke,1))); fe=0.0_rk
            call result%reactions%accumulate_element(maps(e),ke,fe,result%active_solution,status)
            if (.not.status%is_ok()) return
            deallocate(ke,fe)
        end do
        if (allocated(model%loads%loads)) then
            do i=1,size(model%loads%loads)
                cpos=model%constraints%find_position_by_dof(model%loads%loads(i)%dof_id)
                if (cpos/=0_index_kind) then
                    call result%reactions%add_constrained_external_load(model%loads%loads(i)%dof_id,model%loads%loads(i)%value,status)
                    if (.not.status%is_ok()) return
                end if
            end do
        end if
    end subroutine recover_reactions

end module fem_linear_static_analysis
