program test_error_paths
    !! Foundation katmaninda yalnızca başarılı akışları değil, hatalı kullanım
    !! durumlarını da kilitleyen testler. V0.2 ile mesh/DOF veri modeli büyüdüğünde
    !! bu hata sözleşmeleri üst katmanların güvenilir davranması için kritiktir.
    use fem_kinds, only : rk, index_kind
    use fem_status, only : status_t, FEM_STATUS_INVALID_ARGUMENT, &
                           FEM_STATUS_SIZE_MISMATCH, FEM_STATUS_NOT_INITIALIZED, &
                           FEM_STATUS_NUMERICAL_FAILURE
    use fem_vector_math, only : vector_dot, normalize_vector
    use fem_matrix_math, only : matrix_vector_product
    use fem_state_buffer, only : state_buffer_t
    use test_support, only : assert_true, assert_equal_int
    implicit none

    real(rk) :: a2(2), b3(3), dot_value
    real(rk) :: matrix22(2,2), zero_vector(3)
    real(rk), allocatable :: result(:)
    type(status_t) :: status
    type(state_buffer_t) :: state

    a2 = [1.0_rk, 2.0_rk]
    b3 = [1.0_rk, 2.0_rk, 3.0_rk]
    call vector_dot(a2, b3, dot_value, status)
    call assert_equal_int(status%code, FEM_STATUS_SIZE_MISMATCH, &
        "farklı boyutlu dot product SIZE_MISMATCH vermeli")

    zero_vector = 0.0_rk
    call normalize_vector(zero_vector, result, status)
    call assert_equal_int(status%code, FEM_STATUS_NUMERICAL_FAILURE, &
        "sıfır vektör normalize edilirken numerical failure vermeli")
    call assert_true(allocated(result), "başarısız normalize sonucu tanımlı dizi dönmeli")
    call assert_true(size(result) == 3, "başarısız normalize sonucu giriş boyutunu korumalı")

    matrix22 = 0.0_rk
    call matrix_vector_product(matrix22, b3, result, status)
    call assert_equal_int(status%code, FEM_STATUS_SIZE_MISMATCH, &
        "matris-vektör boyut uyuşmazlığı SIZE_MISMATCH vermeli")

    call state%begin_trial(status)
    call assert_equal_int(status%code, FEM_STATUS_NOT_INITIALIZED, &
        "initialize edilmemiş state begin_trial ile reddedilmeli")

    call state%initialize(-1_index_kind, status=status)
    call assert_equal_int(status%code, FEM_STATUS_INVALID_ARGUMENT, &
        "negatif state değişken sayısı reddedilmeli")

    call state%initialize(2_index_kind, status=status)
    call state%set_trial_value(0_index_kind, 1.0_rk, status)
    call assert_equal_int(status%code, FEM_STATUS_INVALID_ARGUMENT, &
        "1 tabanlı state indeksinin alt sınırı korunmalı")

    write(*, '(A)') "PASS unit_error_paths"
end program test_error_paths
