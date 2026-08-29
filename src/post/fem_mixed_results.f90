module fem_mixed_results
    !! V0.10 mixed u-p post-processing data path.
    !! P0 pressure element-associated DOF olarak saklanir ve sonuc da element ID
    !! ile raporlanir. Nodal pressure interpolation bu baseline'in parcasi degildir.
    use fem_kinds, only : rk,id_kind,index_kind
    use fem_ids, only : INVALID_ID
    use fem_model, only : model_t
    use fem_fields, only : FIELD_ID_PRESSURE_P0
    use fem_element_registry, only : ELEMENT_MIXED_UP_HEX8_P0
    use fem_status, only : status_t,FEM_STATUS_INVALID_ARGUMENT,FEM_STATUS_SIZE_MISMATCH
    implicit none
    private
    type,public :: element_pressure_results_t
        integer(id_kind),allocatable :: element_ids(:)
        real(rk),allocatable :: pressure(:)
    contains
        procedure :: clear => pressure_results_clear
    end type element_pressure_results_t
    public :: recover_mixed_p0_pressure
contains
    subroutine pressure_results_clear(this)
        class(element_pressure_results_t),intent(inout)::this
        if(allocated(this%element_ids))deallocate(this%element_ids)
        if(allocated(this%pressure))deallocate(this%pressure)
    end subroutine pressure_results_clear

    subroutine recover_mixed_p0_pressure(model,active_unknowns,result,status)
        type(model_t),intent(in)::model
        real(rk),intent(in)::active_unknowns(:)
        type(element_pressure_results_t),intent(inout)::result
        type(status_t),intent(out)::status
        integer::e,n,k
        integer(index_kind)::dof_pos,constraint_pos
        integer(id_kind)::dof_id,eq
        call status%clear();call result%clear()
        if(size(active_unknowns)/=int(model%numbering%active_equation_count))then
            call status%set_error(FEM_STATUS_SIZE_MISMATCH,'Pressure recovery active unknown boyutu equation count ile uyusmuyor.');return
        end if
        if(.not.allocated(model%mesh%elements))then
            allocate(result%element_ids(0),result%pressure(0));return
        end if
        n=0
        do e=1,size(model%mesh%elements)
            if(model%mesh%elements(e)%formulation_id==ELEMENT_MIXED_UP_HEX8_P0)n=n+1
        end do
        allocate(result%element_ids(n),result%pressure(n));k=0
        do e=1,size(model%mesh%elements)
            if(model%mesh%elements(e)%formulation_id/=ELEMENT_MIXED_UP_HEX8_P0)cycle
            k=k+1;result%element_ids(k)=model%mesh%elements(e)%id
            dof_pos=model%dofs%find_by_address(model%mesh%elements(e)%id,FIELD_ID_PRESSURE_P0,1)
            if(dof_pos==0_index_kind)then
                call status%set_error(FEM_STATUS_INVALID_ARGUMENT,'Mixed pressure recovery icin P0 pressure DOF bulunamadi.');return
            end if
            dof_id=model%dofs%dofs(dof_pos)%id
            constraint_pos=model%constraints%find_position_by_dof(dof_id)
            if(constraint_pos/=0_index_kind)then
                result%pressure(k)=model%constraints%constraints(constraint_pos)%prescribed_value
            else
                eq=model%numbering%equation_of(dof_id)
                if(eq==INVALID_ID)then
                    call status%set_error(FEM_STATUS_INVALID_ARGUMENT,'Mixed pressure recovery aktif pressure equation bulamadi.');return
                end if
                result%pressure(k)=active_unknowns(int(eq)+1)
            end if
        end do
    end subroutine recover_mixed_p0_pressure
end module fem_mixed_results
