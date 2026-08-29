program ver_v090_007_j2_loading
    use fem_kinds, only : rk
    use fem_j2_plasticity
    use fem_status, only : status_t
    use test_support, only : assert_true,assert_close
    implicit none
    type(j2_material_t)::m
    type(j2_state_t)::state
    type(j2_response_t)::r
    type(status_t)::status
    real(rk)::e(3,3),alpha_after,expected_yield
    m=j2_material_t(young_modulus=210e9_rk,poisson_ratio=.3_rk,yield_stress=250e6_rk,isotropic_hardening_modulus=2e9_rk)
    call state%initialize();call state%begin_trial(status)
    e=0._rk;e(1,1)=4e-4_rk;call j2_update(m,e,state,r,status)
    call assert_true(status%is_ok().and..not.r%plastic,'J2 initial elastic loading')
    call state%commit(status);call state%begin_trial(status)
    e(1,1)=4e-3_rk;call j2_update(m,e,state,r,status)
    call assert_true(status%is_ok().and.r%plastic,'J2 yield and return mapping')
    expected_yield=m%yield_stress+m%isotropic_hardening_modulus*state%trial_equivalent_plastic_strain
    call assert_close(r%equivalent_stress,expected_yield,50._rk,2e-10_rk,'Returned stress hardening surface uzerinde')
    call state%commit(status);alpha_after=state%committed_equivalent_plastic_strain
    call state%begin_trial(status)
    e(1,1)=3.5e-3_rk;call j2_update(m,e,state,r,status)
    call assert_true(status%is_ok().and..not.r%plastic,'Small unloading increment elastic olmali')
    call assert_close(state%trial_equivalent_plastic_strain,alpha_after,1e-15_rk,1e-13_rk,'Elastic unloading alpha degistirmez')
    call state%revert(status)
    call assert_close(state%trial_equivalent_plastic_strain,alpha_after,1e-15_rk,1e-13_rk,'J2 revert committed history')
end program ver_v090_007_j2_loading
