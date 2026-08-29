program patch_v050_004_plane_strain
    use fem_kinds, only : rk,id_kind
    use fem_linear_elastic_material, only : linear_elastic_material_t,constitutive_plane_strain
    use fem_linear_continuum, only : quad4_recover_plane,PLANE_MODE_STRAIN
    use fem_status, only : status_t
    use test_support, only : assert_true,assert_close
    implicit none
    type(linear_elastic_material_t) :: m
    type(status_t) :: status
    real(rk) :: x(2,4),u(8),d(3,3),eps_expected(3),sig_expected(3),sigma_zz_expected
    real(rk), allocatable :: strain(:,:),stress(:,:),points(:,:),stress_zz(:)
    integer :: a,p
    m=linear_elastic_material_t(id=2_id_kind,name='mat',young_modulus=12.0e6_rk,poisson_ratio=0.30_rk)
    x=reshape([0.0_rk,0.0_rk, 2.0_rk,0.0_rk, 2.0_rk,1.0_rk, 0.0_rk,1.0_rk],[2,4])
    do a=1,4
        u(2*a-1)=0.01_rk*x(1,a)
        u(2*a)=0.02_rk*x(2,a)
    end do
    eps_expected=[0.01_rk,0.02_rk,0.0_rk]
    call constitutive_plane_strain(m,d,status); call assert_true(status%is_ok(),'plane strain D')
    sig_expected=matmul(d,eps_expected)
    sigma_zz_expected=m%lame_lambda()*(eps_expected(1)+eps_expected(2))
    call quad4_recover_plane(x,m,PLANE_MODE_STRAIN,u,strain,stress,points,status,stress_zz)
    call assert_true(status%is_ok(),'plane strain recovery')
    do p=1,size(strain,2)
        call assert_close(maxval(abs(strain(:,p)-eps_expected)),0.0_rk,1.0e-12_rk,0.0_rk,'constant plane strain')
        call assert_close(maxval(abs(stress(:,p)-sig_expected)),0.0_rk,1.0e-6_rk,0.0_rk,'in-plane plane strain stress')
        call assert_close(stress_zz(p),sigma_zz_expected,1.0e-6_rk,1.0e-12_rk,'plane strain sigma zz')
    end do
    write(*,'(A)') 'PASS PATCH-V050-004 plane strain recovery including sigma_zz'
end program patch_v050_004_plane_strain
