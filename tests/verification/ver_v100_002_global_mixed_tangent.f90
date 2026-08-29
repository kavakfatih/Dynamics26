program ver_v100_002_global_mixed_tangent
    use fem_kinds, only : rk,id_kind,index_kind
    use fem_model, only : model_t
    use fem_fields, only : FIELD_ID_DISPLACEMENT,FIELD_ID_PRESSURE_P0
    use fem_topology, only : TOPOLOGY_HEX8
    use fem_element_registry, only : ELEMENT_MIXED_UP_HEX8_P0
    use fem_hyperelastic_material, only : hyperelastic_material_t,HYPER_MOONEY_RIVLIN
    use fem_nonlinear_assembly, only : nonlinear_system_t,evaluate_nonlinear_system
    use fem_status, only : status_t
    use test_support, only : assert_true,assert_equal_int
    implicit none
    type(model_t) :: model
    type(status_t) :: status
    type(hyperelastic_material_t) :: mat
    type(nonlinear_system_t) :: base,plus_sys,minus_sys
    integer(id_kind),parameter :: nodes(8)=[101_id_kind,7_id_kind,88_id_kind,13_id_kind,205_id_kind,3_id_kind,55_id_kind,144_id_kind]
    real(rk) :: x(3,8),h,err,scale
    real(rk), allocatable :: q(:),qp(:),qm(:),fd(:,:),dense(:,:)
    real(rk)::amat(3,3)
    integer::a,c,j
    integer(index_kind)::pos
    integer(id_kind)::dof_id,eq

    call unit_cube(x)
    do a=1,8;call model%mesh%add_node(nodes(a),x(:,a),status);call assert_true(status%is_ok(),'add mixed node');end do
    call model%mesh%add_element(700_id_kind,TOPOLOGY_HEX8,nodes,status);call assert_true(status%is_ok(),'add mixed element')
    call model%mesh%assign_element_formulation(700_id_kind,ELEMENT_MIXED_UP_HEX8_P0,status);call assert_true(status%is_ok(),'assign mixed formulation')
    mat=hyperelastic_material_t(id=77_id_kind,name='Mixed MR global',model=HYPER_MOONEY_RIVLIN,bulk_modulus=1.5e9_rk,c10=0.75e6_rk,c01=0.22e6_rk)
    call model%hyperelastic_materials%add(mat,status);call assert_true(status%is_ok(),'add mixed hyper material')
    call model%mesh%assign_element_properties(700_id_kind,77_id_kind,-1_id_kind,status);call assert_true(status%is_ok(),'mixed properties')
    call model%initialize_standard_registries(status);call assert_true(status%is_ok(),'mixed registries')
    call model%build_nodal_field_dofs(FIELD_ID_DISPLACEMENT,status);call assert_true(status%is_ok(),'mixed u dofs')
    call model%build_element_field_dofs(FIELD_ID_PRESSURE_P0,status);call assert_true(status%is_ok(),'mixed p0 dof')
    call model%renumber(status);call assert_true(status%is_ok(),'mixed numbering')
    call assert_equal_int(int(model%numbering%active_equation_count),25,'mixed active equation count')

    allocate(q(25),qp(25),qm(25),fd(25,25));q=0.0_rk
    amat=reshape([0.025_rk,-0.007_rk,0.004_rk,0.011_rk,0.016_rk,0.005_rk,0.002_rk,-0.003_rk,-0.010_rk],[3,3])
    do a=1,8
        do c=1,3
            pos=model%dofs%find_by_address(nodes(a),FIELD_ID_DISPLACEMENT,c);dof_id=model%dofs%dofs(pos)%id
            eq=model%numbering%equation_of(dof_id);q(int(eq)+1)=dot_product(amat(c,:),x(:,a))
        end do
    end do
    pos=model%dofs%find_by_address(700_id_kind,FIELD_ID_PRESSURE_P0,1);dof_id=model%dofs%dofs(pos)%id;eq=model%numbering%equation_of(dof_id)
    q(int(eq)+1)=0.18e6_rk
    call evaluate_nonlinear_system(model,q,base,status);call assert_true(status%is_ok(),'global mixed base')
    call assert_true(base%has_mixed_pressure,'global system detects mixed pressure')
    call assert_equal_int(base%pressure_unknown_count,1,'one global pressure unknown')
    call assert_equal_int(base%displacement_unknown_count,24,'24 global displacement unknowns')
    call base%tangent%to_dense(dense,status);call assert_true(status%is_ok(),'mixed tangent to dense')

    do j=1,25
        qp=q;qm=q
        if(j==int(eq)+1)then;h=10.0_rk;else;h=1.e-7_rk;end if
        qp(j)=qp(j)+h;qm(j)=qm(j)-h
        call evaluate_nonlinear_system(model,qp,plus_sys,status);call assert_true(status%is_ok(),'global mixed plus')
        call evaluate_nonlinear_system(model,qm,minus_sys,status);call assert_true(status%is_ok(),'global mixed minus')
        fd(:,j)=(plus_sys%internal_force-minus_sys%internal_force)/(2.0_rk*h)
    end do
    scale=max(1.0_rk,sqrt(sum(fd*fd)));err=sqrt(sum((dense-fd)**2))/scale
    if(err>8.e-6_rk)write(*,'(A,ES12.4)')'global mixed tangent rel err=',err
    call assert_true(err<8.e-6_rk,'global mixed sparse tangent FD Jacobian')
    call assert_true(maxval(abs(dense-transpose(dense)))<1.e-10_rk*max(1._rk,maxval(abs(dense))),'global mixed tangent symmetric')
    write(*,'(A)')'PASS VER-V100-002 global mixed sparse tangent'
contains
    subroutine unit_cube(coords)
        real(rk),intent(out)::coords(3,8)
        coords(:,1)=[0._rk,0._rk,0._rk];coords(:,2)=[1._rk,0._rk,0._rk]
        coords(:,3)=[1._rk,1._rk,0._rk];coords(:,4)=[0._rk,1._rk,0._rk]
        coords(:,5)=[0._rk,0._rk,1._rk];coords(:,6)=[1._rk,0._rk,1._rk]
        coords(:,7)=[1._rk,1._rk,1._rk];coords(:,8)=[0._rk,1._rk,1._rk]
    end subroutine unit_cube
end program ver_v100_002_global_mixed_tangent
