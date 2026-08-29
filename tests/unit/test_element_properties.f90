program test_element_properties
    use fem_kinds,  only : rk, id_kind
    use fem_ids,    only : INVALID_ID
    use fem_mesh,   only : mesh_t
    use fem_topology, only : TOPOLOGY_BAR2
    use fem_status, only : status_t
    use test_support, only : assert_true, assert_equal_id
    implicit none

    type(mesh_t) :: mesh
    type(status_t) :: status

    call mesh%add_node(1_id_kind, [0.0_rk, 0.0_rk, 0.0_rk], status)
    call mesh%add_node(2_id_kind, [1.0_rk, 0.0_rk, 0.0_rk], status)
    call mesh%add_element(50_id_kind, TOPOLOGY_BAR2, [1_id_kind, 2_id_kind], status)
    call assert_true(status%is_ok(), "element kurulumu")

    call mesh%assign_element_properties(50_id_kind, 700_id_kind, 800_id_kind, status)
    call assert_true(status%is_ok(), "material/section ID link")
    call assert_equal_id(mesh%elements(1)%material_id, 700_id_kind, "material ID kalici link")
    call assert_equal_id(mesh%elements(1)%section_id, 800_id_kind, "section ID kalici link")

    call mesh%assign_element_properties(50_id_kind, 701_id_kind, INVALID_ID, status)
    call assert_true(status%is_ok(), "section kullanmayan element INVALID_ID tutabilir")
    call assert_equal_id(mesh%elements(1)%section_id, INVALID_ID, "section optional")

    write(*, '(A)') "PASS unit_element_properties"
end program test_element_properties
