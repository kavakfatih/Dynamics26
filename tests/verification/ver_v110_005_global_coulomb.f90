program ver_v110_005_global_coulomb
    use fem_kinds, only : rk, id_kind, index_kind
    use fem_model, only : model_t
    use fem_fields, only : FIELD_ID_DISPLACEMENT
    use fem_topology, only : TOPOLOGY_HEX8
    use fem_element_registry, only : ELEMENT_TOTAL_LAGRANGIAN_HEX8
    use fem_linear_elastic_material, only : linear_elastic_material_t
    use fem_contact_types
    use fem_nonlinear_solver, only : nonlinear_solver_options_t, nonlinear_static_result_t, solve_nonlinear_static
    use fem_nonlinear_assembly, only : nonlinear_system_t, evaluate_nonlinear_system
    use fem_linear_solver, only : LINEAR_SOLVER_DENSE_REFERENCE
    use fem_status, only : status_t
    use test_support, only : assert_true, assert_close
    implicit none
    type(model_t)::model
    type(nonlinear_solver_options_t)::opt
    type(nonlinear_static_result_t)::result
    type(nonlinear_system_t)::system
    type(status_t)::status
    real(rk),parameter::mu=0.30_rk,normal_load=1000.0_rk,lateral_load=1000.0_rk
    call build_model(model,status);call assert_true(status%is_ok(),"global Coulomb model")
    opt%linear%backend=LINEAR_SOLVER_DENSE_REFERENCE;opt%initial_load_increment=0.2_rk;opt%maximum_load_increment=0.25_rk
    opt%minimum_load_increment=1.0e-5_rk;opt%max_iterations=30;opt%line_search=.true.
    call solve_nonlinear_static(model,opt,result,status)
    call assert_true(status%is_ok().and.result%converged,"global Coulomb Newton")
    call evaluate_nonlinear_system(model,result%active_displacement,system,status,1.0_rk)
    call assert_true(status%is_ok(),"final Coulomb evaluation")
    call assert_true(system%active_contact_count==4,"four active friction contacts")
    call assert_true(result%final_slip_contact_count>=1,"global Newton iteration reaches Coulomb slip")
    call assert_true(system%total_contact_tangential_force>0.0_rk,"nonzero friction force")
    call assert_true(system%total_contact_tangential_force<=mu*system%total_contact_normal_force*(1.0_rk+1.0e-8_rk), &
        "Coulomb resultant bounded by mu*N")
    call assert_close(system%total_contact_normal_force,normal_load,2.0e-2_rk,1.0e-4_rk,"normal equilibrium under friction")
    write(*,'(A)')'PASS VER-V110-005 global Coulomb contact'
    write(*,'(A,I0,A,I0)')'stick=',system%stick_contact_count,' slip=',system%slip_contact_count
    write(*,'(A,ES14.6,A,ES14.6)')'N=',system%total_contact_normal_force,' T=',system%total_contact_tangential_force
