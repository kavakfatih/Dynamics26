module nonlinear_test_models
    use fem_kinds, only : rk, id_kind, index_kind
    use fem_model, only : model_t
    use fem_fields, only : FIELD_ID_DISPLACEMENT
    use fem_topology, only : TOPOLOGY_HEX8
    use fem_element_registry, only : ELEMENT_TOTAL_LAGRANGIAN_HEX8
    use fem_linear_elastic_material, only : linear_elastic_material_t
    use fem_status, only : status_t
    use fem_contact_types, only : contact_pair_t, contact_facet_t, &
        CONTACT_ENFORCEMENT_PENALTY, CONTACT_FRICTIONLESS
    implicit none
    private
    public :: build_uniaxial_stvk_hex8, build_contact_stvk_hex8
contains
    subroutine build_uniaxial_stvk_hex8(model, young_modulus, poisson_ratio, length, area, total_force, &
                                        right_x_dofs, status)
        type(model_t), intent(inout) :: model
        real(rk), intent(in) :: young_modulus, poisson_ratio, length, area, total_force
        integer(id_kind), intent(out) :: right_x_dofs(4)
        type(status_t), intent(out) :: status
        integer(id_kind), parameter :: node_ids(8)=[81_id_kind,7_id_kind,42_id_kind,5_id_kind, &
                                                     900_id_kind,11_id_kind,3_id_kind,77_id_kind]
        integer, parameter :: right_nodes(4)=[2,3,6,7]
        integer, parameter :: left_nodes(4)=[1,4,5,8]
        real(rk) :: b, x(3,8)
        type(linear_elastic_material_t) :: mat
        integer(index_kind) :: pos
        integer(id_kind) :: dof_id, constraint_id, load_id
        integer :: a, c, i

        call status%clear()
        call model%clear()
        right_x_dofs = -1_id_kind
        b = sqrt(area)
        x(:,1)=[0.0_rk,0.0_rk,0.0_rk]; x(:,2)=[length,0.0_rk,0.0_rk]
        x(:,3)=[length,b,0.0_rk];       x(:,4)=[0.0_rk,b,0.0_rk]
        x(:,5)=[0.0_rk,0.0_rk,b];       x(:,6)=[length,0.0_rk,b]
        x(:,7)=[length,b,b];             x(:,8)=[0.0_rk,b,b]
        do a=1,8
            call model%mesh%add_node(node_ids(a),x(:,a),status); if(.not.status%is_ok())return
        end do
        call model%mesh%add_element(600_id_kind,TOPOLOGY_HEX8,node_ids,status); if(.not.status%is_ok())return
        call model%mesh%assign_element_formulation(600_id_kind,ELEMENT_TOTAL_LAGRANGIAN_HEX8,status)
        if(.not.status%is_ok())return
        mat=linear_elastic_material_t(id=9_id_kind,name="V0.8 StVK verification", &
            young_modulus=young_modulus,poisson_ratio=poisson_ratio)
        call model%materials%add(mat,status); if(.not.status%is_ok())return
        call model%mesh%assign_element_properties(600_id_kind,9_id_kind,-1_id_kind,status); if(.not.status%is_ok())return
        call model%initialize_standard_registries(status); if(.not.status%is_ok())return
        call model%build_nodal_field_dofs(FIELD_ID_DISPLACEMENT,status); if(.not.status%is_ok())return

        ! Homojen uniaxial finite stretch icin y/z tum dugumlerde kilitli.
        do a=1,8
            do c=2,3
                pos=model%dofs%find_by_address(node_ids(a),FIELD_ID_DISPLACEMENT,c)
                dof_id=model%dofs%dofs(pos)%id
                call model%constraints%add(dof_id,0.0_rk,constraint_id,status); if(.not.status%is_ok())return
            end do
        end do
        ! Sol referans yuzunun x deplasmani sabit.
        do i=1,4
            a=left_nodes(i)
            pos=model%dofs%find_by_address(node_ids(a),FIELD_ID_DISPLACEMENT,1)
            dof_id=model%dofs%dofs(pos)%id
            call model%constraints%add(dof_id,0.0_rk,constraint_id,status); if(.not.status%is_ok())return
        end do
        ! Sag yuzde toplam nominal kuvvet dort esit nodal yuke bolunur.
        do i=1,4
            a=right_nodes(i)
            pos=model%dofs%find_by_address(node_ids(a),FIELD_ID_DISPLACEMENT,1)
            dof_id=model%dofs%dofs(pos)%id
            right_x_dofs(i)=dof_id
            call model%loads%add(dof_id,total_force/4.0_rk,load_id,status); if(.not.status%is_ok())return
        end do
        call model%renumber(status)
    end subroutine build_uniaxial_stvk_hex8


    subroutine build_contact_stvk_hex8(model,total_force,normal_penalty,enforcement,status)
        type(model_t),intent(inout)::model
        real(rk),intent(in)::total_force,normal_penalty
        integer,intent(in),optional::enforcement
        type(status_t),intent(out)::status
        integer(id_kind),parameter::cube_ids(8)=[101_id_kind,102_id_kind,103_id_kind,104_id_kind, &
                                                  105_id_kind,106_id_kind,107_id_kind,108_id_kind]
        integer(id_kind),parameter::master_ids(4)=[201_id_kind,202_id_kind,203_id_kind,204_id_kind]
        real(rk)::x(3,8),xm(3,4)
        type(linear_elastic_material_t)::mat
        type(contact_pair_t)::pair
        type(contact_facet_t)::facet
        integer(index_kind)::pos
        integer(id_kind)::dof_id,constraint_id,load_id
        integer::a,c,enf
        call status%clear();call model%clear()
        x(:,1)=[-0.5_rk,-0.5_rk,0.0_rk];x(:,2)=[0.5_rk,-0.5_rk,0.0_rk]
        x(:,3)=[ 0.5_rk, 0.5_rk,0.0_rk];x(:,4)=[-0.5_rk,0.5_rk,0.0_rk]
        x(:,5)=x(:,1)+[0.0_rk,0.0_rk,1.0_rk];x(:,6)=x(:,2)+[0.0_rk,0.0_rk,1.0_rk]
        x(:,7)=x(:,3)+[0.0_rk,0.0_rk,1.0_rk];x(:,8)=x(:,4)+[0.0_rk,0.0_rk,1.0_rk]
        xm(:,1)=x(:,1);xm(:,2)=x(:,2);xm(:,3)=x(:,3);xm(:,4)=x(:,4)
        do a=1,8;call model%mesh%add_node(cube_ids(a),x(:,a),status);if(.not.status%is_ok())return;end do
        do a=1,4;call model%mesh%add_node(master_ids(a),xm(:,a),status);if(.not.status%is_ok())return;end do
        call model%mesh%add_element(700_id_kind,TOPOLOGY_HEX8,cube_ids,status);if(.not.status%is_ok())return
        call model%mesh%assign_element_formulation(700_id_kind,ELEMENT_TOTAL_LAGRANGIAN_HEX8,status);if(.not.status%is_ok())return
        mat=linear_elastic_material_t(id=77_id_kind,name="Contact StVK",young_modulus=1.0e6_rk,poisson_ratio=0.30_rk)
        call model%materials%add(mat,status);if(.not.status%is_ok())return
        call model%mesh%assign_element_properties(700_id_kind,77_id_kind,-1_id_kind,status);if(.not.status%is_ok())return
        call model%initialize_standard_registries(status);if(.not.status%is_ok())return
        call model%build_nodal_field_dofs(FIELD_ID_DISPLACEMENT,status);if(.not.status%is_ok())return
        ! Cube x/y kilitli; z yonu contact tarafindan desteklenir.
        do a=1,8
            do c=1,2
                pos=model%dofs%find_by_address(cube_ids(a),FIELD_ID_DISPLACEMENT,c);dof_id=model%dofs%dofs(pos)%id
                call model%constraints%add(dof_id,0.0_rk,constraint_id,status);if(.not.status%is_ok())return
            end do
        end do
        ! Rigid master facet node'larinin tum displacement DOF'lari sabit.
        do a=1,4
            do c=1,3
                pos=model%dofs%find_by_address(master_ids(a),FIELD_ID_DISPLACEMENT,c);dof_id=model%dofs%dofs(pos)%id
                call model%constraints%add(dof_id,0.0_rk,constraint_id,status);if(.not.status%is_ok())return
            end do
        end do
        ! Top face compressive nodal loads.
        do a=5,8
            pos=model%dofs%find_by_address(cube_ids(a),FIELD_ID_DISPLACEMENT,3);dof_id=model%dofs%dofs(pos)%id
            call model%loads%add(dof_id,-total_force/4.0_rk,load_id,status);if(.not.status%is_ok())return
        end do
        facet=contact_facet_t(id=900_id_kind,node_ids=master_ids)
        pair%id=901_id_kind;allocate(pair%slave_node_ids(4));pair%slave_node_ids=cube_ids(1:4)
        allocate(pair%master_facets(1));pair%master_facets=[facet]
        enf=CONTACT_ENFORCEMENT_PENALTY;if(present(enforcement))enf=enforcement
        pair%enforcement=enf;pair%friction_model=CONTACT_FRICTIONLESS
        pair%normal_penalty=normal_penalty;pair%search_distance=0.05_rk;pair%activation_tolerance=1.0e-12_rk
        call model%contacts%add(pair,status);if(.not.status%is_ok())return
        call model%renumber(status)
    end subroutine build_contact_stvk_hex8

end module nonlinear_test_models
