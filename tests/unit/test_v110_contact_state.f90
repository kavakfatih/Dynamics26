program test_v110_contact_state
    use fem_kinds, only : rk, id_kind
    use fem_mesh, only : mesh_t
    use fem_contact_types
    use fem_status, only : status_t
    use test_support, only : assert_true, assert_close, assert_equal_int
    implicit none
    type(mesh_t)::mesh
    type(contact_pair_t)::pair
    type(contact_facet_t)::facet
    type(status_t)::status
    integer::i
    integer(id_kind),parameter::ids(5)=[1_id_kind,2_id_kind,3_id_kind,4_id_kind,9_id_kind]
    real(rk),parameter::x(3,5)=reshape([ &
        -1.0_rk,-1.0_rk,0.0_rk, 1.0_rk,-1.0_rk,0.0_rk, 1.0_rk,1.0_rk,0.0_rk, &
        -1.0_rk,1.0_rk,0.0_rk, 0.0_rk,0.0_rk,0.1_rk],[3,5])
    do i=1,5;call mesh%add_node(ids(i),x(:,i),status);end do
    facet=contact_facet_t(id=100_id_kind,node_ids=ids(1:4))
    pair%id=200_id_kind;allocate(pair%slave_node_ids(1));pair%slave_node_ids=[9_id_kind]
    allocate(pair%master_facets(1));pair%master_facets=[facet]
    pair%normal_penalty=1000.0_rk;pair%search_distance=0.5_rk
    call pair%prepare(mesh,status);call assert_true(status%is_ok(),"prepare state")
    call assert_close(pair%states(1)%committed_position(3),0.1_rk,1e-14_rk,1e-14_rk,"initial position")
    call pair%begin_trial(status);pair%states(1)%trial_normal_multiplier=12.0_rk
    pair%states(1)%trial_status=CONTACT_STATE_STICK
    call pair%commit(status)
    call assert_close(pair%states(1)%committed_normal_multiplier,12.0_rk,1e-14_rk,1e-14_rk,"commit multiplier")
    call assert_equal_int(pair%states(1)%committed_status,CONTACT_STATE_STICK,"commit state")
    call pair%begin_trial(status);pair%states(1)%trial_normal_multiplier=99.0_rk
    call pair%revert(status)
    call assert_close(pair%states(1)%trial_normal_multiplier,12.0_rk,1e-14_rk,1e-14_rk,"revert trial")
    write(*,'(A)')'PASS V0.11 contact state commit/revert'
end program
