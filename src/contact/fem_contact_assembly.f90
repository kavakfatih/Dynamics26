module fem_contact_assembly
    !! Contact contribution assembly. Contact force residual'a +f_c olarak,
    !! Newton effective tangent ise K_c=-df_c/du olarak eklenir.
    use fem_kinds, only : rk, id_kind, index_kind
    use fem_ids, only : INVALID_ID
    use fem_model, only : model_t
    use fem_fields, only : FIELD_ID_DISPLACEMENT
    use fem_contact_types, only : CONTACT_STATE_STICK, CONTACT_STATE_SLIP
    use fem_contact_search, only : contact_search_result_t, search_master_facet
    use fem_contact_enforcement, only : contact_point_response_t, evaluate_contact_point
    use fem_sparse_matrix, only : csr_matrix_t
    use fem_status, only : status_t, FEM_STATUS_INVALID_ARGUMENT
    implicit none
    private

    type, public :: contact_assembly_summary_t
        integer :: active_count=0
        integer :: stick_count=0
        integer :: slip_count=0
        real(rk) :: maximum_penetration=0.0_rk
        real(rk) :: total_normal_force=0.0_rk
        real(rk) :: total_tangential_force=0.0_rk
    end type contact_assembly_summary_t

    public :: assemble_contact_contributions
contains
    subroutine assemble_contact_contributions(model,active_unknowns,tangent,residual,summary,status)
        type(model_t),intent(inout)::model
        real(rk),intent(in)::active_unknowns(:)
        type(csr_matrix_t),intent(inout)::tangent
        real(rk),intent(inout)::residual(:)
        type(contact_assembly_summary_t),intent(out)::summary
        type(status_t),intent(out)::status
        type(contact_search_result_t)::search
        type(contact_point_response_t)::response
        real(rk)::u(3),x(3)
        integer(id_kind)::eq(3)
        logical::active_component(3)
        integer::ip,is,c,d
        integer(index_kind)::node_pos
        call status%clear();summary=contact_assembly_summary_t()
        if(.not.allocated(model%contacts%pairs))return
        call model%contacts%prepare(model%mesh,status);if(.not.status%is_ok())return
        call model%contacts%begin_trial(status);if(.not.status%is_ok())return
        do ip=1,size(model%contacts%pairs)
            do is=1,size(model%contacts%pairs(ip)%slave_node_ids)
                node_pos=model%mesh%find_node_position(model%contacts%pairs(ip)%slave_node_ids(is))
                if(node_pos==0_index_kind)then
                    call status%set_error(FEM_STATUS_INVALID_ARGUMENT,"Contact assembly slave node mesh icinde bulunamadi.");return
                end if
                call nodal_displacement_and_equations(model,model%contacts%pairs(ip)%slave_node_ids(is), &
                    active_unknowns,u,eq,active_component,status);if(.not.status%is_ok())return
                x=model%mesh%nodes(node_pos)%x+u
                call search_master_facet(model%mesh,model%contacts%pairs(ip),x,search,status);if(.not.status%is_ok())return
                call evaluate_contact_point(model%contacts%pairs(ip),model%contacts%pairs(ip)%states(is), &
                    x,search,response,status);if(.not.status%is_ok())return
                if(.not.response%active)cycle
                summary%active_count=summary%active_count+1
                if(response%state==CONTACT_STATE_STICK)summary%stick_count=summary%stick_count+1
                if(response%state==CONTACT_STATE_SLIP)summary%slip_count=summary%slip_count+1
                summary%maximum_penetration=max(summary%maximum_penetration,max(0.0_rk,-response%gap))
                summary%total_normal_force=summary%total_normal_force+response%normal_force
                summary%total_tangential_force=summary%total_tangential_force+response%tangential_force_norm
                do c=1,3
                    if(.not.active_component(c))cycle
                    residual(int(eq(c))+1)=residual(int(eq(c))+1)+response%force(c)
                    do d=1,3
                        if(.not.active_component(d))cycle
                        call tangent%add_value(eq(c),eq(d),response%effective_tangent(c,d),status)
                        if(.not.status%is_ok())return
                    end do
                end do
            end do
        end do
    end subroutine assemble_contact_contributions

    subroutine nodal_displacement_and_equations(model,node_id,active_unknowns,u,eq,active_component,status)
        type(model_t),intent(in)::model
        integer(id_kind),intent(in)::node_id
        real(rk),intent(in)::active_unknowns(:)
        real(rk),intent(out)::u(3)
        integer(id_kind),intent(out)::eq(3)
        logical,intent(out)::active_component(3)
        type(status_t),intent(out)::status
        integer::c
        integer(index_kind)::dof_pos,constraint_pos
        integer(id_kind)::dof_id
        call status%clear();u=0.0_rk;eq=INVALID_ID;active_component=.false.
        do c=1,3
            dof_pos=model%dofs%find_by_address(node_id,FIELD_ID_DISPLACEMENT,c)
            if(dof_pos==0_index_kind)then
                call status%set_error(FEM_STATUS_INVALID_ARGUMENT,"Contact slave node displacement DOF'u bulunamadi.");return
            end if
            dof_id=model%dofs%dofs(dof_pos)%id
            constraint_pos=model%constraints%find_position_by_dof(dof_id)
            if(constraint_pos/=0_index_kind)then
                u(c)=model%constraints%constraints(constraint_pos)%prescribed_value
            else
                eq(c)=model%numbering%equation_of(dof_id)
                if(eq(c)==INVALID_ID)then
                    call status%set_error(FEM_STATUS_INVALID_ARGUMENT,"Contact slave aktif DOF equation ID almamis.");return
                end if
                if(int(eq(c))+1>size(active_unknowns))then
                    call status%set_error(FEM_STATUS_INVALID_ARGUMENT,"Contact equation active unknown vector disinda.");return
                end if
                u(c)=active_unknowns(int(eq(c))+1);active_component(c)=.true.
            end if
        end do
    end subroutine nodal_displacement_and_equations
end module fem_contact_assembly
