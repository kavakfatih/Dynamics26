program test_numbering
    use fem_kinds,       only : rk, id_kind, index_kind
    use fem_ids,         only : INVALID_ID
    use fem_dofs,        only : dof_set_t
    use fem_constraints, only : constraint_set_t
    use fem_numbering,   only : equation_map_t
    use fem_status,      only : status_t
    use test_support, only : assert_true, assert_equal_id, assert_equal_index
    implicit none

    type(dof_set_t) :: dofs
    type(constraint_set_t) :: constraints
    type(equation_map_t) :: numbering
    type(status_t) :: status
    integer(id_kind) :: d(4), cid, temp
    integer :: i

    do i = 1, 4
        call dofs%add(100_id_kind + int(i, id_kind), 0_id_kind, 1, d(i), status)
        call assert_true(status%is_ok(), "DOF set kurulumu")
    end do
    call constraints%add(d(2), 0.0_rk, cid, status)
    call assert_true(status%is_ok(), "constraint kurulumu")
    call constraints%add(d(4), 1.0_rk, cid, status)
    call assert_true(status%is_ok(), "ikinci constraint kurulumu")

    call numbering%build(dofs, constraints, status)
    call assert_true(status%is_ok(), "equation numbering basarili olmali")
    call assert_equal_index(numbering%active_equation_count, 2_index_kind, "yalnizca serbest DOF sayisi")
    call assert_equal_id(numbering%equation_of(d(1)), 0_id_kind, "ilk serbest DOF equation 0")
    call assert_equal_id(numbering%equation_of(d(2)), INVALID_ID, "constraint'li DOF equation almaz")
    call assert_equal_id(numbering%equation_of(d(3)), 1_id_kind, "ikinci serbest DOF equation 1")
    call assert_equal_id(numbering%equation_of(d(4)), INVALID_ID, "constraint'li DOF equation almaz")

    ! Numbering ikinci kez kuruldugunda ayni model ayni equation ID'leri uretmeli.
    temp = numbering%equation_of(d(3))
    call numbering%build(dofs, constraints, status)
    call assert_true(status%is_ok(), "renumber basarili olmali")
    call assert_equal_id(numbering%equation_of(d(3)), temp, "numbering deterministik olmali")

    write(*, '(A)') "PASS unit_numbering"
end program test_numbering
