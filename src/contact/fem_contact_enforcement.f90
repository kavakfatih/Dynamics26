module fem_contact_enforcement
    !! Rigid-master node contact enforcement. Response tangent Newton denkleminde
    !! kullanilan K_contact = -d(f_contact)/d(u_slave) isaretindedir.
    use fem_kinds, only : rk
    use fem_contact_types, only : contact_pair_t, contact_point_state_t, &
        CONTACT_ENFORCEMENT_PENALTY, CONTACT_ENFORCEMENT_AUGMENTED_LAGRANGIAN, &
        CONTACT_FRICTIONLESS, CONTACT_STATE_OPEN, CONTACT_STATE_STICK, CONTACT_STATE_SLIP
    use fem_contact_search, only : contact_search_result_t
    use fem_status, only : status_t, FEM_STATUS_INVALID_ARGUMENT
    implicit none
    private

    type, public :: contact_point_response_t
        logical :: active=.false.
        integer :: state=CONTACT_STATE_OPEN
        real(rk)::gap=huge(1.0_rk)
        real(rk)::normal_force=0.0_rk
        real(rk)::tangential_force_norm=0.0_rk
        real(rk)::force(3)=0.0_rk
        real(rk)::effective_tangent(3,3)=0.0_rk
    end type contact_point_response_t

    public :: evaluate_contact_point
contains
    subroutine evaluate_contact_point(pair,state,current_position,search,response,status)
        type(contact_pair_t),intent(in)::pair
        type(contact_point_state_t),intent(inout)::state
        real(rk),intent(in)::current_position(3)
        type(contact_search_result_t),intent(in)::search
        type(contact_point_response_t),intent(out)::response
        type(status_t),intent(out)::status
        real(rk)::n(3),p(3,3),eye(3,3),ds(3),qtrial(3),q(3),t(3)
        real(rk)::normal_force,qnorm,limit,kn,kt,mu,active_measure
        real(rk)::gradn(3),dqdx(3,3),tt(3,3)
        integer::i
        call status%clear();response=contact_point_response_t();call state%begin_trial()
        state%trial_position=current_position
        if(.not.search%found)then
            state%trial_master_facet_id=-1;state%trial_status=CONTACT_STATE_OPEN
            state%trial_normal_multiplier=0.0_rk;state%trial_gap=0.0_rk
            state%trial_tangential_traction=0.0_rk;return
        end if
        n=search%normal;response%gap=search%gap;kn=pair%normal_penalty;kt=pair%tangential_penalty;mu=pair%friction_coefficient
        eye=0.0_rk;do i=1,3;eye(i,i)=1.0_rk;end do
        p=eye-outer(n,n)
        select case(pair%enforcement)
        case(CONTACT_ENFORCEMENT_PENALTY)
            active_measure=-kn*search%gap
            normal_force=max(0.0_rk,active_measure)
            response%active=search%gap<=pair%activation_tolerance
            state%trial_normal_multiplier=0.0_rk
        case(CONTACT_ENFORCEMENT_AUGMENTED_LAGRANGIAN)
            ! Incremental AL state: ayni committed configuration yeniden evaluate
            ! edildiginde multiplier ikinci kez artmaz.
            active_measure=state%committed_normal_multiplier-kn*(search%gap-state%committed_gap)
            normal_force=max(0.0_rk,active_measure)
            response%active=normal_force>0.0_rk.or.search%gap<=pair%activation_tolerance
            state%trial_normal_multiplier=normal_force
        case default
            call status%set_error(FEM_STATUS_INVALID_ARGUMENT,"Contact enforcement tipi gecersiz.");return
        end select
        state%trial_master_facet_id=search%facet_id
        if(.not.response%active)then
            state%trial_status=CONTACT_STATE_OPEN;state%trial_gap=0.0_rk
            state%trial_tangential_traction=0.0_rk;return
        end if
        state%trial_gap=search%gap
        response%normal_force=normal_force
        response%force=normal_force*n
        ! Active-set tangent g=0 aninda dahi normal destek stiffness'i tasir.
        response%effective_tangent=kn*outer(n,n)
        if(pair%friction_model==CONTACT_FRICTIONLESS.or.normal_force<=0.0_rk)then
            state%trial_status=merge(CONTACT_STATE_STICK,CONTACT_STATE_OPEN,normal_force>0.0_rk)
            response%state=state%trial_status;state%trial_tangential_traction=0.0_rk;return
        end if
        ! Master rigid oldugu icin incremental relative tangential motion slave position farkidir.
        ! Facet degisirse eski tangential history tasinmaz.
        if(state%committed_master_facet_id/=search%facet_id)then
            qtrial=-kt*matmul(p,current_position-state%committed_position)
        else
            ds=matmul(p,current_position-state%committed_position)
            qtrial=state%committed_tangential_traction-kt*ds
        end if
        qnorm=sqrt(max(0.0_rk,dot_product(qtrial,qtrial)));limit=mu*normal_force
        if(qnorm<=limit+1.0e-12_rk*max(1.0_rk,limit))then
            q=qtrial;response%effective_tangent=response%effective_tangent+kt*p
            state%trial_status=CONTACT_STATE_STICK
        else
            if(qnorm<=tiny(1.0_rk))then
                q=0.0_rk;state%trial_status=CONTACT_STATE_STICK
            else
                t=qtrial/qnorm;q=limit*t;tt=outer(t,t)
                gradn=-kn*n
                dqdx=mu*outer(t,gradn)-mu*normal_force*kt/qnorm*matmul(eye-tt,p)
                response%effective_tangent=response%effective_tangent-dqdx
                state%trial_status=CONTACT_STATE_SLIP
            end if
        end if
        state%trial_tangential_traction=q;response%force=response%force+q
        response%tangential_force_norm=sqrt(max(0.0_rk,dot_product(q,q)))
        response%state=state%trial_status
    end subroutine evaluate_contact_point

    pure function outer(a,b) result(c)
        real(rk),intent(in)::a(3),b(3);real(rk)::c(3,3);integer::i,j
        do i=1,3;do j=1,3;c(i,j)=a(i)*b(j);end do;end do
    end function outer
end module fem_contact_enforcement
