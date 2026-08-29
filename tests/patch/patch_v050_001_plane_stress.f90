program patch_v050_001_plane_stress
    use fem_kinds, only : rk,id_kind
    use fem_linear_elastic_material, only : linear_elastic_material_t,constitutive_plane_stress
    use fem_linear_continuum, only : quad4_recover_plane,PLANE_MODE_STRESS
    use fem_status, only : status_t
    use test_support, only : assert_true,assert_close
    implicit none
    type(linear_elastic_material_t) :: m
    type(status_t) :: status
    real(rk) :: x(2,4),u(8),d(3,3),eps_expected(3),sig_expected(3)
    real(rk), allocatable :: strain(:,:),stress(:,:),points(:,:)
    integer :: a,p
    m=linear_elastic_material_t(id=1_id_kind,name="mat",young_modulus=10.0e6_rk,poisson_ratio=0.25_rk)
    x=reshape([0.0_rk,0.0_rk, 2.0_rk,0.0_rk, 2.0_rk,1.0_rk, 0.0_rk,1.0_rk],[2,4])
    do a=1,4
        u(2*a-1)=0.01_rk*x(1,a)+0.02_rk*x(2,a)
        u(2*a)= -0.03_rk*x(1,a)+0.04_rk*x(2,a)
    end do
    eps_expected=[0.01_rk,0.04_rk,-0.01_rk]
    call constitutive_plane_stress(m,d,status); sig_expected=matmul(d,eps_expected)
    call quad4_recover_plane(x,m,PLANE_MODE_STRESS,u,strain,stress,points,status)
    call assert_true(status%is_ok(),"plane recovery")
    do p=1,size(strain,2)
        call assert_close(maxval(abs(strain(:,p)-eps_expected)),0.0_rk,1.0e-12_rk,0.0_rk,"constant plane strain")
        call assert_close(maxval(abs(stress(:,p)-sig_expected)),0.0_rk,1.0e-6_rk,0.0_rk,"constant plane stress")
    end do
    write(*,'(A)') "PASS PATCH-V050-001 plane stress recovery"
end program patch_v050_001_plane_stress
