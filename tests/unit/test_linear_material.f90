program test_linear_material
    use fem_kinds, only : rk, id_kind, index_kind
    use fem_linear_elastic_material, only : linear_elastic_material_t, material_registry_t, &
        constitutive_3d, constitutive_plane_stress, constitutive_plane_strain, constitutive_axisymmetric
    use fem_status, only : status_t
    use test_support, only : assert_true, assert_close, assert_equal_index
    implicit none
    type(linear_elastic_material_t) :: m
    type(material_registry_t) :: reg
    type(status_t) :: status
    real(rk) :: d6(6,6), dps(3,3), dpe(3,3), dax(4,4), g
    m=linear_elastic_material_t(id=10_id_kind,name="Steel",young_modulus=210.0e9_rk,poisson_ratio=0.3_rk,density=7850.0_rk)
    call m%validate(status); call assert_true(status%is_ok(),"material validate")
    g=210.0e9_rk/(2.0_rk*1.3_rk)
    call assert_close(m%shear_modulus(),g,1.0e-4_rk,1.0e-13_rk,"shear modulus")
    call constitutive_3d(m,d6,status); call assert_true(status%is_ok(),"3D D")
    call assert_true(maxval(abs(d6-transpose(d6))) < 1.0e-6_rk,"3D constitutive symmetric")
    call assert_close(d6(4,4),g,1.0e-4_rk,1.0e-13_rk,"3D shear diagonal")
    call constitutive_plane_stress(m,dps,status); call assert_true(status%is_ok(),"plane stress D")
    call constitutive_plane_strain(m,dpe,status); call assert_true(status%is_ok(),"plane strain D")
    call constitutive_axisymmetric(m,dax,status); call assert_true(status%is_ok(),"axisym D")
    call assert_close(dps(3,3),g,1.0e-4_rk,1.0e-13_rk,"plane stress shear")
    call assert_close(dpe(3,3),g,1.0e-4_rk,1.0e-13_rk,"plane strain shear")
    call assert_close(dax(3,3),g,1.0e-4_rk,1.0e-13_rk,"axisym shear")
    call reg%add(m,status); call assert_true(status%is_ok(),"material registry add")
    call assert_equal_index(reg%count(),1_index_kind,"material registry count")
    m%poisson_ratio=0.5_rk; call m%validate(status); call assert_true(.not.status%is_ok(),"nu=0.5 rejected")
    write(*,'(A)') "PASS unit_linear_material"
end program test_linear_material
