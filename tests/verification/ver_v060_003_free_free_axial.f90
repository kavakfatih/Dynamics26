program ver_v060_003_free_free_axial
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
    type(model_t)::model
    type(linear_elastic_material_t)::mat
    type(section_t)::sec
    type(modal_analysis_options_t)::opt
    type(modal_result_t)::res
    type(status_t)::status
    integer(index_kind)::pos
    integer(id_kind)::did,cid
    integer::node,comp
    real(rk)::e,rho,a,l,expected_lambda

    e=70.0e9_rk;rho=2700.0_rk;a=2.0e-4_rk;l=1.5_rk
    call model%mesh%add_node(41_id_kind,[0.0_rk,0.0_rk,0.0_rk],status)
    call model%mesh%add_node(9_id_kind,[l,0.0_rk,0.0_rk],status)
    call model%mesh%add_element(77_id_kind,TOPOLOGY_BAR2,[41_id_kind,9_id_kind],status)
    call model%mesh%assign_element_formulation(77_id_kind,ELEMENT_TRUSS2,status)
    mat=linear_elastic_material_t(id=3_id_kind,name="Al",young_modulus=e,poisson_ratio=0.33_rk,density=rho)
    sec=section_t(id=8_id_kind,name="Bar",kind=SECTION_TRUSS,area=a)
    call model%materials%add(mat,status);call model%sections%add(sec,status)
    call model%mesh%assign_element_properties(77_id_kind,3_id_kind,8_id_kind,status)
    call model%initialize_standard_registries(status)
    call model%build_nodal_field_dofs(FIELD_ID_DISPLACEMENT,status)

    ! y/z hareketleri formulation mekanizmasi olmasin; x ekseni iki ucta serbesttir.
    do node=1,2
        do comp=2,3
            pos=model%dofs%find_by_address(model%mesh%nodes(node)%id,FIELD_ID_DISPLACEMENT,comp)
            did=model%dofs%dofs(pos)%id
            call model%constraints%add(did,0.0_rk,cid,status)
        end do
    end do

    opt%eigen%backend=EIGEN_SOLVER_DENSE_REFERENCE
    opt%eigen%requested_modes=1
    opt%mass_kind=MASS_CONSISTENT
    call solve_modal_analysis(model,opt,res,status)
    call assert_true(status%is_ok(),"Free-free axial modal solve")
    call assert_true(res%zero_mode_count==1,"One axial rigid translation detected")
    expected_lambda=12.0_rk*e/(rho*l*l)
    call assert_close(res%eigenvalues(1),expected_lambda,1e-4_rk,1e-10_rk,"free-free flexible eigenvalue")
    call assert_true(res%residual_norms(1)<1e-10_rk,"free-free modal residual")
end program
