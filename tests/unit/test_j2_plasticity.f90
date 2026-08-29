program test_j2_plasticity
    use fem_kinds, only : rk
    use fem_j2_plasticity
    use fem_status, only : status_t
    use test_support, only : assert_true, assert_close
    implicit none
    type(j2_material_t) :: m
    type(j2_state_t) :: state
    type(j2_response_t) :: r
    type(status_t) :: status
    real(rk) :: e(3,3)
    m=j2_material_t(young_modulus=210.0e9_rk,poisson_ratio=0.3_rk,yield_stress=250.0e6_rk,isotropic_hardening_modulus=1.0e9_rk)
    call state%initialize();call state%begin_trial(status);call assert_true(status%is_ok(),'J2 begin trial')
    e=0.0_rk;e(1,1)=5.0e-4_rk
    call j2_update(m,e,state,r,status);call assert_true(status%is_ok(),'J2 elastic update');call assert_true(.not.r%plastic,'Sub-yield response elastic')
    call state%revert(status);call assert_close(state%trial_equivalent_plastic_strain,0.0_rk,1e-15_rk,0.0_rk,'Revert plastic history')
    e=0.0_rk;e(1,1)=3.0e-3_rk
    call j2_update(m,e,state,r,status);call assert_true(status%is_ok(),'J2 plastic update');call assert_true(r%plastic,'Above-yield response plastic')
    call assert_true(state%trial_equivalent_plastic_strain>0.0_rk,'Plastic alpha artmali')
    call state%commit(status);call assert_true(status%is_ok(),'J2 commit')
    call assert_close(state%committed_equivalent_plastic_strain,state%trial_equivalent_plastic_strain,1e-15_rk,1e-14_rk,'Committed alpha')
end program test_j2_plasticity
