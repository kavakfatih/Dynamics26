program test_v110_contact_geometry
    use fem_kinds, only : rk, id_kind
    use fem_mesh, only : mesh_t
    use fem_contact_types, only : contact_facet_t, contact_pair_t, CONTACT_ENFORCEMENT_PENALTY
    use fem_contact_geometry, only : facet_normal, closest_point_on_facet
    use fem_contact_search, only : contact_search_result_t, search_master_facet
    use fem_status, only : status_t
    use test_support, only : assert_true, assert_close, assert_equal_int
    implicit none
    type(mesh_t)::mesh
    type(contact_facet_t)::facet1,facet2
    type(contact_pair_t)::pair
    type(contact_search_result_t)::search
    type(status_t)::status
    real(rk)::n(3),cp(3),gap,distance

    call mesh%add_node(10_id_kind,[-1.0_rk,-1.0_rk,0.0_rk],status)
    call mesh%add_node(20_id_kind,[ 1.0_rk,-1.0_rk,0.0_rk],status)
    call mesh%add_node(30_id_kind,[ 1.0_rk, 1.0_rk,0.0_rk],status)
    call mesh%add_node(40_id_kind,[-1.0_rk, 1.0_rk,0.0_rk],status)
    call mesh%add_node(50_id_kind,[-1.0_rk,-1.0_rk,2.0_rk],status)
    call mesh%add_node(60_id_kind,[ 1.0_rk,-1.0_rk,2.0_rk],status)
    call mesh%add_node(70_id_kind,[ 1.0_rk, 1.0_rk,2.0_rk],status)
    call mesh%add_node(80_id_kind,[-1.0_rk, 1.0_rk,2.0_rk],status)
    facet1=contact_facet_t(id=101_id_kind,node_ids=[10_id_kind,20_id_kind,30_id_kind,40_id_kind])
    facet2=contact_facet_t(id=102_id_kind,node_ids=[50_id_kind,60_id_kind,70_id_kind,80_id_kind])
    call facet_normal(mesh,facet1,n,status);call assert_true(status%is_ok(),"facet normal")
    call assert_close(n(3),1.0_rk,1e-14_rk,1e-14_rk,"facet ordering +z normal")
    call closest_point_on_facet(mesh,facet1,[0.25_rk,-0.4_rk,-0.10_rk],cp,n,gap,distance,status)
    call assert_true(status%is_ok(),"closest point")
    call assert_close(cp(1),0.25_rk,1e-13_rk,1e-13_rk,"closest x")
    call assert_close(cp(2),-0.4_rk,1e-13_rk,1e-13_rk,"closest y")
    call assert_close(gap,-0.10_rk,1e-13_rk,1e-13_rk,"signed gap penetration")

    pair%id=500_id_kind;allocate(pair%slave_node_ids(1));pair%slave_node_ids=[10_id_kind]
    allocate(pair%master_facets(2));pair%master_facets=[facet1,facet2]
    pair%normal_penalty=1.0e5_rk;pair%search_distance=0.5_rk;pair%enforcement=CONTACT_ENFORCEMENT_PENALTY
    call search_master_facet(mesh,pair,[0.1_rk,0.2_rk,1.85_rk],search,status)
    call assert_true(status%is_ok().and.search%found,"broad+narrow search")
    call assert_true(search%facet_id==102_id_kind,"nearest facet chosen")
    call assert_equal_int(search%broad_phase_candidates,1,"AABB broad phase prunes distant facet")
    write(*,'(A)')'PASS V0.11 contact geometry/search'
end program
