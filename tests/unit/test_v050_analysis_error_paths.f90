program test_v050_analysis_error_paths
    use fem_kinds, only : rk,id_kind,index_kind
    use fem_model, only : model_t
    use fem_fields, only : FIELD_ID_DISPLACEMENT
    use fem_topology, only : TOPOLOGY_BAR2
    use fem_element_registry, only : ELEMENT_TRUSS2,ELEMENT_PLANE_STRESS_QUAD4
    use fem_linear_elastic_material, only : linear_elastic_material_t
    use fem_sections, only : section_t,SECTION_PLANE
    use fem_linear_static_analysis, only : linear_static_result_t,solve_linear_static
    use fem_linear_solver, only : linear_solver_options_t,LINEAR_SOLVER_DENSE_REFERENCE
    use fem_status, only : status_t
    use test_support, only : assert_true
    implicit none

    call test_topology_formulation_mismatch()
    call test_section_kind_mismatch()
    write(*,'(A)') 'PASS V0.5 linear-analysis error paths'
contains
    subroutine build_bar(model, formulation_id, status)
        type(model_t), intent(inout) :: model
        integer(id_kind), intent(in) :: formulation_id
        type(status_t), intent(out) :: status
        integer :: c
        integer(index_kind) :: pos
        integer(id_kind) :: did,cid
        call model%mesh%add_node(11_id_kind,[0.0_rk,0.0_rk,0.0_rk],status); if(.not.status%is_ok()) return
        call model%mesh%add_node(29_id_kind,[1.0_rk,0.0_rk,0.0_rk],status); if(.not.status%is_ok()) return
        call model%mesh%add_element(7_id_kind,TOPOLOGY_BAR2,[11_id_kind,29_id_kind],status); if(.not.status%is_ok()) return
        call model%mesh%assign_element_formulation(7_id_kind,formulation_id,status); if(.not.status%is_ok()) return
        call model%initialize_standard_registries(status); if(.not.status%is_ok()) return
        call model%build_nodal_field_dofs(FIELD_ID_DISPLACEMENT,status); if(.not.status%is_ok()) return
        do c=1,3
            pos=model%dofs%find_by_address(11_id_kind,FIELD_ID_DISPLACEMENT,c); did=model%dofs%dofs(pos)%id
            call model%constraints%add(did,0.0_rk,cid,status); if(.not.status%is_ok()) return
        end do
        do c=2,3
            pos=model%dofs%find_by_address(29_id_kind,FIELD_ID_DISPLACEMENT,c); did=model%dofs%dofs(pos)%id
            call model%constraints%add(did,0.0_rk,cid,status); if(.not.status%is_ok()) return
        end do
    end subroutine build_bar

    subroutine test_topology_formulation_mismatch()
        type(model_t) :: model
        type(linear_static_result_t) :: result
        type(linear_solver_options_t) :: options
        type(status_t) :: status
        call build_bar(model,ELEMENT_PLANE_STRESS_QUAD4,status); call assert_true(status%is_ok(),'mismatch model setup')
        options%backend=LINEAR_SOLVER_DENSE_REFERENCE
        call solve_linear_static(model,options,result,status)
        call assert_true(.not.status%is_ok(),'topology/formulation mismatch must fail')
    end subroutine test_topology_formulation_mismatch

    subroutine test_section_kind_mismatch()
        type(model_t) :: model
        type(linear_elastic_material_t) :: mat
        type(section_t) :: sec
        type(linear_static_result_t) :: result
        type(linear_solver_options_t) :: options
        type(status_t) :: status
        integer(index_kind) :: pos
        integer(id_kind) :: did,lid
        call build_bar(model,ELEMENT_TRUSS2,status); call assert_true(status%is_ok(),'section mismatch model setup')
        mat=linear_elastic_material_t(id=3_id_kind,name='steel',young_modulus=200.0e9_rk,poisson_ratio=0.3_rk)
        call model%materials%add(mat,status); call assert_true(status%is_ok(),'material setup')
        sec=section_t(id=4_id_kind,name='wrong-plane',kind=SECTION_PLANE,thickness=0.01_rk)
        call model%sections%add(sec,status); call assert_true(status%is_ok(),'section setup')
        call model%mesh%assign_element_properties(7_id_kind,3_id_kind,4_id_kind,status); call assert_true(status%is_ok(),'property setup')
        pos=model%dofs%find_by_address(29_id_kind,FIELD_ID_DISPLACEMENT,1); did=model%dofs%dofs(pos)%id
        call model%loads%add(did,100.0_rk,lid,status); call assert_true(status%is_ok(),'load setup')
        options%backend=LINEAR_SOLVER_DENSE_REFERENCE
        call solve_linear_static(model,options,result,status)
        call assert_true(.not.status%is_ok(),'TRUSS2 with plane section must fail')
    end subroutine test_section_kind_mismatch
end program test_v050_analysis_error_paths
