program ver_v050_003_linear_driver
    use fem_kinds, only : rk,id_kind,index_kind
    use fem_model, only : model_t
    use fem_fields, only : FIELD_ID_DISPLACEMENT
    use fem_topology, only : TOPOLOGY_BAR2
    use fem_element_registry, only : ELEMENT_TRUSS2
    use fem_linear_elastic_material, only : linear_elastic_material_t
    use fem_sections, only : section_t,SECTION_TRUSS
    use fem_linear_static_analysis, only : linear_static_result_t,solve_linear_static
    use fem_linear_solver, only : linear_solver_options_t,LINEAR_SOLVER_DENSE_REFERENCE
    use fem_status, only : status_t
    use test_support, only : assert_true,assert_close
    implicit none
    type(model_t) :: model
    type(linear_elastic_material_t) :: mat
    type(section_t) :: sec
    type(linear_static_result_t) :: result
    type(linear_solver_options_t) :: options
    type(status_t) :: status
    integer(id_kind) :: cid,lid,did
    integer(index_kind) :: pos
    real(rk) :: e,a,l,f,expected
    integer :: c
    e=200.0e9_rk; a=2.5e-4_rk; l=1.5_rk; f=2500.0_rk; expected=f*l/(e*a)
    call model%mesh%add_node(80_id_kind,[0.0_rk,0.0_rk,0.0_rk],status); call assert_true(status%is_ok(),"n1")
    call model%mesh%add_node(3_id_kind,[l,0.0_rk,0.0_rk],status); call assert_true(status%is_ok(),"n2")
    call model%mesh%add_element(44_id_kind,TOPOLOGY_BAR2,[80_id_kind,3_id_kind],status); call assert_true(status%is_ok(),"e1")
    call model%mesh%assign_element_formulation(44_id_kind,ELEMENT_TRUSS2,status); call assert_true(status%is_ok(),"formulation")
    mat=linear_elastic_material_t(id=5_id_kind,name="steel",young_modulus=e,poisson_ratio=0.3_rk)
    call model%materials%add(mat,status); call assert_true(status%is_ok(),"material")
    sec=section_t(id=9_id_kind,name="rod",kind=SECTION_TRUSS,area=a)
    call model%sections%add(sec,status); call assert_true(status%is_ok(),"section")
    call model%mesh%assign_element_properties(44_id_kind,5_id_kind,9_id_kind,status); call assert_true(status%is_ok(),"properties")
    call model%initialize_standard_registries(status); call assert_true(status%is_ok(),"registries")
    call model%build_nodal_field_dofs(FIELD_ID_DISPLACEMENT,status); call assert_true(status%is_ok(),"dofs")
    do c=1,3
        pos=model%dofs%find_by_address(80_id_kind,FIELD_ID_DISPLACEMENT,c); did=model%dofs%dofs(pos)%id
        call model%constraints%add(did,0.0_rk,cid,status); call assert_true(status%is_ok(),"root support")
    end do
    do c=2,3
        pos=model%dofs%find_by_address(3_id_kind,FIELD_ID_DISPLACEMENT,c); did=model%dofs%dofs(pos)%id
        call model%constraints%add(did,0.0_rk,cid,status); call assert_true(status%is_ok(),"tip lateral")
    end do
    pos=model%dofs%find_by_address(3_id_kind,FIELD_ID_DISPLACEMENT,1); did=model%dofs%dofs(pos)%id
    call model%loads%add(did,f,lid,status); call assert_true(status%is_ok(),"load")
    options%backend=LINEAR_SOLVER_DENSE_REFERENCE
    call solve_linear_static(model,options,result,status); call assert_true(status%is_ok(),"linear driver")
    call assert_true(result%solver_statistics%converged,"linear driver converged")
    call assert_close(result%value_of_dof(did),expected,1.0e-12_rk,1.0e-10_rk,"driver tip displacement")
    pos=model%dofs%find_by_address(80_id_kind,FIELD_ID_DISPLACEMENT,1); did=model%dofs%dofs(pos)%id
    call assert_close(result%reactions%value_of(did),-f,1.0e-8_rk,1.0e-10_rk,"driver support reaction")
    write(*,'(A)') "PASS VER-V050-003 generic linear static driver"
end program ver_v050_003_linear_driver
