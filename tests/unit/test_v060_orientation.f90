program test_v060_orientation
    use fem_kinds,only:rk,id_kind
    use fem_model,only:model_t
    use fem_topology,only:TOPOLOGY_BAR2
    use fem_status,only:status_t
    use test_support,only:assert_true,assert_equal_id
    implicit none
    type(model_t)::model;type(status_t)::status
    real(rk)::axes(3,3)
    axes=0.0_rk;axes(1,1)=1.0_rk;axes(2,2)=1.0_rk;axes(3,3)=1.0_rk
    call model%mesh%add_node(1_id_kind,[0.0_rk,0.0_rk,0.0_rk],status)
    call model%mesh%add_node(2_id_kind,[1.0_rk,0.0_rk,0.0_rk],status)
    call model%mesh%add_element(10_id_kind,TOPOLOGY_BAR2,[1_id_kind,2_id_kind],status)
    call model%frames%add(77_id_kind,[0.0_rk,0.0_rk,0.0_rk],axes,status)
    call model%assign_element_orientation(10_id_kind,77_id_kind,status)
    call assert_true(status%is_ok(),"orientation frame assignment")
    call assert_equal_id(model%mesh%elements(1)%orientation_frame_id,77_id_kind,"element stores orientation frame ID")
    call model%assign_element_orientation(10_id_kind,99_id_kind,status)
    call assert_true(.not.status%is_ok(),"unknown orientation frame rejected")
end program
