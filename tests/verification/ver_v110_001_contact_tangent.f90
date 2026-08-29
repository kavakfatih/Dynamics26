program ver_v110_001_contact_tangent
    use fem_kinds, only : rk, id_kind
    use fem_contact_types
    use fem_contact_search, only : contact_search_result_t
    use fem_contact_enforcement, only : contact_point_response_t, evaluate_contact_point
    use fem_status, only : status_t
    use test_support, only : assert_true, assert_close
    implicit none
    type(contact_pair_t)::pair
    type(contact_point_state_t)::state
    type(contact_search_result_t)::search
    type(contact_point_response_t)::resp,rp,rm
    type(status_t)::status
    real(rk)::x(3),xp(3),xm(3),fd(3,3),h
    integer::j,i
    pair%id=1_id_kind;pair%normal_penalty=2.5e4_rk;pair%tangential_penalty=8.0e3_rk
    pair%friction_coefficient=0.35_rk;pair%friction_model=CONTACT_FRICTION_COULOMB
    pair%enforcement=CONTACT_ENFORCEMENT_PENALTY;pair%search_distance=1.0_rk
    state%initialized=.true.;state%committed_position=[0.0_rk,0.0_rk,0.0_rk]
    state%committed_master_facet_id=10_id_kind
    search%found=.true.;search%facet_id=10_id_kind;search%normal=[0.0_rk,0.0_rk,1.0_rk]
    search%gap=-0.02_rk
    x=[0.001_rk,0.0_rk,-0.02_rk]
    call evaluate_contact_point(pair,state,x,search,resp,status)
    call assert_true(status%is_ok().and.resp%state==CONTACT_STATE_STICK,"stick response")
    h=1.0e-7_rk
    do j=1,3
        xp=x;xm=x;xp(j)=xp(j)+h;xm(j)=xm(j)-h
        ! Rigid plane gap varies with z perturbation.
        search%gap=xp(3);call evaluate_contact_point(pair,state,xp,search,rp,status)
        search%gap=xm(3);call evaluate_contact_point(pair,state,xm,search,rm,status)
        fd(:,j)=-(rp%force-rm%force)/(2.0_rk*h)
    end do
    do i=1,3;do j=1,3
        call assert_close(resp%effective_tangent(i,j),fd(i,j),2.0e-4_rk,2.0e-6_rk,"stick consistent tangent")
    end do;end do

    ! Slip state away from Coulomb transition; x slip is large.
    x=[0.20_rk,0.03_rk,-0.02_rk];search%gap=x(3)
    call evaluate_contact_point(pair,state,x,search,resp,status)
    call assert_true(status%is_ok().and.resp%state==CONTACT_STATE_SLIP,"slip response")
    do j=1,3
        xp=x;xm=x;xp(j)=xp(j)+h;xm(j)=xm(j)-h
        search%gap=xp(3);call evaluate_contact_point(pair,state,xp,search,rp,status)
        search%gap=xm(3);call evaluate_contact_point(pair,state,xm,search,rm,status)
        fd(:,j)=-(rp%force-rm%force)/(2.0_rk*h)
    end do
    do i=1,3;do j=1,3
        call assert_close(resp%effective_tangent(i,j),fd(i,j),5.0e-3_rk,5.0e-5_rk,"slip consistent tangent")
    end do;end do
    write(*,'(A)')'PASS VER-V110-001 contact consistent tangent'
end program
