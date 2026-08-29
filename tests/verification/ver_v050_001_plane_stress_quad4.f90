program ver_v050_001_plane_stress_quad4
    !! Tek QUAD4 dikdortgen, uniform x-traction. Bilinear alan analitik affine
    !! plane-stress cozumunu tam yeniden uretmelidir.
    use fem_kinds, only : rk,id_kind,index_kind
    use fem_model, only : model_t
    use fem_fields, only : FIELD_ID_DISPLACEMENT
    use fem_topology, only : TOPOLOGY_QUAD4
    use fem_linear_elastic_material, only : linear_elastic_material_t
    use fem_linear_continuum, only : quad4_stiffness_plane,PLANE_MODE_STRESS
    use fem_element_dof_map, only : element_dof_map_t
    use fem_sparsity_graph, only : sparsity_graph_t
    use fem_sparse_matrix, only : csr_matrix_t,matrix_properties_t,MATRIX_SYMMETRY_SYMMETRIC,MATRIX_DEFINITENESS_SPD_EXPECTED
    use fem_linear_assembly, only : assemble_stiffness_with_constraints,add_active_equation_load
    use fem_linear_solver, only : linear_solver_options_t,linear_solver_statistics_t,solve_linear_system,LINEAR_SOLVER_DENSE_REFERENCE
    use fem_reactions, only : reaction_vector_t
    use fem_status, only : status_t
    use test_support, only : assert_true,assert_close
    implicit none
    type(model_t) :: model
    type(linear_elastic_material_t) :: mat
    type(element_dof_map_t) :: map
    type(sparsity_graph_t) :: graph
    type(csr_matrix_t) :: k
    type(matrix_properties_t) :: props
    type(linear_solver_options_t) :: opt
    type(linear_solver_statistics_t) :: stats
    type(reaction_vector_t) :: reactions
    type(status_t) :: status
    integer(id_kind) :: cid,did
    integer(index_kind) :: pos
    integer(id_kind),parameter :: nid(4)=[10_id_kind,40_id_kind,7_id_kind,99_id_kind]
    integer(id_kind),parameter :: eid=501_id_kind
    real(rk) :: x(2,4),ke(8,8),fe(8),rhs(5),force,e,nu,t,h,l,sigma,ux,uy_top
    real(rk),allocatable :: sol(:)
    integer :: a
    x=reshape([0.0_rk,0.0_rk, 2.0_rk,0.0_rk, 2.0_rk,1.0_rk, 0.0_rk,1.0_rk],[2,4])
    e=70.0e9_rk; nu=0.33_rk; t=0.02_rk; h=1.0_rk; l=2.0_rk; force=1.0e6_rk
    sigma=force/(h*t); ux=sigma*l/e; uy_top=-nu*sigma*h/e
    mat=linear_elastic_material_t(id=1_id_kind,name="Al",young_modulus=e,poisson_ratio=nu,density=2700.0_rk)
    do a=1,4
        call model%mesh%add_node(nid(a),[x(1,a),x(2,a),0.0_rk],status); call assert_true(status%is_ok(),"node")
    end do
    call model%mesh%add_element(eid,TOPOLOGY_QUAD4,nid,status); call assert_true(status%is_ok(),"element")
    call model%initialize_standard_registries(status); call assert_true(status%is_ok(),"registries")
    call model%build_nodal_field_dofs(FIELD_ID_DISPLACEMENT,status); call assert_true(status%is_ok(),"dofs")
    ! Tum z DOF'lari; sol kenar ux; node1 uy sabit.
    do a=1,4
        pos=model%dofs%find_by_address(nid(a),FIELD_ID_DISPLACEMENT,3); did=model%dofs%dofs(pos)%id
        call model%constraints%add(did,0.0_rk,cid,status); call assert_true(status%is_ok(),"z support")
    end do
    do a=1,4,3
        pos=model%dofs%find_by_address(nid(a),FIELD_ID_DISPLACEMENT,1); did=model%dofs%dofs(pos)%id
        call model%constraints%add(did,0.0_rk,cid,status); call assert_true(status%is_ok(),"left ux")
    end do
    pos=model%dofs%find_by_address(nid(1),FIELD_ID_DISPLACEMENT,2); did=model%dofs%dofs(pos)%id
    call model%constraints%add(did,0.0_rk,cid,status); call assert_true(status%is_ok(),"anchor uy")
    call model%renumber(status); call assert_true(status%is_ok(),"numbering")
    call assert_true(model%numbering%active_equation_count==5_index_kind,"five active dofs")
    call map%build_nodal_field(model%mesh,eid,FIELD_ID_DISPLACEMENT,2,model%dofs,model%constraints,model%numbering,status)
    call assert_true(status%is_ok(),"map")
    call graph%build(model%numbering%active_equation_count,[map],status); call assert_true(status%is_ok(),"graph")
    props%symmetry=MATRIX_SYMMETRY_SYMMETRIC; props%definiteness=MATRIX_DEFINITENESS_SPD_EXPECTED
    call k%initialize_from_graph(graph,props,status); call assert_true(status%is_ok(),"K init")
    call quad4_stiffness_plane(x,mat,t,PLANE_MODE_STRESS,ke,status); call assert_true(status%is_ok(),"Ke")
    rhs=0.0_rk; fe=0.0_rk
    call assemble_stiffness_with_constraints(k,rhs,map,ke,fe,status); call assert_true(status%is_ok(),"assembly")
    do a=2,3
        pos=model%dofs%find_by_address(nid(a),FIELD_ID_DISPLACEMENT,1); did=model%dofs%dofs(pos)%id
        call add_active_equation_load(rhs,model%numbering%equation_of(did),0.5_rk*force,status)
        call assert_true(status%is_ok(),"edge load")
    end do
    opt%backend=LINEAR_SOLVER_DENSE_REFERENCE
    call solve_linear_system(k,rhs,sol,opt,stats,status); call assert_true(status%is_ok() .and. stats%converged,"solve")
    pos=model%dofs%find_by_address(nid(2),FIELD_ID_DISPLACEMENT,1); did=model%dofs%dofs(pos)%id
    call assert_close(sol(int(model%numbering%equation_of(did))+1),ux,1.0e-10_rk,1.0e-10_rk,"right ux")
    pos=model%dofs%find_by_address(nid(3),FIELD_ID_DISPLACEMENT,2); did=model%dofs%dofs(pos)%id
    call assert_close(sol(int(model%numbering%equation_of(did))+1),uy_top,1.0e-10_rk,1.0e-10_rk,"top poisson uy")
    call reactions%initialize(model%constraints)
    call reactions%accumulate_element(map,ke,fe,sol,status); call assert_true(status%is_ok(),"reactions")
    ! Sol kenar x reaksiyonlarinin toplami -F.
    sigma=0.0_rk
    do a=1,4,3
        pos=model%dofs%find_by_address(nid(a),FIELD_ID_DISPLACEMENT,1); did=model%dofs%dofs(pos)%id
        sigma=sigma+reactions%value_of(did)
    end do
    call assert_close(sigma,-force,1.0e-5_rk,1.0e-10_rk,"x reaction equilibrium")
    write(*,'(A)') "PASS VER-V050-001 single QUAD4 plane stress traction"
end program ver_v050_001_plane_stress_quad4
