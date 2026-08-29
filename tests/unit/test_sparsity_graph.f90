program test_sparsity_graph
    use fem_kinds, only : id_kind, index_kind
    use fem_element_dof_map, only : element_dof_map_t
    use fem_sparsity_graph, only : sparsity_graph_t
    use fem_status, only : status_t
    use test_support, only : assert_true, assert_equal_index, assert_equal_id
    implicit none

    type(element_dof_map_t) :: maps(2)
    type(sparsity_graph_t) :: graph
    type(status_t) :: status

    allocate(maps(1)%equation_ids(2), maps(2)%equation_ids(2))
    maps(1)%equation_ids = [0_id_kind, 1_id_kind]
    maps(2)%equation_ids = [1_id_kind, 2_id_kind]

    call graph%build(3_index_kind, maps, status)
    call assert_true(status%is_ok(), "chain sparsity graph")
    call assert_equal_index(graph%nnz(), 7_index_kind, "3 equation chain nnz")
    call assert_equal_index(graph%row_ptr(1), 1_index_kind, "row0 start")
    call assert_equal_index(graph%row_ptr(2), 3_index_kind, "row1 start")
    call assert_equal_index(graph%row_ptr(3), 6_index_kind, "row2 start")
    call assert_equal_index(graph%row_ptr(4), 8_index_kind, "csr sentinel")
    call assert_equal_id(graph%col_ind(1), 0_id_kind, "row0 col0")
    call assert_equal_id(graph%col_ind(2), 1_id_kind, "row0 col1")
    call assert_true(graph%contains(1_id_kind, 2_id_kind), "1-2 structural coupling")
    call assert_true(graph%contains(2_id_kind, 1_id_kind), "symmetric structural coupling")
    call assert_true(.not. graph%contains(0_id_kind, 2_id_kind), "non-neighbor structural zero")

    write(*,'(A)') "PASS unit_sparsity_graph"
end program test_sparsity_graph
