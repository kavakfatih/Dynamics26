program test_linear_elements
    use fem_kinds, only : rk,id_kind
    use fem_linear_elastic_material, only : linear_elastic_material_t
    use fem_linear_continuum, only : quad4_stiffness_plane,quad4_stiffness_axisymmetric,hex8_stiffness,PLANE_MODE_STRESS
    use fem_linear_beam, only : beam2_frame_stiffness_2d
    use fem_status, only : status_t
    use test_support, only : assert_true,assert_close
    implicit none
    type(linear_elastic_material_t) :: m
    type(status_t) :: status
    real(rk) :: q(2,4),a(2,4),h(3,8),kq(8,8),ka(8,8),kh(24,24),kb(6,6),v8(8),v24(24),v6(6)
    m=linear_elastic_material_t(id=1_id_kind,name="steel",young_modulus=200.0e9_rk,poisson_ratio=0.3_rk,density=7800.0_rk)
    q=reshape([0.0_rk,0.0_rk, 2.0_rk,0.0_rk, 2.0_rk,1.0_rk, 0.0_rk,1.0_rk],[2,4])
    call quad4_stiffness_plane(q,m,0.01_rk,PLANE_MODE_STRESS,kq,status)
    call assert_true(status%is_ok(),"plane stiffness")
    call assert_true(maxval(abs(kq-transpose(kq)))<1.0e-4_rk,"plane stiffness symmetric")
    v8=[1.0_rk,2.0_rk,1.0_rk,2.0_rk,1.0_rk,2.0_rk,1.0_rk,2.0_rk]
    call assert_true(abs(dot_product(v8,matmul(kq,v8)))<1.0e-2_rk,"plane rigid translation zero energy")
    a=reshape([1.0_rk,0.0_rk, 2.0_rk,0.0_rk, 2.0_rk,1.0_rk, 1.0_rk,1.0_rk],[2,4])
    call quad4_stiffness_axisymmetric(a,m,ka,status); call assert_true(status%is_ok(),"axisym stiffness")
    call assert_true(maxval(abs(ka-transpose(ka)))<1.0e-3_rk,"axisym symmetric")
    h=reshape([0.0_rk,0.0_rk,0.0_rk, 1.0_rk,0.0_rk,0.0_rk, 1.0_rk,1.0_rk,0.0_rk, 0.0_rk,1.0_rk,0.0_rk, &
               0.0_rk,0.0_rk,1.0_rk, 1.0_rk,0.0_rk,1.0_rk, 1.0_rk,1.0_rk,1.0_rk, 0.0_rk,1.0_rk,1.0_rk],[3,8])
    call hex8_stiffness(h,m,kh,status); call assert_true(status%is_ok(),"hex stiffness")
    call assert_true(maxval(abs(kh-transpose(kh)))<1.0e-3_rk,"hex symmetric")
    v24=0.0_rk; v24(1:24:3)=1.0_rk
    call assert_true(abs(dot_product(v24,matmul(kh,v24)))<1.0e-2_rk,"hex rigid translation zero energy")
    call beam2_frame_stiffness_2d([0.0_rk,0.0_rk],[2.0_rk,0.0_rk],200.0e9_rk,1.0e-3_rk,2.0e-6_rk,kb,status)
    call assert_true(status%is_ok(),"beam stiffness")
    call assert_true(maxval(abs(kb-transpose(kb)))<1.0e-6_rk,"beam symmetric")
    v6=[1.0_rk,2.0_rk,0.0_rk,1.0_rk,2.0_rk,0.0_rk]
    call assert_close(dot_product(v6,matmul(kb,v6)),0.0_rk,1.0e-6_rk,0.0_rk,"beam rigid translation")
    write(*,'(A)') "PASS unit_linear_elements"
end program test_linear_elements
