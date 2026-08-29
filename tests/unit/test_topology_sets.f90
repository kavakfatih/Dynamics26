program test_topology_sets
    use fem_kinds,    only : id_kind, index_kind
    use fem_topology, only : topology_registry_t, TOPOLOGY_BAR2, TOPOLOGY_QUAD4
    use fem_sets,     only : set_registry_t, SET_KIND_NODE, SET_KIND_ELEMENT
    use fem_status,   only : status_t
    use test_support, only : assert_true, assert_equal_index, assert_equal_int
    implicit none

    type(topology_registry_t) :: topologies
    type(set_registry_t) :: sets
    type(status_t) :: status
    integer(index_kind) :: pos

    call topologies%register_standard(status)
    call assert_true(status%is_ok(), "standard topology registry")
    call assert_equal_index(topologies%count(), 3_index_kind, "BAR2 QUAD4 HEX8 kayitli")
    pos = topologies%find_position(TOPOLOGY_BAR2)
    call assert_true(pos > 0_index_kind, "BAR2 topology bulunmali")
    call assert_equal_int(topologies%topologies(pos)%node_count, 2, "BAR2 node count")
    pos = topologies%find_position(TOPOLOGY_QUAD4)
    call assert_equal_int(topologies%topologies(pos)%topological_dimension, 2, "QUAD4 topological dimension")

    call sets%add(10_id_kind, "fixed_nodes", SET_KIND_NODE, [100_id_kind, 900_id_kind], status)
    call assert_true(status%is_ok(), "node set eklenmeli")
    call assert_true(sets%sets(1)%contains(900_id_kind), "set member lookup")
    call sets%add(11_id_kind, "elements", SET_KIND_ELEMENT, [77_id_kind], status)
    call assert_true(status%is_ok(), "element set eklenmeli")
    call sets%add(12_id_kind, "bad", SET_KIND_NODE, [100_id_kind, 100_id_kind], status)
    call assert_true(.not. status%is_ok(), "duplicate set member reddedilmeli")

    write(*, '(A)') "PASS unit_topology_sets"
end program test_topology_sets
