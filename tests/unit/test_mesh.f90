program test_mesh
    use fem_kinds,  only : rk, id_kind, index_kind
    use fem_mesh,   only : mesh_t
    use fem_topology, only : TOPOLOGY_BAR2
    use fem_status, only : status_t
    use test_support, only : assert_true, assert_equal_index, assert_equal_id
    implicit none

    type(mesh_t) :: mesh
    type(status_t) :: status
    integer(id_kind) :: conn(2)

    call mesh%add_node(100_id_kind, [0.0_rk, 0.0_rk, 0.0_rk], status)
    call assert_true(status%is_ok(), "ilk node eklenmeli")
    call mesh%add_node(900_id_kind, [1.0_rk, 0.0_rk, 0.0_rk], status)
    call assert_true(status%is_ok(), "ikinci node eklenmeli")

    call assert_equal_index(mesh%node_count(), 2_index_kind, "node count")
    call assert_equal_index(mesh%find_node_position(100_id_kind), 1_index_kind, "Node ID 100 array position 1")
    call assert_equal_index(mesh%find_node_position(900_id_kind), 2_index_kind, "Node ID 900 array position 2")
    call assert_true(mesh%find_node_position(2_id_kind) == 0_index_kind, "array position Node ID gibi kullanilmamali")

    conn = [900_id_kind, 100_id_kind]
    call mesh%add_element(77_id_kind, TOPOLOGY_BAR2, conn, status)
    call assert_true(status%is_ok(), "BAR2 connectivity Node ID ile kabul edilmeli")
    call assert_equal_id(mesh%elements(1)%node_ids(1), 900_id_kind, "connectivity array index degil Node ID saklar")
    call mesh%validate_connectivity(status)
    call assert_true(status%is_ok(), "mesh connectivity gecerli olmali")

    call mesh%add_node(100_id_kind, [2.0_rk, 0.0_rk, 0.0_rk], status)
    call assert_true(.not. status%is_ok(), "duplicate Node ID reddedilmeli")

    conn = [100_id_kind, 123456_id_kind]
    call mesh%add_element(78_id_kind, TOPOLOGY_BAR2, conn, status)
    call assert_true(.not. status%is_ok(), "dangling Node ID connectivity reddedilmeli")

    conn = [100_id_kind, 100_id_kind]
    call mesh%add_element(79_id_kind, TOPOLOGY_BAR2, conn, status)
    call assert_true(.not. status%is_ok(), "degenerate duplicate connectivity reddedilmeli")

    write(*, '(A)') "PASS unit_mesh"
end program test_mesh
