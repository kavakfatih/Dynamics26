program ver_v110_002_augmented_lagrangian
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
    type(contact_point_response_t)::resp
    type(status_t)::status
    pair%id=1_id_kind;pair%normal_penalty=1000.0_rk;pair%search_distance=1.0_rk
    pair%enforcement=CONTACT_ENFORCEMENT_AUGMENTED_LAGRANGIAN
    state%initialized=.true.;state%committed_position=0.0_rk
    search%found=.true.;search%facet_id=7_id_kind;search%normal=[0.0_rk,0.0_rk,1.0_rk]
    search%gap=-0.01_rk
    call evaluate_contact_point(pair,state,[0.0_rk,0.0_rk,-0.01_rk],search,resp,status)
    call assert_close(state%trial_normal_multiplier,10.0_rk,1e-12_rk,1e-12_rk,"AL first multiplier")
    call state%commit()
    call evaluate_contact_point(pair,state,[0.0_rk,0.0_rk,-0.01_rk],search,resp,status)
    call assert_close(state%trial_normal_multiplier,10.0_rk,1e-12_rk,1e-12_rk,"AL committed-state reevaluation invariant")
    search%gap=-0.015_rk
    call evaluate_contact_point(pair,state,[0.0_rk,0.0_rk,-0.015_rk],search,resp,status)
    call assert_close(state%trial_normal_multiplier,15.0_rk,1e-12_rk,1e-12_rk,"AL incremental penetration multiplier")
    call state%revert()
    call assert_close(state%trial_normal_multiplier,10.0_rk,1e-12_rk,1e-12_rk,"AL revert")
    search%gap=0.02_rk
    call evaluate_contact_point(pair,state,[0.0_rk,0.0_rk,0.02_rk],search,resp,status)
    call assert_close(state%trial_normal_multiplier,0.0_rk,1e-12_rk,1e-12_rk,"AL release")
    call assert_true(.not.resp%active,"AL opens after multiplier exhausted")
    write(*,'(A)')'PASS VER-V110-002 augmented Lagrangian state'
end program
