program test_v100_pressure_results
    use fem_kinds,only:rk,id_kind,index_kind
    use fem_model,only:model_t
    use fem_fields,only:FIELD_ID_DISPLACEMENT,FIELD_ID_PRESSURE_P0
    use fem_topology,only:TOPOLOGY_HEX8
    use fem_element_registry,only:ELEMENT_MIXED_UP_HEX8_P0
    use fem_mixed_results,only:element_pressure_results_t,recover_mixed_p0_pressure
    use fem_status,only:status_t
    use test_support,only:assert_true,assert_close,assert_equal_int
    implicit none
    type(model_t)::model
    type(element_pressure_results_t)::result
    type(status_t)::status
    real(rk),allocatable::q(:)
    real(rk)::x(3,8)
    integer(id_kind),parameter::nodes(8)=[2_id_kind,4_id_kind,6_id_kind,8_id_kind,10_id_kind,12_id_kind,14_id_kind,16_id_kind]
    integer(index_kind)::pos
    integer(id_kind)::did,eq
    integer::a
    call cube(x)
    do a=1,8;call model%mesh%add_node(nodes(a),x(:,a),status);call assert_true(status%is_ok(),'pressure result node');end do
    call model%mesh%add_element(333_id_kind,TOPOLOGY_HEX8,nodes,status);call assert_true(status%is_ok(),'pressure result element')
    call model%mesh%assign_element_formulation(333_id_kind,ELEMENT_MIXED_UP_HEX8_P0,status);call assert_true(status%is_ok(),'pressure result formulation')
    call model%initialize_standard_registries(status);call assert_true(status%is_ok(),'pressure result registry')
    call model%build_nodal_field_dofs(FIELD_ID_DISPLACEMENT,status);call assert_true(status%is_ok(),'pressure result u dofs')
    call model%build_element_field_dofs(FIELD_ID_PRESSURE_P0,status);call assert_true(status%is_ok(),'pressure result p dof')
    call model%renumber(status);call assert_true(status%is_ok(),'pressure result numbering')
    allocate(q(int(model%numbering%active_equation_count)));q=0._rk
    pos=model%dofs%find_by_address(333_id_kind,FIELD_ID_PRESSURE_P0,1);did=model%dofs%dofs(pos)%id;eq=model%numbering%equation_of(did)
    q(int(eq)+1)=2.75e6_rk
    call recover_mixed_p0_pressure(model,q,result,status);call assert_true(status%is_ok(),'pressure recovery')
    call assert_equal_int(size(result%pressure),1,'one P0 pressure result')
    call assert_close(result%pressure(1),2.75e6_rk,1.e-9_rk,1.e-14_rk,'P0 pressure recovered')
    call assert_true(result%element_ids(1)==333_id_kind,'pressure result keeps element ID')
    write(*,'(A)')'PASS unit_v100_pressure_results'
contains
    subroutine cube(c)
        real(rk),intent(out)::c(3,8)
        c(:,1)=[0._rk,0._rk,0._rk];c(:,2)=[1._rk,0._rk,0._rk];c(:,3)=[1._rk,1._rk,0._rk];c(:,4)=[0._rk,1._rk,0._rk]
        c(:,5)=[0._rk,0._rk,1._rk];c(:,6)=[1._rk,0._rk,1._rk];c(:,7)=[1._rk,1._rk,1._rk];c(:,8)=[0._rk,1._rk,1._rk]
    end subroutine cube
end program test_v100_pressure_results
