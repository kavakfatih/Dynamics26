module fem_modal_analysis
    !! V0.6.0 free-vibration analysis: K phi = lambda M phi.
    use fem_kinds,only:rk,id_kind,index_kind
    use fem_ids,only:INVALID_ID
    use fem_constants,only:FEM_PI
    use fem_model,only:model_t
    use fem_element_registry,only:ELEMENT_TRUSS2,ELEMENT_BEAM2_LINEAR_2D,ELEMENT_PLANE_STRESS_QUAD4, &
        ELEMENT_PLANE_STRAIN_QUAD4,ELEMENT_AXISYM_QUAD4,ELEMENT_SOLID_HEX8
    use fem_element_dof_map,only:element_dof_map_t
    use fem_sparsity_graph,only:sparsity_graph_t
    use fem_sparse_matrix,only:csr_matrix_t,matrix_properties_t,MATRIX_SYMMETRY_SYMMETRIC,MATRIX_DEFINITENESS_SPD_EXPECTED
    use fem_linear_assembly,only:assemble_matrix_by_equation
    use fem_linear_static_analysis,only:build_element_map,element_stiffness,validate_active_dof_coverage
    use fem_structural_mass,only:MASS_CONSISTENT,MASS_LUMPED,truss2_mass_3d,beam2_frame_mass_2d, &
        quad4_mass_plane,quad4_mass_axisymmetric,hex8_mass
    use fem_sections,only:SECTION_TRUSS,SECTION_PLANE,SECTION_BEAM
    use fem_eigen_solver,only:eigen_solver_options_t,solve_generalized_eigen
    use fem_generalized_eigen_utils,only:compute_modal_residuals,estimate_matrix_nullity
    use fem_status,only:status_t,FEM_STATUS_INVALID_ARGUMENT,FEM_STATUS_NOT_INITIALIZED,FEM_STATUS_NUMERICAL_FAILURE
    implicit none; private

    type,public :: modal_analysis_options_t
        type(eigen_solver_options_t) :: eigen
        integer :: mass_kind=MASS_CONSISTENT
    end type
    type,public :: modal_result_t
        real(rk),allocatable :: eigenvalues(:),angular_frequencies(:),frequencies_hz(:)
        real(rk),allocatable :: active_modes(:,:),dof_modes(:,:),residual_norms(:)
        integer(id_kind),allocatable :: dof_ids(:)
        integer :: zero_mode_count=0
    contains
        procedure :: clear=>modal_result_clear
    end type
    public :: solve_modal_analysis,element_mass
