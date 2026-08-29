program ver_v050_002_beam_cantilever
    !! Tek Euler-Bernoulli beam: tip force icin v=F L^3/(3EI), theta=F L^2/(2EI).
    use fem_kinds, only : rk,id_kind,index_kind
    use fem_model, only : model_t
    use fem_fields, only : FIELD_ID_DISPLACEMENT,FIELD_ID_ROTATION
    use fem_topology, only : TOPOLOGY_BAR2
    use fem_linear_beam, only : beam2_frame_stiffness_2d
    use fem_element_dof_map, only : element_dof_map_t
    use fem_sparsity_graph, only : sparsity_graph_t
    use fem_sparse_matrix, only : csr_matrix_t,matrix_properties_t,MATRIX_SYMMETRY_SYMMETRIC,MATRIX_DEFINITENESS_SPD_EXPECTED
    use fem_linear_assembly, only : assemble_stiffness_with_constraints,add_active_equation_load
    use fem_linear_solver, only : linear_solver_options_t,linear_solver_statistics_t,solve_linear_system,LINEAR_SOLVER_DENSE_REFERENCE
    use fem_status, only : status_t
    use test_support, only : assert_true,assert_close
    implicit none
    type(model_t) :: model
    type(element_dof_map_t) :: map
    type(sparsity_graph_t) :: graph
    type(csr_matrix_t) :: k
    type(matrix_properties_t) :: props
    type(linear_solver_options_t) :: opt
    type(linear_solver_statistics_t) :: stats
    type(status_t) :: status
    integer(id_kind) :: cid,did
    integer(index_kind) :: pos
    integer(id_kind),parameter :: n1=100_id_kind,n2=4_id_kind,eid=8_id_kind
    integer(id_kind),parameter :: local_fields(3)=[FIELD_ID_DISPLACEMENT,FIELD_ID_DISPLACEMENT,FIELD_ID_ROTATION]
    integer,parameter :: local_comps(3)=[1,2,3]
    real(rk) :: ke(6,6),fe(6),rhs(3),e,a,iz,l,f,vexp,rexp
    real(rk),allocatable :: sol(:)
    integer :: n,c
    e=210.0e9_rk; a=2.0e-3_rk; iz=8.0e-6_rk; l=2.5_rk; f=-5000.0_rk
    vexp=f*l**3/(3.0_rk*e*iz); rexp=f*l**2/(2.0_rk*e*iz)
    call model%mesh%add_node(n1,[0.0_rk,0.0_rk,0.0_rk],status); call assert_true(status%is_ok(),"n1")
    call model%mesh%add_node(n2,[l,0.0_rk,0.0_rk],status); call assert_true(status%is_ok(),"n2")
    call model%mesh%add_element(eid,TOPOLOGY_BAR2,[n1,n2],status); call assert_true(status%is_ok(),"beam element")
    call model%initialize_standard_registries(status); call assert_true(status%is_ok(),"registry")
    call model%build_nodal_field_dofs(FIELD_ID_DISPLACEMENT,status); call assert_true(status%is_ok(),"disp dofs")
    call model%build_nodal_field_dofs(FIELD_ID_ROTATION,status); call assert_true(status%is_ok(),"rot dofs")
    ! Kullanilmayan uzay DOF'larini sabitle: uz=0, rx=ry=0 her node. Node1 ux,uy,rz de sabit.
    do n=1,2
        pos=model%dofs%find_by_address(merge(n1,n2,n==1),FIELD_ID_DISPLACEMENT,3); did=model%dofs%dofs(pos)%id
        call model%constraints%add(did,0.0_rk,cid,status)
        do c=1,2
            pos=model%dofs%find_by_address(merge(n1,n2,n==1),FIELD_ID_ROTATION,c); did=model%dofs%dofs(pos)%id
            call model%constraints%add(did,0.0_rk,cid,status)
        end do
    end do
    do c=1,2
        pos=model%dofs%find_by_address(n1,FIELD_ID_DISPLACEMENT,c); did=model%dofs%dofs(pos)%id
        call model%constraints%add(did,0.0_rk,cid,status)
    end do
    pos=model%dofs%find_by_address(n1,FIELD_ID_ROTATION,3); did=model%dofs%dofs(pos)%id
    call model%constraints%add(did,0.0_rk,cid,status)
    call model%renumber(status); call assert_true(status%is_ok(),"numbering")
    call assert_true(model%numbering%active_equation_count==3_index_kind,"beam three active dofs")
    call map%build_nodal_components(model%mesh,eid,local_fields,local_comps,model%dofs,model%constraints,model%numbering,status)
    call assert_true(status%is_ok(),"beam map")
    call graph%build(model%numbering%active_equation_count,[map],status); call assert_true(status%is_ok(),"beam graph")
    props%symmetry=MATRIX_SYMMETRY_SYMMETRIC; props%definiteness=MATRIX_DEFINITENESS_SPD_EXPECTED
    call k%initialize_from_graph(graph,props,status); call assert_true(status%is_ok(),"beam K")
    call beam2_frame_stiffness_2d([0.0_rk,0.0_rk],[l,0.0_rk],e,a,iz,ke,status); call assert_true(status%is_ok(),"beam Ke")
    rhs=0.0_rk; fe=0.0_rk; call assemble_stiffness_with_constraints(k,rhs,map,ke,fe,status); call assert_true(status%is_ok(),"beam assembly")
    pos=model%dofs%find_by_address(n2,FIELD_ID_DISPLACEMENT,2); did=model%dofs%dofs(pos)%id
    call add_active_equation_load(rhs,model%numbering%equation_of(did),f,status); call assert_true(status%is_ok(),"tip force")
    opt%backend=LINEAR_SOLVER_DENSE_REFERENCE
    call solve_linear_system(k,rhs,sol,opt,stats,status); call assert_true(status%is_ok() .and. stats%converged,"beam solve")
    call assert_close(sol(int(model%numbering%equation_of(did))+1),vexp,1.0e-12_rk,1.0e-10_rk,"tip displacement")
    pos=model%dofs%find_by_address(n2,FIELD_ID_ROTATION,3); did=model%dofs%dofs(pos)%id
    call assert_close(sol(int(model%numbering%equation_of(did))+1),rexp,1.0e-12_rk,1.0e-10_rk,"tip rotation")
    write(*,'(A)') "PASS VER-V050-002 beam cantilever"
end program ver_v050_002_beam_cantilever
