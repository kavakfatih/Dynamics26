program test_v100_mixed_error_paths
    use fem_kinds, only : rk,id_kind
    use fem_model, only : model_t
    use fem_fields, only : FIELD_ID_DISPLACEMENT,FIELD_ID_PRESSURE,FIELD_ID_PRESSURE_P0
    use fem_topology, only : TOPOLOGY_HEX8
    use fem_element_registry, only : ELEMENT_MIXED_UP_HEX8_P0
    use fem_hyperelastic_material, only : hyperelastic_material_t,HYPER_NEO_HOOKEAN
    use fem_linear_elastic_material, only : linear_elastic_material_t
    use fem_nonlinear_assembly, only : nonlinear_system_t,evaluate_nonlinear_system
    use fem_nonlinear_solver, only : nonlinear_solver_options_t,nonlinear_static_result_t,solve_nonlinear_static
    use fem_linear_solver, only : LINEAR_SOLVER_SPARSE_CG
    use fem_status, only : status_t,FEM_STATUS_INVALID_ARGUMENT
    use test_support, only : assert_true
    implicit none
    type(model_t) :: model
    type(status_t) :: status
    type(nonlinear_system_t) :: system
    type(nonlinear_solver_options_t) :: options
    type(nonlinear_static_result_t) :: result
    real(rk),allocatable :: q(:)

    call build_base(model,.true.,status);call assert_true(status%is_ok(),'error path base without pressure')
    call model%renumber(status);call assert_true(status%is_ok(),'numbering without pressure')
    allocate(q(int(model%numbering%active_equation_count)));q=0._rk
    call evaluate_nonlinear_system(model,q,system,status)
    call assert_true(status%code==FEM_STATUS_INVALID_ARGUMENT,'mixed formulation rejects missing P0 pressure DOF')
    deallocate(q)

    call build_base(model,.true.,status);call assert_true(status%is_ok(),'coverage base')
    call model%build_element_field_dofs(FIELD_ID_PRESSURE_P0,status);call assert_true(status%is_ok(),'coverage P0')
    call model%build_nodal_field_dofs(FIELD_ID_PRESSURE,status);call assert_true(status%is_ok(),'unused nodal pressure')
    call model%renumber(status);call assert_true(status%is_ok(),'coverage numbering')
    allocate(q(int(model%numbering%active_equation_count)));q=0._rk
    call evaluate_nonlinear_system(model,q,system,status)
    call assert_true(status%code==FEM_STATUS_INVALID_ARGUMENT,'unused active nodal pressure DOF rejected')
    deallocate(q)

    call build_base(model,.true.,status);call assert_true(status%is_ok(),'CG base')
    call model%build_element_field_dofs(FIELD_ID_PRESSURE_P0,status);call assert_true(status%is_ok(),'CG P0')
    options=nonlinear_solver_options_t();options%linear%backend=LINEAR_SOLVER_SPARSE_CG
    call solve_nonlinear_static(model,options,result,status)
    call assert_true(status%code==FEM_STATUS_INVALID_ARGUMENT,'CG rejected for mixed saddle-point tangent')

    call build_base(model,.false.,status);call assert_true(status%is_ok(),'linear material mixed base')
    call model%build_element_field_dofs(FIELD_ID_PRESSURE_P0,status);call assert_true(status%is_ok(),'linear mixed P0')
    call model%renumber(status);call assert_true(status%is_ok(),'linear mixed numbering')
    allocate(q(int(model%numbering%active_equation_count)));q=0._rk
    call evaluate_nonlinear_system(model,q,system,status)
    call assert_true(status%code==FEM_STATUS_INVALID_ARGUMENT,'mixed formulation requires hyperelastic material')

    write(*,'(A)')'PASS unit_v100_mixed_error_paths'
contains
    subroutine build_base(m,use_hyper,status_out)
        type(model_t),intent(inout)::m
        logical,intent(in)::use_hyper
        type(status_t),intent(out)::status_out
        integer(id_kind),parameter::nodes(8)=[1_id_kind,5_id_kind,9_id_kind,13_id_kind,17_id_kind,21_id_kind,25_id_kind,29_id_kind]
        real(rk)::x(3,8)
        type(hyperelastic_material_t)::hmat
        type(linear_elastic_material_t)::lmat
        integer::a
        call m%clear();call unit_cube(x)
        do a=1,8;call m%mesh%add_node(nodes(a),x(:,a),status_out);if(.not.status_out%is_ok())return;end do
        call m%mesh%add_element(80_id_kind,TOPOLOGY_HEX8,nodes,status_out);if(.not.status_out%is_ok())return
        call m%mesh%assign_element_formulation(80_id_kind,ELEMENT_MIXED_UP_HEX8_P0,status_out);if(.not.status_out%is_ok())return
        if(use_hyper)then
            hmat=hyperelastic_material_t(id=6_id_kind,name='mixed error NH',model=HYPER_NEO_HOOKEAN,bulk_modulus=1.e9_rk,c10=0.5e6_rk)
            call m%hyperelastic_materials%add(hmat,status_out)
        else
            lmat=linear_elastic_material_t(id=6_id_kind,name='invalid mixed linear',young_modulus=3.e6_rk,poisson_ratio=0.49_rk)
            call m%materials%add(lmat,status_out)
        end if
        if(.not.status_out%is_ok())return
        call m%mesh%assign_element_properties(80_id_kind,6_id_kind,-1_id_kind,status_out);if(.not.status_out%is_ok())return
        call m%initialize_standard_registries(status_out);if(.not.status_out%is_ok())return
        call m%build_nodal_field_dofs(FIELD_ID_DISPLACEMENT,status_out)
    end subroutine build_base
    subroutine unit_cube(coords)
        real(rk),intent(out)::coords(3,8)
        coords(:,1)=[0._rk,0._rk,0._rk];coords(:,2)=[1._rk,0._rk,0._rk]
        coords(:,3)=[1._rk,1._rk,0._rk];coords(:,4)=[0._rk,1._rk,0._rk]
        coords(:,5)=[0._rk,0._rk,1._rk];coords(:,6)=[1._rk,0._rk,1._rk]
        coords(:,7)=[1._rk,1._rk,1._rk];coords(:,8)=[0._rk,1._rk,1._rk]
    end subroutine unit_cube
end program test_v100_mixed_error_paths
