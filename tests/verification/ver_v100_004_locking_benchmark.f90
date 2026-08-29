program ver_v100_004_locking_benchmark
    use fem_kinds, only : rk,id_kind,index_kind
    use fem_model, only : model_t
    use fem_fields, only : FIELD_ID_DISPLACEMENT,FIELD_ID_PRESSURE_P0
    use fem_topology, only : TOPOLOGY_HEX8
    use fem_element_registry, only : ELEMENT_TOTAL_LAGRANGIAN_HEX8,ELEMENT_MIXED_UP_HEX8_P0
    use fem_hyperelastic_material, only : hyperelastic_material_t,HYPER_NEO_HOOKEAN
    use fem_nonlinear_assembly, only : nonlinear_system_t,evaluate_nonlinear_system
    use fem_linear_solver, only : linear_solver_options_t,linear_solver_statistics_t,LINEAR_SOLVER_DENSE_REFERENCE,solve_linear_system
    use fem_status, only : status_t
    use test_support, only : assert_true
    implicit none
    type(model_t) :: penalty_model,mixed_model
    type(nonlinear_system_t) :: penalty_system,mixed_system
    type(linear_solver_options_t) :: linear_options
    type(linear_solver_statistics_t) :: stats
    type(status_t) :: status
    real(rk),allocatable :: q0(:),penalty_solution(:),mixed_solution(:)
    real(rk) :: penalty_tip,mixed_tip,ratio

    call build_cantilever(penalty_model,.false.,status);call assert_true(status%is_ok(),'build penalty cantilever')
    call build_cantilever(mixed_model,.true.,status);call assert_true(status%is_ok(),'build mixed cantilever')

    allocate(q0(int(penalty_model%numbering%active_equation_count)));q0=0._rk
    call evaluate_nonlinear_system(penalty_model,q0,penalty_system,status,1._rk);call assert_true(status%is_ok(),'penalty reference tangent')
    deallocate(q0);allocate(q0(int(mixed_model%numbering%active_equation_count)));q0=0._rk
    call evaluate_nonlinear_system(mixed_model,q0,mixed_system,status,1._rk);call assert_true(status%is_ok(),'mixed reference tangent')

    linear_options%backend=LINEAR_SOLVER_DENSE_REFERENCE
    call solve_linear_system(penalty_system%tangent,penalty_system%residual,penalty_solution,linear_options,stats,status)
    call assert_true(status%is_ok(),'penalty reference-tangent solve')
    call solve_linear_system(mixed_system%tangent,mixed_system%residual,mixed_solution,linear_options,stats,status)
    call assert_true(status%is_ok(),'mixed reference-tangent solve')

    penalty_tip=average_tip_y(penalty_model,penalty_solution)
    mixed_tip=average_tip_y(mixed_model,mixed_solution)
    ratio=abs(mixed_tip)/max(abs(penalty_tip),tiny(1._rk))
    write(*,'(A,ES12.4,A,ES12.4,A,F10.4)')'locking benchmark penalty tip=',penalty_tip,' mixed tip=',mixed_tip,' ratio=',ratio
    call assert_true(penalty_tip<0._rk.and.mixed_tip<0._rk,'both formulations bend in load direction')
    call assert_true(ratio>1.20_rk,'mixed Q1/P0 is materially less volumetrically locked than penalty Q1')
    call assert_true(abs(mixed_tip)<0.05_rk,'reference-tangent benchmark remains in small displacement range')
    write(*,'(A)')'PASS VER-V100-004 nearly-incompressible reference-tangent locking benchmark'
