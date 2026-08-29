program ver_v060_001_axial_modal
    use fem_kinds,only:rk,id_kind,index_kind
    use fem_model,only:model_t
    use fem_fields,only:FIELD_ID_DISPLACEMENT
    use fem_topology,only:TOPOLOGY_BAR2
    use fem_element_registry,only:ELEMENT_TRUSS2
    use fem_linear_elastic_material,only:linear_elastic_material_t
    use fem_sections,only:section_t,SECTION_TRUSS
    use fem_modal_analysis,only:modal_analysis_options_t,modal_result_t,solve_modal_analysis
    use fem_eigen_solver,only:EIGEN_SOLVER_DENSE_REFERENCE
    use fem_structural_mass,only:MASS_CONSISTENT
    use fem_status,only:status_t
    use test_support,only:assert_true,assert_close
    implicit none
    type(model_t)::model;type(linear_elastic_material_t)::mat;type(section_t)::sec
    type(modal_analysis_options_t)::opt;type(modal_result_t)::res;type(status_t)::status
    integer(id_kind)::cid,did;integer(index_kind)::pos;integer::node,comp
    real(rk)::e,rho,a,lambda1,lambda2
    e=210.0e9_rk;rho=7800.0_rk;a=1.0e-4_rk
    call model%mesh%add_node(100_id_kind,[0.0_rk,0.0_rk,0.0_rk],status)
    call model%mesh%add_node(7_id_kind,[1.0_rk,0.0_rk,0.0_rk],status)
    call model%mesh%add_node(900_id_kind,[2.0_rk,0.0_rk,0.0_rk],status)
    call model%mesh%add_element(50_id_kind,TOPOLOGY_BAR2,[100_id_kind,7_id_kind],status)
    call model%mesh%add_element(10_id_kind,TOPOLOGY_BAR2,[7_id_kind,900_id_kind],status)
    call model%mesh%assign_element_formulation(50_id_kind,ELEMENT_TRUSS2,status);call model%mesh%assign_element_formulation(10_id_kind,ELEMENT_TRUSS2,status)
    mat=linear_elastic_material_t(id=5_id_kind,name="Steel",young_modulus=e,poisson_ratio=0.3_rk,density=rho);call model%materials%add(mat,status)
    sec=section_t(id=9_id_kind,name="Bar",kind=SECTION_TRUSS,area=a);call model%sections%add(sec,status)
    call model%mesh%assign_element_properties(50_id_kind,5_id_kind,9_id_kind,status);call model%mesh%assign_element_properties(10_id_kind,5_id_kind,9_id_kind,status)
    call model%initialize_standard_registries(status);call model%build_nodal_field_dofs(FIELD_ID_DISPLACEMENT,status)
    do node=1,3
        do comp=1,3
            if(node==1 .or. comp>1)then
                pos=model%dofs%find_by_address(model%mesh%nodes(node)%id,FIELD_ID_DISPLACEMENT,comp);did=model%dofs%dofs(pos)%id
                call model%constraints%add(did,0.0_rk,cid,status)
            end if
        end do
    end do
    opt%eigen%backend=EIGEN_SOLVER_DENSE_REFERENCE;opt%eigen%requested_modes=2;opt%mass_kind=MASS_CONSISTENT
    call solve_modal_analysis(model,opt,res,status);call assert_true(status%is_ok(),"Two-element axial modal solve")
    lambda1=(e/rho)*(30.0_rk-18.0_rk*sqrt(2.0_rk))/7.0_rk
    lambda2=(e/rho)*(30.0_rk+18.0_rk*sqrt(2.0_rk))/7.0_rk
    call assert_close(res%eigenvalues(1),lambda1,1e-4_rk,1e-9_rk,"axial lambda1")
    call assert_close(res%eigenvalues(2),lambda2,1e-4_rk,1e-9_rk,"axial lambda2")
    call assert_true(maxval(res%residual_norms)<1e-10_rk,"modal residual")
end program
