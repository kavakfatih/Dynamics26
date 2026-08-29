program test_stvk_material
    use fem_kinds, only : rk, id_kind
    use fem_stvk_material, only : stvk_response
    use fem_linear_elastic_material, only : linear_elastic_material_t
    use fem_status, only : status_t
    use test_support, only : assert_true, assert_close
    implicit none
    type(linear_elastic_material_t) :: mat
    type(status_t) :: status
    real(rk) :: e(3,3),s(3,3),c(6,6),lambda,g

    mat=linear_elastic_material_t(id=1_id_kind,name="StVK test",young_modulus=210.0e9_rk,poisson_ratio=0.3_rk)
    e=0.0_rk;e(1,1)=0.01_rk
    call stvk_response(mat,e,s,c,status)
    call assert_true(status%is_ok(), "stvk status")
    lambda=mat%lame_lambda();g=mat%shear_modulus()
    call assert_close(s(1,1), (lambda+2.0_rk*g)*0.01_rk, 1.0e-4_rk, 1.0e-4_rk, "S11")
    call assert_close(s(2,2), lambda*0.01_rk, 1.0e-4_rk, 1.0e-4_rk, "S22")
    call assert_close(c(4,4), g, 1.0e-4_rk, 1.0e-4_rk, "C44")
end program test_stvk_material
