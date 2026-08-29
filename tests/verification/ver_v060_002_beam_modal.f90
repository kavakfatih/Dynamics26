program ver_v060_002_beam_modal
    use fem_kinds,only:rk,id_kind,index_kind
    use fem_constants,only:FEM_PI
    use fem_model,only:model_t
    use fem_fields,only:FIELD_ID_DISPLACEMENT,FIELD_ID_ROTATION
    use fem_topology,only:TOPOLOGY_BAR2
    use fem_element_registry,only:ELEMENT_BEAM2_LINEAR_2D
    use fem_linear_elastic_material,only:linear_elastic_material_t
    use fem_sections,only:section_t,SECTION_BEAM
    use fem_modal_analysis,only:modal_analysis_options_t,modal_result_t,solve_modal_analysis
    use fem_eigen_solver,only:EIGEN_SOLVER_DENSE_REFERENCE
    use fem_structural_mass,only:MASS_CONSISTENT
    use fem_status,only:status_t
    use test_support,only:assert_true,assert_close
    implicit none
    integer,parameter::ne=8
    type(model_t)::model;type(linear_elastic_material_t)::mat;type(section_t)::sec
    type(modal_analysis_options_t)::opt;type(modal_result_t)::res;type(status_t)::status
    integer::i,c;integer(index_kind)::pos;integer(id_kind)::cid,did
    real(rk)::e,rho,a,iz,l,le,beta1,f_exact
    e=210.0e9_rk;rho=7800.0_rk;a=0.01_rk;iz=8.333333333333333e-6_rk;l=2.0_rk;le=l/real(ne,rk)
    do i=0,ne
        call model%mesh%add_node(int(100+i,id_kind),[real(i,rk)*le,0.0_rk,0.0_rk],status)
    end do
    do i=1,ne
        call model%mesh%add_element(int(200+i,id_kind),TOPOLOGY_BAR2,[int(99+i,id_kind),int(100+i,id_kind)],status)
        call model%mesh%assign_element_formulation(int(200+i,id_kind),ELEMENT_BEAM2_LINEAR_2D,status)
    end do
    mat=linear_elastic_material_t(id=5_id_kind,name="Steel",young_modulus=e,poisson_ratio=0.3_rk,density=rho)
    call model%materials%add(mat,status)
    sec=section_t(id=9_id_kind,name="Beam",kind=SECTION_BEAM,area=a,iz=iz)
    call model%sections%add(sec,status)
    do i=1,ne
        call model%mesh%assign_element_properties(int(200+i,id_kind),5_id_kind,9_id_kind,status)
    end do
    call model%initialize_standard_registries(status)
    call model%build_nodal_field_dofs(FIELD_ID_DISPLACEMENT,status)
    call model%build_nodal_field_dofs(FIELD_ID_ROTATION,status)
    ! Every node: unused uz, rx, ry are constrained. Root additionally u,v,rz fixed.
    do i=0,ne
        pos=model%dofs%find_by_address(int(100+i,id_kind),FIELD_ID_DISPLACEMENT,3);did=model%dofs%dofs(pos)%id
        call model%constraints%add(did,0.0_rk,cid,status)
        do c=1,2
            pos=model%dofs%find_by_address(int(100+i,id_kind),FIELD_ID_ROTATION,c);did=model%dofs%dofs(pos)%id
            call model%constraints%add(did,0.0_rk,cid,status)
        end do
    end do
    do c=1,2
        pos=model%dofs%find_by_address(100_id_kind,FIELD_ID_DISPLACEMENT,c);did=model%dofs%dofs(pos)%id
        call model%constraints%add(did,0.0_rk,cid,status)
    end do
    pos=model%dofs%find_by_address(100_id_kind,FIELD_ID_ROTATION,3);did=model%dofs%dofs(pos)%id
    call model%constraints%add(did,0.0_rk,cid,status)
    opt%eigen%backend=EIGEN_SOLVER_DENSE_REFERENCE;opt%eigen%requested_modes=3;opt%mass_kind=MASS_CONSISTENT
    call solve_modal_analysis(model,opt,res,status)
    call assert_true(status%is_ok(),"8-element cantilever beam modal solve")
    beta1=1.875104068711961_rk
    f_exact=beta1**2/(2.0_rk*FEM_PI*l*l)*sqrt(e*iz/(rho*a))
    call assert_close(res%frequencies_hz(1),f_exact,1e-6_rk,3e-4_rk,"beam first bending frequency")
    call assert_true(maxval(res%residual_norms)<1e-8_rk,"beam modal residual")
end program
