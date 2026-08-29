program test_ids
    use fem_kinds, only : id_kind
    use fem_ids, only : INVALID_ID, id_is_valid
    use test_support, only : assert_true
    implicit none

    call assert_true(.not. id_is_valid(INVALID_ID), "INVALID_ID gecersiz olmali")
    call assert_true(id_is_valid(0_id_kind), "0 kimligi V0.1.0 politikasinda gecerlidir")
    call assert_true(id_is_valid(42_id_kind), "pozitif kimlik gecerli olmali")
    write(*, '(A)') "PASS unit_ids"
end program test_ids
