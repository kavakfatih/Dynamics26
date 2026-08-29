program test_metadata
    use fem_metadata, only : MATRIX_GENERAL, MATRIX_SPD, STRESS_CAUCHY, &
                             STRESS_SECOND_PIOLA_KIRCHHOFF, STRESS_FIRST_PIOLA_KIRCHHOFF, &
                             RESULT_INTEGRATION_POINT, RESULT_NODAL_AVERAGED
    use test_support, only : assert_true
    implicit none

    call assert_true(MATRIX_GENERAL /= MATRIX_SPD, "matrix class enumlari benzersiz olmali")
    call assert_true(STRESS_CAUCHY /= STRESS_SECOND_PIOLA_KIRCHHOFF, &
        "stress measure metadata benzersiz olmali")
    call assert_true(STRESS_FIRST_PIOLA_KIRCHHOFF /= STRESS_SECOND_PIOLA_KIRCHHOFF, &
        "PK1 ve PK2 metadata benzersiz olmali")
    call assert_true(RESULT_INTEGRATION_POINT /= RESULT_NODAL_AVERAGED, &
        "result location metadata benzersiz olmali")
    write(*, '(A)') "PASS unit_metadata"
end program test_metadata
