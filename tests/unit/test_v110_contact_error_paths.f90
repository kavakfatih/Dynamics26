program test_v110_contact_error_paths
    use fem_kinds, only : rk, id_kind
    use fem_model, only : model_t
    use fem_contact_types
    use fem_status, only : status_t
    use test_support, only : assert_true
    implicit none
    type(model_t)::model
    type(contact_pair_t)::pair
    type(contact_facet_t)::facet
    type(status_t)::status
    integer::i
    integer(id_kind),parameter::ids(5)=[1_id_kind,2_id_kind,3_id_kind,4_id_kind,5_id_kind]
    real(rk),parameter::x(3,5)=reshape([ &
      0.0_rk,0.0_rk,0.0_rk, 1.0_rk,0.0_rk,0.0_rk, 1.0_rk,1.0_rk,0.0_rk, &
      0.0_rk,1.0_rk,0.0_rk, 0.5_rk,0.5_rk,0.1_rk],[3,5])
    do i=1,5;call model%mesh%add_node(ids(i),x(:,i),status);end do
    facet=contact_facet_t(id=10_id_kind,node_ids=ids(1:4))
    pair%id=20_id_kind;allocate(pair%slave_node_ids(1));pair%slave_node_ids=[5_id_kind]
    allocate(pair%master_facets(1));pair%master_facets=[facet]
    pair%search_distance=0.2_rk;pair%normal_penalty=-1.0_rk
    call pair%validate(model%mesh,status);call assert_true(.not.status%is_ok(),"negative normal penalty rejected")
    pair%normal_penalty=1.0e4_rk;pair%friction_model=CONTACT_FRICTION_COULOMB;pair%tangential_penalty=0.0_rk
    call pair%validate(model%mesh,status);call assert_true(.not.status%is_ok(),"Coulomb without tangential penalty rejected")
    pair%friction_model=CONTACT_FRICTIONLESS;pair%master_facets(1)%node_ids(4)=999_id_kind
    call pair%validate(model%mesh,status);call assert_true(.not.status%is_ok(),"missing facet node rejected")
    write(*,'(A)')'PASS V0.11 contact error paths'
end program