contains
    subroutine modal_result_clear(this)
        class(modal_result_t),intent(inout)::this
        if(allocated(this%eigenvalues))deallocate(this%eigenvalues)
        if(allocated(this%angular_frequencies))deallocate(this%angular_frequencies)
        if(allocated(this%frequencies_hz))deallocate(this%frequencies_hz)
        if(allocated(this%active_modes))deallocate(this%active_modes)
        if(allocated(this%dof_modes))deallocate(this%dof_modes)
        if(allocated(this%residual_norms))deallocate(this%residual_norms)
        if(allocated(this%dof_ids))deallocate(this%dof_ids)
        this%zero_mode_count=0
    end subroutine

    subroutine solve_modal_analysis(model,options,result,status)
        type(model_t),intent(inout)::model; type(modal_analysis_options_t),intent(in)::options
        type(modal_result_t),intent(inout)::result; type(status_t),intent(out)::status
        type(element_dof_map_t),allocatable::maps(:); type(sparsity_graph_t)::graph
        type(csr_matrix_t)::ks,ms; type(matrix_properties_t)::props
        real(rk),allocatable::ke(:,:),me(:,:),kd(:,:),md(:,:)
        integer::e,i,j; integer(id_kind)::eq
        call status%clear(); call result%clear()
        if(.not.allocated(model%mesh%elements).or..not.allocated(model%dofs%dofs))then
            call status%set_error(FEM_STATUS_NOT_INITIALIZED,"Modal analysis mesh/DOF sistemi hazir degil.");return
        end if
        if(allocated(model%constraints%constraints))then
            do i=1,size(model%constraints%constraints)
                if(abs(model%constraints%constraints(i)%prescribed_value)>1.0e-14_rk)then
                    call status%set_error(FEM_STATUS_INVALID_ARGUMENT,"Free-vibration modal analysis yalnizca sifir essential constraint kabul eder.");return
                end if
            end do
        end if
        call model%renumber(status);if(.not.status%is_ok())return
        if(model%numbering%active_equation_count<1_index_kind)then
            call status%set_error(FEM_STATUS_INVALID_ARGUMENT,"Modal analysis en az bir active equation gerektirir.");return
        end if
        allocate(maps(size(model%mesh%elements)))
        do e=1,size(maps);call build_element_map(model,e,maps(e),status);if(.not.status%is_ok())return;end do
        call validate_active_dof_coverage(model,maps,status);if(.not.status%is_ok())return
        call graph%build(model%numbering%active_equation_count,maps,status);if(.not.status%is_ok())return
        props%symmetry=MATRIX_SYMMETRY_SYMMETRIC;props%definiteness=MATRIX_DEFINITENESS_SPD_EXPECTED
        call ks%initialize_from_graph(graph,props,status);if(.not.status%is_ok())return
        call ms%initialize_from_graph(graph,props,status);if(.not.status%is_ok())return
        do e=1,size(maps)
            call element_stiffness(model,e,ke,status);if(.not.status%is_ok())return
            call element_mass(model,e,options%mass_kind,me,status);if(.not.status%is_ok())return
            call assemble_matrix_by_equation(ks,maps(e)%equation_ids,ke,status);if(.not.status%is_ok())return
            call assemble_matrix_by_equation(ms,maps(e)%equation_ids,me,status);if(.not.status%is_ok())return
            deallocate(ke,me)
        end do
        call ks%to_dense(kd,status);if(.not.status%is_ok())return
        call ms%to_dense(md,status);if(.not.status%is_ok())return
        result%zero_mode_count=estimate_matrix_nullity(kd)
        if(any([(md(i,i)<=0.0_rk,i=1,size(md,1))]))then
            call status%set_error(FEM_STATUS_NUMERICAL_FAILURE,"Active modal mass matrix sifir/negatif diagonal iceriyor.");return
        end if
        call solve_generalized_eigen(kd,md,options%eigen,result%eigenvalues,result%active_modes,status)
        if(.not.status%is_ok())return
        allocate(result%angular_frequencies(size(result%eigenvalues)),result%frequencies_hz(size(result%eigenvalues)))
        result%angular_frequencies=sqrt(result%eigenvalues)
        result%frequencies_hz=result%angular_frequencies/(2.0_rk*FEM_PI)
        call compute_modal_residuals(kd,md,result%eigenvalues,result%active_modes,result%residual_norms,status)
        if(.not.status%is_ok())return
        allocate(result%dof_ids(size(model%dofs%dofs)),result%dof_modes(size(model%dofs%dofs),size(result%eigenvalues)))
        result%dof_modes=0.0_rk
        do i=1,size(model%dofs%dofs)
            result%dof_ids(i)=model%dofs%dofs(i)%id
            eq=model%numbering%equation_of(result%dof_ids(i))
            if(eq/=INVALID_ID)then
                do j=1,size(result%eigenvalues);result%dof_modes(i,j)=result%active_modes(int(eq)+1,j);end do
            end if
        end do
    end subroutine

    subroutine element_mass(model,e,mass_kind,me,status)
        type(model_t),intent(in)::model;integer,intent(in)::e,mass_kind
        real(rk),allocatable,intent(out)::me(:,:);type(status_t),intent(out)::status
        integer(index_kind)::mpos,spos,npos;integer::a;integer(id_kind)::fid
        real(rk)::x3(3,8),x2(2,8)
        fid=model%mesh%elements(e)%formulation_id
        mpos=model%materials%find_position(model%mesh%elements(e)%material_id)
        if(mpos==0_index_kind)then;allocate(me(0,0));call status%set_error(FEM_STATUS_INVALID_ARGUMENT,"Modal element material bulunamadi.");return;end if
        if(model%materials%materials(mpos)%density<=0.0_rk)then;allocate(me(0,0));call status%set_error(FEM_STATUS_INVALID_ARGUMENT,"Modal analysis material density > 0 gerektirir.");return;end if
        x3=0.0_rk;x2=0.0_rk
        do a=1,size(model%mesh%elements(e)%node_ids);npos=model%mesh%find_node_position(model%mesh%elements(e)%node_ids(a));x3(:,a)=model%mesh%nodes(npos)%x;x2(:,a)=x3(1:2,a);end do
        select case(fid)
        case(ELEMENT_TRUSS2)
            spos=model%sections%find_position(model%mesh%elements(e)%section_id)
            if(spos==0.or.model%sections%sections(spos)%kind/=SECTION_TRUSS)then;allocate(me(0,0));call status%set_error(FEM_STATUS_INVALID_ARGUMENT,"TRUSS2 modal section uyusmuyor.");return;end if
            allocate(me(6,6));call truss2_mass_3d(x3(:,1),x3(:,2),model%materials%materials(mpos)%density,model%sections%sections(spos)%area,mass_kind,me,status)
        case(ELEMENT_BEAM2_LINEAR_2D)
            spos=model%sections%find_position(model%mesh%elements(e)%section_id)
            if(spos==0.or.model%sections%sections(spos)%kind/=SECTION_BEAM)then;allocate(me(0,0));call status%set_error(FEM_STATUS_INVALID_ARGUMENT,"BEAM2 modal section uyusmuyor.");return;end if
            allocate(me(6,6));call beam2_frame_mass_2d(x2(:,1),x2(:,2),model%materials%materials(mpos)%density,model%sections%sections(spos)%area,mass_kind,me,status)
        case(ELEMENT_PLANE_STRESS_QUAD4,ELEMENT_PLANE_STRAIN_QUAD4)
            spos=model%sections%find_position(model%mesh%elements(e)%section_id)
            if(spos==0.or.model%sections%sections(spos)%kind/=SECTION_PLANE)then;allocate(me(0,0));call status%set_error(FEM_STATUS_INVALID_ARGUMENT,"Plane modal section uyusmuyor.");return;end if
            allocate(me(8,8));call quad4_mass_plane(x2(:,1:4),model%materials%materials(mpos)%density,model%sections%sections(spos)%thickness,mass_kind,me,status)
        case(ELEMENT_AXISYM_QUAD4)
            allocate(me(8,8));call quad4_mass_axisymmetric(x2(:,1:4),model%materials%materials(mpos)%density,mass_kind,me,status)
        case(ELEMENT_SOLID_HEX8)
            allocate(me(24,24));call hex8_mass(x3(:,1:8),model%materials%materials(mpos)%density,mass_kind,me,status)
        case default
            allocate(me(0,0));call status%set_error(FEM_STATUS_INVALID_ARGUMENT,"Modal mass formulation desteklenmiyor.")
        end select
    end subroutine
end module fem_modal_analysis
