program test_dofs_constraints
    use fem_kinds,       only : rk, id_kind, index_kind
    use fem_dofs,        only : dof_set_t
    use fem_constraints, only : constraint_set_t
    use fem_status,      only : status_t
    use test_support, only : assert_true, assert_equal_id, assert_equal_index
    implicit none

    type(dof_set_t) :: dofs
    type(constraint_set_t) :: constraints
    type(status_t) :: status
    integer(id_kind) :: d0, d1, cid

    call dofs%add(100_id_kind, 0_id_kind, 1, d0, status)
    call assert_true(status%is_ok(), "ilk DOF eklenmeli")
    call dofs%add(100_id_kind, 0_id_kind, 2, d1, status)
    call assert_true(status%is_ok(), "ikinci DOF eklenmeli")
    call assert_equal_id(d0, 0_id_kind, "DOF ID 0'dan baslar")
    call assert_equal_id(d1, 1_id_kind, "DOF ID deterministik artar")
    call assert_true(d0 /= 100_id_kind, "DOF ID Node ID degildir")
    call assert_equal_index(dofs%find_by_address(100_id_kind, 0_id_kind, 2), 2_index_kind, "DOF fiziksel adres lookup")

    call dofs%add(100_id_kind, 0_id_kind, 2, d1, status)
    call assert_true(.not. status%is_ok(), "duplicate DOF address reddedilmeli")

    call constraints%add(d0, 0.0_rk, cid, status)
    call assert_true(status%is_ok(), "constraint DOF ID ile eklenmeli")
    call assert_true(constraints%is_constrained(d0), "DOF constrained gorunmeli")
    call constraints%add(d0, 1.0_rk, cid, status)
    call assert_true(.not. status%is_ok(), "ayni DOF'a duplicate essential constraint reddedilmeli")

    write(*, '(A)') "PASS unit_dofs_constraints"
end program test_dofs_constraints