contains
    subroutine build_model(m,status)
        type(model_t),intent(inout)::m;type(status_t),intent(out)::status
        integer(id_kind),parameter::cube(8)=[11_id_kind,12_id_kind,13_id_kind,14_id_kind,15_id_kind,16_id_kind,17_id_kind,18_id_kind]
        integer(id_kind),parameter::master(4)=[21_id_kind,22_id_kind,23_id_kind,24_id_kind]
        real(rk)::x(3,8),xm(3,4)
        type(linear_elastic_material_t)::mat
        type(contact_pair_t)::pair;type(contact_facet_t)::facet
        integer(index_kind)::pos;integer(id_kind)::dof,cid,lid;integer::a,c
        call status%clear();call m%clear()
        x(:,1)=[-0.5_rk,-0.5_rk,0.0_rk];x(:,2)=[0.5_rk,-0.5_rk,0.0_rk];x(:,3)=[0.5_rk,0.5_rk,0.0_rk];x(:,4)=[-0.5_rk,0.5_rk,0.0_rk]
        x(:,5)=x(:,1)+[0.0_rk,0.0_rk,1.0_rk];x(:,6)=x(:,2)+[0.0_rk,0.0_rk,1.0_rk];x(:,7)=x(:,3)+[0.0_rk,0.0_rk,1.0_rk];x(:,8)=x(:,4)+[0.0_rk,0.0_rk,1.0_rk]
        xm=x(:,1:4)
        do a=1,8;call m%mesh%add_node(cube(a),x(:,a),status);if(.not.status%is_ok())return;end do
        do a=1,4;call m%mesh%add_node(master(a),xm(:,a),status);if(.not.status%is_ok())return;end do
        call m%mesh%add_element(70_id_kind,TOPOLOGY_HEX8,cube,status);if(.not.status%is_ok())return
        call m%mesh%assign_element_formulation(70_id_kind,ELEMENT_TOTAL_LAGRANGIAN_HEX8,status);if(.not.status%is_ok())return
        mat=linear_elastic_material_t(id=71_id_kind,name="Friction StVK",young_modulus=1.0e6_rk,poisson_ratio=0.30_rk)
        call m%materials%add(mat,status);if(.not.status%is_ok())return
        call m%mesh%assign_element_properties(70_id_kind,71_id_kind,-1_id_kind,status);if(.not.status%is_ok())return
        call m%initialize_standard_registries(status);if(.not.status%is_ok())return
        call m%build_nodal_field_dofs(FIELD_ID_DISPLACEMENT,status);if(.not.status%is_ok())return
        ! y tum cube node'larda kilitli; tek top x DOF rigid-x mode'u kaldirir.
        do a=1,8
            pos=m%dofs%find_by_address(cube(a),FIELD_ID_DISPLACEMENT,2);dof=m%dofs%dofs(pos)%id
            call m%constraints%add(dof,0.0_rk,cid,status);if(.not.status%is_ok())return
        end do
        pos=m%dofs%find_by_address(cube(5),FIELD_ID_DISPLACEMENT,1);dof=m%dofs%dofs(pos)%id
        call m%constraints%add(dof,0.0_rk,cid,status);if(.not.status%is_ok())return
        do a=1,4
            do c=1,3
                pos=m%dofs%find_by_address(master(a),FIELD_ID_DISPLACEMENT,c);dof=m%dofs%dofs(pos)%id
                call m%constraints%add(dof,0.0_rk,cid,status);if(.not.status%is_ok())return
            end do
        end do
        do a=5,8
            pos=m%dofs%find_by_address(cube(a),FIELD_ID_DISPLACEMENT,3);dof=m%dofs%dofs(pos)%id
            call m%loads%add(dof,-normal_load/4.0_rk,lid,status);if(.not.status%is_ok())return
        end do
        do a=6,8
            pos=m%dofs%find_by_address(cube(a),FIELD_ID_DISPLACEMENT,1);dof=m%dofs%dofs(pos)%id
            call m%loads%add(dof,lateral_load/3.0_rk,lid,status);if(.not.status%is_ok())return
        end do
        facet=contact_facet_t(id=80_id_kind,node_ids=master)
        pair%id=81_id_kind;allocate(pair%slave_node_ids(4));pair%slave_node_ids=cube(1:4)
        allocate(pair%master_facets(1));pair%master_facets=[facet]
        pair%enforcement=CONTACT_ENFORCEMENT_PENALTY;pair%friction_model=CONTACT_FRICTION_COULOMB
        pair%normal_penalty=1.0e8_rk;pair%tangential_penalty=5.0e6_rk;pair%friction_coefficient=mu
        pair%search_distance=0.05_rk;pair%activation_tolerance=1.0e-12_rk
        call m%contacts%add(pair,status);if(.not.status%is_ok())return
        call m%renumber(status)
    end subroutine build_model
end program
