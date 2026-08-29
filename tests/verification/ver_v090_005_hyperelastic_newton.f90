program ver_v090_005_hyperelastic_newton
    use fem_kinds, only : rk,id_kind,index_kind
    use fem_model, only : model_t
    use fem_fields, only : FIELD_ID_DISPLACEMENT
    use fem_topology, only : TOPOLOGY_HEX8
    use fem_element_registry, only : ELEMENT_TOTAL_LAGRANGIAN_HEX8
    use fem_hyperelastic_material, only : hyperelastic_material_t,HYPER_NEO_HOOKEAN,hyperelastic_response
    use fem_finite_strain_kinematics, only : second_pk_to_first_pk
    use fem_nonlinear_solver, only : nonlinear_solver_options_t,nonlinear_static_result_t,solve_nonlinear_static
    use fem_linear_solver, only : LINEAR_SOLVER_DENSE_REFERENCE
    use fem_status, only : status_t
    use test_support, only : assert_true,assert_close
    implicit none
    type(model_t)::model
    type(hyperelastic_material_t)::mat
    type(nonlinear_solver_options_t)::options
    type(nonlinear_static_result_t)::result
    type(status_t)::status
    integer(id_kind),parameter::node_ids(8)=[81_id_kind,7_id_kind,42_id_kind,5_id_kind,900_id_kind,11_id_kind,3_id_kind,77_id_kind]
    integer,parameter::right_nodes(4)=[2,3,6,7],left_nodes(4)=[1,4,5,8]
    integer(id_kind)::right_dofs(4),dof_id,constraint_id,load_id,eq
    integer(index_kind)::pos
    real(rk)::x(3,8),area,length,b,lambda_target,f(3,3),s(3,3),p(3,3),d(6,6),w,total_force,sum_u
    integer::a,c,i

    area=1.0_rk;length=1.0_rk;b=1.0_rk;lambda_target=1.08_rk
    mat=hyperelastic_material_t(id=9_id_kind,name='NH Newton',model=HYPER_NEO_HOOKEAN,bulk_modulus=12.e6_rk,c10=1.0e6_rk)
    f=0.0_rk;f(1,1)=lambda_target;f(2,2)=1._rk;f(3,3)=1._rk
    call hyperelastic_response(mat,f,s,d,w,status);call assert_true(status%is_ok(),'target material point')
    call second_pk_to_first_pk(f,s,p);total_force=p(1,1)*area

    x(:,1)=[0._rk,0._rk,0._rk];x(:,2)=[length,0._rk,0._rk];x(:,3)=[length,b,0._rk];x(:,4)=[0._rk,b,0._rk]
    x(:,5)=[0._rk,0._rk,b];x(:,6)=[length,0._rk,b];x(:,7)=[length,b,b];x(:,8)=[0._rk,b,b]
    do a=1,8;call model%mesh%add_node(node_ids(a),x(:,a),status);call assert_true(status%is_ok(),'add node');end do
    call model%mesh%add_element(600_id_kind,TOPOLOGY_HEX8,node_ids,status);call assert_true(status%is_ok(),'add hex')
    call model%mesh%assign_element_formulation(600_id_kind,ELEMENT_TOTAL_LAGRANGIAN_HEX8,status);call assert_true(status%is_ok(),'formulation')
    call model%hyperelastic_materials%add(mat,status);call assert_true(status%is_ok(),'hyper registry')
    call model%mesh%assign_element_properties(600_id_kind,9_id_kind,-1_id_kind,status);call assert_true(status%is_ok(),'properties')
    call model%initialize_standard_registries(status);call assert_true(status%is_ok(),'registries')
    call model%build_nodal_field_dofs(FIELD_ID_DISPLACEMENT,status);call assert_true(status%is_ok(),'dofs')
    do a=1,8
        do c=2,3
            pos=model%dofs%find_by_address(node_ids(a),FIELD_ID_DISPLACEMENT,c);dof_id=model%dofs%dofs(pos)%id
            call model%constraints%add(dof_id,0._rk,constraint_id,status);call assert_true(status%is_ok(),'yz bc')
        end do
    end do
    do i=1,4
        a=left_nodes(i);pos=model%dofs%find_by_address(node_ids(a),FIELD_ID_DISPLACEMENT,1);dof_id=model%dofs%dofs(pos)%id
        call model%constraints%add(dof_id,0._rk,constraint_id,status);call assert_true(status%is_ok(),'left bc')
    end do
    do i=1,4
        a=right_nodes(i);pos=model%dofs%find_by_address(node_ids(a),FIELD_ID_DISPLACEMENT,1);dof_id=model%dofs%dofs(pos)%id;right_dofs(i)=dof_id
        call model%loads%add(dof_id,total_force/4._rk,load_id,status);call assert_true(status%is_ok(),'load')
    end do
    options%initial_load_increment=0.25_rk;options%minimum_load_increment=1.e-4_rk;options%maximum_load_increment=0.5_rk
    options%max_iterations=20;options%line_search=.true.;options%adaptive_stepping=.true.;options%linear%backend=LINEAR_SOLVER_DENSE_REFERENCE
    call solve_nonlinear_static(model,options,result,status);call assert_true(status%is_ok().and.result%converged,'hyperelastic Newton solve')
    sum_u=0._rk
    do i=1,4;eq=model%numbering%equation_of(right_dofs(i));sum_u=sum_u+result%active_displacement(int(eq)+1);end do
    call assert_close(sum_u/4._rk,(lambda_target-1._rk)*length,2.e-8_rk,2.e-7_rk,'Hyperelastic Newton target stretch')
end program ver_v090_005_hyperelastic_newton
