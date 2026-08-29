program test_hyperelastic_material
    use fem_kinds, only : rk, id_kind
    use fem_hyperelastic_material
    use fem_status, only : status_t
    use test_support, only : assert_true, assert_close
    implicit none
    type(hyperelastic_material_t) :: m
    type(status_t) :: status
    real(rk) :: f(3,3),s(3,3),d(6,6),w

    f=0.0_rk;f(1,1)=1.0_rk;f(2,2)=1.0_rk;f(3,3)=1.0_rk
    m=hyperelastic_material_t(id=1_id_kind,name='NH',model=HYPER_NEO_HOOKEAN,bulk_modulus=2.0e9_rk,c10=1.5e6_rk)
    call hyperelastic_response(m,f,s,d,w,status)
    call assert_true(status%is_ok(),'Neo-Hookean identity response status')
    call assert_close(maxval(abs(s)),0.0_rk,1.0e-6_rk,0.0_rk,'Neo-Hookean identity stress zero')
    call assert_close(w,0.0_rk,1.0e-10_rk,0.0_rk,'Neo-Hookean identity energy zero')
    call assert_close(m%initial_shear_modulus(),3.0e6_rk,1.0e-6_rk,1.0e-12_rk,'Neo-Hookean G0=2C10')

    m=hyperelastic_material_t(id=2_id_kind,name='MR',model=HYPER_MOONEY_RIVLIN,bulk_modulus=2.0e9_rk,c10=1.0e6_rk,c01=0.5e6_rk)
    call m%validate(status);call assert_true(status%is_ok(),'Mooney validation')
    call assert_close(m%initial_shear_modulus(),3.0e6_rk,1.0e-6_rk,1.0e-12_rk,'Mooney G0')

    m=hyperelastic_material_t(id=3_id_kind,name='Yeoh',model=HYPER_YEOH,bulk_modulus=2.0e9_rk,c10=1.5e6_rk,c20=0.2e6_rk,c30=0.05e6_rk)
    call m%validate(status);call assert_true(status%is_ok(),'Yeoh validation')

    m=hyperelastic_material_t(id=4_id_kind,name='Ogden',model=HYPER_OGDEN,bulk_modulus=2.0e9_rk,ogden_term_count=2, &
        ogden_mu=[2.0e6_rk,1.0e6_rk,0.0_rk],ogden_alpha=[2.0_rk,-2.0_rk,0.0_rk])
    call m%validate(status);call assert_true(status%is_ok(),'Ogden validation')
    call hyperelastic_response(m,f,s,d,w,status);call assert_true(status%is_ok(),'Ogden identity response')
    call assert_close(maxval(abs(s)),0.0_rk,1.0e-5_rk,0.0_rk,'Ogden identity stress zero')


    ! Cok-terimli Ogden fitting'lerinde tekil mu_i katsayilari isaretli olabilir;
    ! baseline stabilite kapisi toplam baslangic kayma modulu sum(mu_i)>0 kosuludur.
    m=hyperelastic_material_t(id=5_id_kind,name='Ogden signed mu',model=HYPER_OGDEN,bulk_modulus=2.0e9_rk,ogden_term_count=2, &
        ogden_mu=[3.0e6_rk,-1.0e6_rk,0.0_rk],ogden_alpha=[1.6_rk,-2.5_rk,0.0_rk])
    call m%validate(status);call assert_true(status%is_ok(),'Ogden signed mu terms with positive total shear should validate')
    call assert_close(m%initial_shear_modulus(),2.0e6_rk,1.0e-6_rk,1.0e-12_rk,'Ogden signed-term G0=sum(mu)')

    m%ogden_mu=[1.0e6_rk,-2.0e6_rk,0.0_rk]
    call m%validate(status);call assert_true(.not.status%is_ok(),'Ogden non-positive sum(mu) should be rejected')
end program test_hyperelastic_material