contains
    subroutine build_cantilever(model,mixed,status_out)
        type(model_t),intent(inout)::model
        logical,intent(in)::mixed
        type(status_t),intent(out)::status_out
        integer,parameter::nx=4
        real(rk),parameter::length=4._rk,height=1._rk,width=1._rk,total_load=-100._rk
        integer(id_kind)::node_ids(0:nx,0:1,0:1),conn(8),element_id,dof_id,cid,lid
        real(rk)::x(3)
        type(hyperelastic_material_t)::mat
        integer::i,j,k,c
        integer(index_kind)::pos

        call model%clear();call status_out%clear()
        do i=0,nx
            do j=0,1
                do k=0,1
                    node_ids(i,j,k)=1000_id_kind+int(i,id_kind)*100_id_kind+int(j,id_kind)*10_id_kind+int(k,id_kind)
                    x=[length*real(i,rk)/real(nx,rk),height*real(j,rk),width*real(k,rk)]
                    call model%mesh%add_node(node_ids(i,j,k),x,status_out);if(.not.status_out%is_ok())return
                end do
            end do
        end do
        do i=0,nx-1
            conn=[node_ids(i,0,0),node_ids(i+1,0,0),node_ids(i+1,1,0),node_ids(i,1,0), &
                  node_ids(i,0,1),node_ids(i+1,0,1),node_ids(i+1,1,1),node_ids(i,1,1)]
            element_id=500_id_kind+int(i,id_kind)
            call model%mesh%add_element(element_id,TOPOLOGY_HEX8,conn,status_out);if(.not.status_out%is_ok())return
            if(mixed)then
                call model%mesh%assign_element_formulation(element_id,ELEMENT_MIXED_UP_HEX8_P0,status_out)
            else
                call model%mesh%assign_element_formulation(element_id,ELEMENT_TOTAL_LAGRANGIAN_HEX8,status_out)
            end if
            if(.not.status_out%is_ok())return
        end do
        mat=hyperelastic_material_t(id=77_id_kind,name='locking benchmark NH',model=HYPER_NEO_HOOKEAN, &
            bulk_modulus=1.e10_rk,c10=0.5e6_rk)
        call model%hyperelastic_materials%add(mat,status_out);if(.not.status_out%is_ok())return
        do i=0,nx-1
            element_id=500_id_kind+int(i,id_kind)
            call model%mesh%assign_element_properties(element_id,77_id_kind,-1_id_kind,status_out);if(.not.status_out%is_ok())return
        end do
        call model%initialize_standard_registries(status_out);if(.not.status_out%is_ok())return
        call model%build_nodal_field_dofs(FIELD_ID_DISPLACEMENT,status_out);if(.not.status_out%is_ok())return
        if(mixed)then
            call model%build_element_field_dofs(FIELD_ID_PRESSURE_P0,status_out);if(.not.status_out%is_ok())return
        end if
        do j=0,1
            do k=0,1
                do c=1,3
                    pos=model%dofs%find_by_address(node_ids(0,j,k),FIELD_ID_DISPLACEMENT,c);dof_id=model%dofs%dofs(pos)%id
                    call model%constraints%add(dof_id,0._rk,cid,status_out);if(.not.status_out%is_ok())return
                end do
            end do
        end do
        do j=0,1
            do k=0,1
                pos=model%dofs%find_by_address(node_ids(nx,j,k),FIELD_ID_DISPLACEMENT,2);dof_id=model%dofs%dofs(pos)%id
                call model%loads%add(dof_id,total_load/4._rk,lid,status_out);if(.not.status_out%is_ok())return
            end do
        end do
        call model%renumber(status_out)
    end subroutine build_cantilever

    real(rk) function average_tip_y(model,solution) result(value)
        type(model_t),intent(in)::model
        real(rk),intent(in)::solution(:)
        integer,parameter::nx=4
        integer::j,k
        integer(index_kind)::pos
        integer(id_kind)::nid,did,eq
        value=0._rk
        do j=0,1
            do k=0,1
                nid=1000_id_kind+int(nx,id_kind)*100_id_kind+int(j,id_kind)*10_id_kind+int(k,id_kind)
                pos=model%dofs%find_by_address(nid,FIELD_ID_DISPLACEMENT,2);did=model%dofs%dofs(pos)%id
                eq=model%numbering%equation_of(did);value=value+solution(int(eq)+1)/4._rk
            end do
        end do
    end function average_tip_y
end program ver_v100_004_locking_benchmark
