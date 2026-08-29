program reg_v010_001_contracts
    use fem_version, only : FEM_VERSION_STRING, FEM_PROJECT_SCHEMA_VERSION, &
                            FEM_RESULT_SCHEMA_VERSION, FEM_C_API_VERSION
    use fem_tensor_notation, only : VOIGT_XX, VOIGT_YY, VOIGT_ZZ, &
                                    VOIGT_XY, VOIGT_YZ, VOIGT_XZ
    use test_support, only : assert_equal_int, assert_equal_string
    implicit none

    call assert_equal_string(FEM_VERSION_STRING, "1.0.2", "application version contract")
    call assert_equal_int(FEM_PROJECT_SCHEMA_VERSION, 1, "project schema contract")
    call assert_equal_int(FEM_RESULT_SCHEMA_VERSION, 1, "result schema contract")
    call assert_equal_int(FEM_C_API_VERSION, 1, "C API contract")
    call assert_equal_int(VOIGT_XX, 1, "Voigt XX contract")
    call assert_equal_int(VOIGT_YY, 2, "Voigt YY contract")
    call assert_equal_int(VOIGT_ZZ, 3, "Voigt ZZ contract")
    call assert_equal_int(VOIGT_XY, 4, "Voigt XY contract")
    call assert_equal_int(VOIGT_YZ, 5, "Voigt YZ contract")
    call assert_equal_int(VOIGT_XZ, 6, "Voigt XZ contract")

    write(*, '(A)') "PASS REG-V010-001"
    write(*, '(A)') "contract=version:1.0.2;project_schema:1;result_schema:1;c_api:1"
    write(*, '(A)') "voigt=XX,YY,ZZ,XY,YZ,XZ"
    write(*, '(A)') "residual=FEXT-FINT"
end program reg_v010_001_contracts
