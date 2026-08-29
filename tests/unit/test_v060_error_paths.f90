program test_v060_error_paths
    use fem_kinds,only:rk,id_kind,index_kind
    use fem_model,only:model_t
    use fem_fields,only:FIELD_ID_DISPLACEMENT
    use fem_topology,only:TOPOLOGY_BAR2
    use fem_element_registry,only:ELEMENT_TRUSS2
    use fem_linear_elastic_material,only:linear_elastic_material_t
    use fem_sections,only:section_t,SECTION_TRUSS
    use fem_modal_analysis,only:modal_analysis_options_t,modal_result_t,solve_modal_analysis
    use fem_eigen_solver,only:eigen_solver_options_t,solve_generalized_eigen
    use fem_status,only:status_t
    use test_support,only:assert_true
    implicit none
    real(rk)::k(2,2),m(2,2);real(rk),allocatable::w(:),phi(:,:)
    type(eigen_solver_options_t)::eig;type(status_t)::status
    type(model_t)::model;type(linear_elastic_material_t)::mat;type(section_t)::sec
    type(modal_analysis_options_t)::opt;type(modal_result_t)::res
    integer(index_kind)::pos;integer(id_kind)::cid,did;integer::c
    k=0.0_rk;m=0.0_rk;k(1,1)=1.0_rk;k(2,2)=2.0_rk;m(1,1)=1.0_rk;m(2,2)=-1.0_rk
    eig%backend=1;eig%requested_modes=1
    call solve_generalized_eigen(k,m,eig,w,phi,status)
    call assert_true(.not.status%is_ok(),"non-SPD mass rejected")
    eig%backend=999
    call solve_generalized_eigen(k,abs(m),eig,w,phi,status)
    call assert_true(.not.status%is_ok(),"unknown eigen backend rejected")

    call model%mesh%add_node(1_id_kind,[0.0_rk,0.0_rk,0.0_rk],status)
    call model%mesh%add_node(2_id_kind,[1.0_rk,0.0_rk,0.0_rk],status)
    call model%mesh%add_element(3_id_kind,TOPOLOGY_BAR2,[1_id_kind,2_id_kind],status)
    call model%mesh%assign_element_formulation(3_id_kind,ELEMENT_TRUSS2,status)
    mat=linear_elastic_material_t(id=4_id_kind,name="NoDensity",young_modulus=1.0e6_rk,poisson_ratio=0.3_rk,density=0.0_rk)
    call model%materials%add(mat,status);sec=section_t(id=5_id_kind,name="A",kind=SECTION_TRUSS,area=1.0_rk);call model%sections%add(sec,status)
    call model%mesh%assign_element_properties(3_id_kind,4_id_kind,5_id_kind,status)
    call model%initialize_standard_registries(status);call model%build_nodal_field_dofs(FIELD_ID_DISPLACEMENT,status)
    do c=1,3
        pos=model%dofs%find_by_address(1_id_kind,FIELD_ID_DISPLACEMENT,c);did=model%dofs%dofs(pos)%id
        call model%constraints%add(did,merge(0.1_rk,0.0_rk,c==1),cid,status)
    end do
    do c=2,3
        pos=model%dofs%find_by_address(2_id_kind,FIELD_ID_DISPLACEMENT,c);did=model%dofs%dofs(pos)%id
        call model%constraints%add(did,0.0_rk,cid,status)
    end do
    opt%eigen%requested_modes=1
    call solve_modal_analysis(model,opt,res,status)
    call assert_true(.not.status%is_ok(),"nonzero modal prescribed displacement rejected")
end program
