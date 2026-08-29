program ver_v070_003_global_nonlinear_assembly
    use fem_kinds,only:rk,id_kind,index_kind
    use fem_model,only:model_t
    use fem_fields,only:FIELD_ID_DISPLACEMENT
    use fem_topology,only:TOPOLOGY_HEX8
    use fem_element_registry,only:ELEMENT_TOTAL_LAGRANGIAN_HEX8
    use fem_linear_elastic_material,only:linear_elastic_material_t
    use fem_nonlinear_assembly,only:nonlinear_system_t,evaluate_nonlinear_system
    use fem_status,only:status_t
    use test_support,only:assert_true
    implicit none
    type(model_t)::model
    type(linear_elastic_material_t)::mat
    type(nonlinear_system_t)::system,plus_system,minus_system
    type(status_t)::status
    real(rk)::x(3,8),hmat(3,3),active(24),plus(24),minus(24),fd(24,24),h,err,scale
    real(rk), allocatable :: dense(:,:)
    integer(id_kind)::nodes(8)=[81_id_kind,7_id_kind,42_id_kind,5_id_kind,900_id_kind,11_id_kind,3_id_kind,77_id_kind]
    integer(index_kind)::pos
    integer(id_kind)::dof_id,eq
    integer::a,c,j

    call unit_cube(x)
    do a=1,8
        call model%mesh%add_node(nodes(a),x(:,a),status);call assert_true(status%is_ok(),"add node")
    end do
    call model%mesh%add_element(600_id_kind,TOPOLOGY_HEX8,nodes,status);call assert_true(status%is_ok(),"add hex")
    call model%mesh%assign_element_formulation(600_id_kind,ELEMENT_TOTAL_LAGRANGIAN_HEX8,status)
    call assert_true(status%is_ok(),"assign nonlinear formulation")
    mat=linear_elastic_material_t(id=9_id_kind,name="global nonlinear",young_modulus=3.e6_rk,poisson_ratio=0.28_rk)
    call model%materials%add(mat,status);call assert_true(status%is_ok(),"add material")
    call model%mesh%assign_element_properties(600_id_kind,9_id_kind,-1_id_kind,status)
    call assert_true(status%is_ok(),"assign properties")
    call model%initialize_standard_registries(status);call assert_true(status%is_ok(),"registries")
    call model%build_nodal_field_dofs(FIELD_ID_DISPLACEMENT,status);call assert_true(status%is_ok(),"dofs")
    call model%renumber(status);call assert_true(status%is_ok(),"renumber")

    hmat=reshape([0.07_rk,-0.012_rk,0.004_rk,0.02_rk,0.035_rk,0.009_rk,0.006_rk,-0.004_rk,-0.015_rk],[3,3])
    active=0.0_rk
    do a=1,8
        do c=1,3
            pos=model%dofs%find_by_address(nodes(a),FIELD_ID_DISPLACEMENT,c)
            dof_id=model%dofs%dofs(pos)%id;eq=model%numbering%equation_of(dof_id)
            active(int(eq)+1)=dot_product(hmat(c,:),x(:,a))
        end do
    end do
    call evaluate_nonlinear_system(model,active,system,status);call assert_true(status%is_ok(),"global nonlinear assembly")
    call system%tangent%to_dense(dense,status);call assert_true(status%is_ok(),"dense tangent view")
    h=1.e-7_rk
    do j=1,24
        plus=active;minus=active;plus(j)=plus(j)+h;minus(j)=minus(j)-h
        call evaluate_nonlinear_system(model,plus,plus_system,status);call assert_true(status%is_ok(),"global plus")
        call evaluate_nonlinear_system(model,minus,minus_system,status);call assert_true(status%is_ok(),"global minus")
        fd(:,j)=(plus_system%internal_force-minus_system%internal_force)/(2._rk*h)
    end do
    scale=max(1._rk,sqrt(sum(fd*fd)));err=sqrt(sum((dense-fd)**2))/scale
    call assert_true(err<2.e-7_rk,"global assembled tangent finite difference")
    call assert_true(maxval(abs(system%residual+system%internal_force))<1.e-12_rk,"zero-load residual convention")
contains
    subroutine unit_cube(coords)
        real(rk),intent(out)::coords(3,8)
        coords(:,1)=[0._rk,0._rk,0._rk];coords(:,2)=[1._rk,0._rk,0._rk]
        coords(:,3)=[1._rk,1._rk,0._rk];coords(:,4)=[0._rk,1._rk,0._rk]
        coords(:,5)=[0._rk,0._rk,1._rk];coords(:,6)=[1._rk,0._rk,1._rk]
        coords(:,7)=[1._rk,1._rk,1._rk];coords(:,8)=[0._rk,1._rk,1._rk]
    end subroutine unit_cube
end program ver_v070_003_global_nonlinear_assembly
