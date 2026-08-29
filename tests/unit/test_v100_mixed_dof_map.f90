program test_v100_mixed_dof_map
    use fem_kinds, only : rk,id_kind,index_kind
    use fem_model, only : model_t
    use fem_fields, only : FIELD_ID_DISPLACEMENT,FIELD_ID_PRESSURE_P0
    use fem_topology, only : TOPOLOGY_HEX8
    use fem_element_dof_map, only : element_dof_map_t
    use fem_status, only : status_t
    use test_support, only : assert_true,assert_equal_index,assert_equal_id
    implicit none
    type(model_t) :: model
    type(element_dof_map_t) :: map
    type(status_t) :: status
    integer(id_kind), parameter :: nodes(8)=[10_id_kind,42_id_kind,7_id_kind,90_id_kind,3_id_kind,65_id_kind,18_id_kind,111_id_kind]
    real(rk) :: x(3,8)
    integer :: a
    integer(index_kind) :: pos
    integer(id_kind) :: pressure_dof,pressure_eq

    call unit_cube(x)
    do a=1,8
        call model%mesh%add_node(nodes(a),x(:,a),status);call assert_true(status%is_ok(),'mixed map node')
    end do
    call model%mesh%add_element(700_id_kind,TOPOLOGY_HEX8,nodes,status);call assert_true(status%is_ok(),'mixed map element')
    call model%initialize_standard_registries(status);call assert_true(status%is_ok(),'mixed map registry')
    call model%build_nodal_field_dofs(FIELD_ID_DISPLACEMENT,status);call assert_true(status%is_ok(),'mixed map u dofs')
    call model%build_element_field_dofs(FIELD_ID_PRESSURE_P0,status);call assert_true(status%is_ok(),'mixed map p dof')
    call model%renumber(status);call assert_true(status%is_ok(),'mixed map numbering')

    call map%build_mixed_up_p0(model%mesh,700_id_kind,FIELD_ID_DISPLACEMENT,FIELD_ID_PRESSURE_P0, &
        model%dofs,model%constraints,model%numbering,status)
    call assert_true(status%is_ok(),'mixed map build')
    call assert_equal_index(map%size(),25_index_kind,'Q1/P0 map has 25 local unknowns')
    pos=model%dofs%find_by_address(700_id_kind,FIELD_ID_PRESSURE_P0,1)
    call assert_true(pos/=0_index_kind,'pressure DOF addressed by element ID')
    pressure_dof=model%dofs%dofs(pos)%id
    pressure_eq=model%numbering%equation_of(pressure_dof)
    call assert_equal_id(map%dof_ids(25),pressure_dof,'pressure is local DOF 25')
    call assert_equal_id(map%equation_ids(25),pressure_eq,'pressure equation mapping')
    call assert_true(model%dofs%find_by_address(nodes(1),FIELD_ID_PRESSURE_P0,1)==0_index_kind, &
        'P0 pressure is not nodal')
    write(*,'(A)')'PASS unit_v100_mixed_dof_map'
contains
    subroutine unit_cube(coords)
        real(rk),intent(out)::coords(3,8)
        coords(:,1)=[0._rk,0._rk,0._rk];coords(:,2)=[1._rk,0._rk,0._rk]
        coords(:,3)=[1._rk,1._rk,0._rk];coords(:,4)=[0._rk,1._rk,0._rk]
        coords(:,5)=[0._rk,0._rk,1._rk];coords(:,6)=[1._rk,0._rk,1._rk]
        coords(:,7)=[1._rk,1._rk,1._rk];coords(:,8)=[0._rk,1._rk,1._rk]
    end subroutine unit_cube
end program test_v100_mixed_dof_map
