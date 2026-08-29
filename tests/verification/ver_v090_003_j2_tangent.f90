program ver_v090_003_j2_tangent
    use fem_kinds, only : rk
    use fem_j2_plasticity
    use fem_tensor_notation, only : strain_voigt_to_tensor, stress_tensor_to_voigt
    use fem_status, only : status_t
    use test_support, only : assert_true
    implicit none
    type(j2_material_t) :: m
    type(j2_state_t) :: base,sp,sm
    type(j2_response_t) :: r,rp,rm
    type(status_t) :: status
    real(rk)::ev(6),ep(6),em(6),e(3,3),sigp(6),sigm(6),fd(6,6),h,err,scale
    integer::col
    m=j2_material_t(young_modulus=210.0e9_rk,poisson_ratio=0.3_rk,yield_stress=250.0e6_rk,isotropic_hardening_modulus=1.5e9_rk)
    call base%initialize();call base%begin_trial(status)
    ev=[0.0035_rk,-0.0004_rk,0.0001_rk,0.0007_rk,0.0002_rk,-0.0003_rk]
    call strain_voigt_to_tensor(ev,e);call j2_update(m,e,base,r,status);call assert_true(status%is_ok().and.r%plastic,'J2 tangent base plastic')
    h=1.0e-8_rk
    do col=1,6
        ep=ev;em=ev;ep(col)=ep(col)+h;em(col)=em(col)-h
        sp=base;sm=base
        ! finite difference must use the same committed state, not the base trial update
        sp%trial_plastic_strain=sp%committed_plastic_strain;sp%trial_equivalent_plastic_strain=sp%committed_equivalent_plastic_strain
        sm%trial_plastic_strain=sm%committed_plastic_strain;sm%trial_equivalent_plastic_strain=sm%committed_equivalent_plastic_strain
        call strain_voigt_to_tensor(ep,e);call j2_update(m,e,sp,rp,status);call assert_true(status%is_ok(),'J2 plus')
        call stress_tensor_to_voigt(rp%stress,sigp)
        call strain_voigt_to_tensor(em,e);call j2_update(m,e,sm,rm,status);call assert_true(status%is_ok(),'J2 minus')
        call stress_tensor_to_voigt(rm%stress,sigm);fd(:,col)=(sigp-sigm)/(2.0_rk*h)
    end do
    err=maxval(abs(r%tangent-fd));scale=max(1.0_rk,maxval(abs(fd)))
    if(err/scale>5e-5_rk)write(*,'(A,ES12.4)')'J2 tangent rel err=',err/scale
    call assert_true(err/scale<5e-5_rk,'J2 algorithmic tangent finite-difference ile uyusmali')
end program ver_v090_003_j2_tangent
