program ver_v100_003_mixed_newton_shear
    use fem_kinds, only : rk,id_kind,index_kind
    use fem_model, only : model_t
    use fem_fields, only : FIELD_ID_DISPLACEMENT,FIELD_ID_PRESSURE_P0
    use fem_topology, only : TOPOLOGY_HEX8
    use fem_element_registry, only : ELEMENT_MIXED_UP_HEX8_P0
    use fem_hyperelastic_material, only : hyperelastic_material_t,HYPER_NEO_HOOKEAN
    use fem_nonlinear_assembly, only : nonlinear_system_t,evaluate_nonlinear_system
    use fem_nonlinear_solver, only : nonlinear_solver_options_t,nonlinear_static_result_t,solve_nonlinear_static
    use fem_status, only : status_t
    use test_support, only : assert_true,assert_close
    implicit none
    type(model_t) :: model
    type(status_t) :: status
    type(hyperelastic_material_t) :: mat
    type(nonlinear_system_t) :: target_system,final_system
    type(nonlinear_solver_options_t) :: options
    type(nonlinear_static_result_t) :: result
    integer(id_kind),parameter :: nodes(8)=[17_id_kind,91_id_kind,4_id_kind,250_id_kind,31_id_kind,8_id_kind,77_id_kind,12_id_kind]
    real(rk) :: x(3,8),gamma,actual,pressure_value
    real(rk), allocatable :: target(:)
    integer::a
    integer(index_kind)::pos
    integer(id_kind)::dof_id,eq,constraint_id,load_id

    call unit_cube(x);gamma=0.12_rk
    do a=1,8;call model%mesh%add_node(nodes(a),x(:,a),status);call assert_true(status%is_ok(),'add shear node');end do
    call model%mesh%add_element(330_id_kind,TOPOLOGY_HEX8,nodes,status);call assert_true(status%is_ok(),'add shear element')
    call model%mesh%assign_element_formulation(330_id_kind,ELEMENT_MIXED_UP_HEX8_P0,status);call assert_true(status%is_ok(),'shear mixed formulation')
    mat=hyperelastic_material_t(id=44_id_kind,name='Mixed shear NH',model=HYPER_NEO_HOOKEAN,bulk_modulus=2.0e9_rk,c10=0.9e6_rk)
    call model%hyperelastic_materials%add(mat,status);call assert_true(status%is_ok(),'shear material')
    call model%mesh%assign_element_properties(330_id_kind,44_id_kind,-1_id_kind,status);call assert_true(status%is_ok(),'shear props')
    call model%initialize_standard_registries(status);call assert_true(status%is_ok(),'shear registries')
    call model%build_nodal_field_dofs(FIELD_ID_DISPLACEMENT,status);call assert_true(status%is_ok(),'shear u dofs')
    call model%build_element_field_dofs(FIELD_ID_PRESSURE_P0,status);call assert_true(status%is_ok(),'shear p dof')

    ! Rigid-body eliminasyonu; manufactured simple shear u_x=gamma*y bu DOF'larda sifirdir.
    call constrain(nodes(1),1);call constrain(nodes(1),2);call constrain(nodes(1),3)
    call constrain(nodes(2),2);call constrain(nodes(2),3);call constrain(nodes(4),3)
    call model%renumber(status);call assert_true(status%is_ok(),'shear numbering')
    allocate(target(int(model%numbering%active_equation_count)));target=0.0_rk
    do a=1,8
        pos=model%dofs%find_by_address(nodes(a),FIELD_ID_DISPLACEMENT,1);dof_id=model%dofs%dofs(pos)%id;eq=model%numbering%equation_of(dof_id)
        if(eq>=0_id_kind)target(int(eq)+1)=gamma*x(2,a)
    end do
    ! J=1 simple shear -> mixed pressure constraint p=K(J-1)=0.
    pos=model%dofs%find_by_address(330_id_kind,FIELD_ID_PRESSURE_P0,1);dof_id=model%dofs%dofs(pos)%id;eq=model%numbering%equation_of(dof_id)
    target(int(eq)+1)=0.0_rk
    call evaluate_nonlinear_system(model,target,target_system,status);call assert_true(status%is_ok(),'manufactured target system')
    call assert_close(target_system%pressure_residual_norm,0.0_rk,1.e-12_rk,0.0_rk,'target pressure residual zero')

    ! Exact active-equation internal force manufactured external load olur.
    do a=1,size(model%numbering%dof_ids)
        if(model%numbering%equation_ids(a)<0_id_kind)cycle
        pos=model%dofs%find_position(model%numbering%dof_ids(a))
        if(model%dofs%dofs(pos)%field_id/=FIELD_ID_DISPLACEMENT)cycle
        eq=model%numbering%equation_ids(a)
        if(abs(target_system%internal_force(int(eq)+1))>1.e-8_rk)then
            call model%loads%add(model%numbering%dof_ids(a),target_system%internal_force(int(eq)+1),load_id,status)
            call assert_true(status%is_ok(),'manufactured nodal load')
        end if
    end do

    options%initial_load_increment=0.25_rk;options%maximum_load_increment=0.5_rk;options%minimum_load_increment=1.e-4_rk
    options%max_iterations=30;options%line_search=.true.;options%residual_relative_tolerance=1.e-9_rk
    options%pressure_residual_relative_tolerance=1.e-9_rk;options%pressure_residual_absolute_tolerance=1.e-12_rk
    options%displacement_relative_tolerance=1.e-9_rk
    call solve_nonlinear_static(model,options,result,status)
    call assert_true(status%is_ok(),'mixed Newton solve status');call assert_true(result%converged,'mixed Newton converged')
    call assert_true(size(result%active_displacement)==size(target),'mixed solution size')
    call assert_true(maxval(abs(result%active_displacement-target))<2.e-8_rk,'mixed Newton recovers manufactured simple shear')
    call evaluate_nonlinear_system(model,result%active_displacement,final_system,status,1.0_rk);call assert_true(status%is_ok(),'final mixed system')
    call assert_true(final_system%pressure_residual_norm<1.e-11_rk,'final pressure residual converged')
    pos=model%dofs%find_by_address(330_id_kind,FIELD_ID_PRESSURE_P0,1);dof_id=model%dofs%dofs(pos)%id;eq=model%numbering%equation_of(dof_id)
    pressure_value=result%active_displacement(int(eq)+1)
    call assert_close(pressure_value,0.0_rk,1.e-5_rk,0.0_rk,'simple shear pressure remains zero')
    pos=model%dofs%find_by_address(nodes(3),FIELD_ID_DISPLACEMENT,1);dof_id=model%dofs%dofs(pos)%id;eq=model%numbering%equation_of(dof_id)
    actual=result%active_displacement(int(eq)+1);call assert_close(actual,gamma,2.e-8_rk,2.e-8_rk,'top shear displacement')
    write(*,'(A)')'PASS VER-V100-003 mixed Newton manufactured simple shear'
contains
    subroutine constrain(node_id,component)
        integer(id_kind),intent(in)::node_id
        integer,intent(in)::component
        integer(index_kind)::dpos
        integer(id_kind)::did
        dpos=model%dofs%find_by_address(node_id,FIELD_ID_DISPLACEMENT,component);did=model%dofs%dofs(dpos)%id
        call model%constraints%add(did,0.0_rk,constraint_id,status);call assert_true(status%is_ok(),'mixed constraint')
    end subroutine constrain
    subroutine unit_cube(coords)
        real(rk),intent(out)::coords(3,8)
        coords(:,1)=[0._rk,0._rk,0._rk];coords(:,2)=[1._rk,0._rk,0._rk]
        coords(:,3)=[1._rk,1._rk,0._rk];coords(:,4)=[0._rk,1._rk,0._rk]
        coords(:,5)=[0._rk,0._rk,1._rk];coords(:,6)=[1._rk,0._rk,1._rk]
        coords(:,7)=[1._rk,1._rk,1._rk];coords(:,8)=[0._rk,1._rk,1._rk]
    end subroutine unit_cube
end program ver_v100_003_mixed_newton_shear
