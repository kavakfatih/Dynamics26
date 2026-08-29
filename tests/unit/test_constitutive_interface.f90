program test_constitutive_interface
    use fem_kinds, only : rk,id_kind
    use fem_hyperelastic_material, only : hyperelastic_material_t,HYPER_NEO_HOOKEAN
    use fem_j2_plasticity, only : j2_material_t,j2_state_t
    use fem_constitutive_interface
    use fem_status, only : status_t
    use test_support, only : assert_true,assert_equal_int
    implicit none
    type(hyperelastic_material_t)::hm
    type(j2_material_t)::pm
    type(j2_state_t)::ps
    type(constitutive_response_t)::r
    type(status_t)::status
    real(rk)::f(3,3),e(3,3)
    hm=hyperelastic_material_t(id=1_id_kind,name='interface',model=HYPER_NEO_HOOKEAN,bulk_modulus=20e6_rk,c10=1e6_rk)
    f=0._rk;f(1,1)=1.1_rk;f(2,2)=1._rk/sqrt(1.1_rk);f(3,3)=f(2,2)
    call evaluate_hyperelastic_material_point(hm,f,r,status);call assert_true(status%is_ok(),'hyper interface status')
    call assert_equal_int(r%stress_measure,STRESS_MEASURE_SECOND_PK,'hyper stress measure')
    call assert_equal_int(r%tangent_measure,TANGENT_MEASURE_DS_DE,'hyper tangent measure')
    call assert_true(r%tangent_is_consistent.and..not.r%stateful,'hyper interface flags')
    pm=j2_material_t(young_modulus=210e9_rk,poisson_ratio=.3_rk,yield_stress=250e6_rk,isotropic_hardening_modulus=1e9_rk)
    call ps%initialize();call ps%begin_trial(status);e=0._rk;e(1,1)=.003_rk
    call evaluate_j2_material_point(pm,e,ps,r,status);call assert_true(status%is_ok(),'J2 interface status')
    call assert_equal_int(r%stress_measure,STRESS_MEASURE_CAUCHY,'J2 stress measure')
    call assert_true(r%stateful.and.r%tangent_is_consistent,'J2 interface flags')
end program test_constitutive_interface
