program patch_v050_003_hex8
    use fem_kinds, only : rk,id_kind
    use fem_linear_elastic_material, only : linear_elastic_material_t
    use fem_linear_continuum, only : hex8_recover
    use fem_status, only : status_t
    use test_support, only : assert_true,assert_close
    implicit none
    type(linear_elastic_material_t) :: m
    type(status_t) :: status
    real(rk) :: x(3,8),u(24),expected(6)
    real(rk), allocatable :: strain(:,:),stress(:,:),points(:,:)
    integer :: a,p
    m=linear_elastic_material_t(id=1_id_kind,name="mat",young_modulus=8.0e6_rk,poisson_ratio=0.2_rk)
    x=reshape([0.0_rk,0.0_rk,0.0_rk, 1.0_rk,0.0_rk,0.0_rk, 1.0_rk,1.0_rk,0.0_rk, 0.0_rk,1.0_rk,0.0_rk, &
               0.0_rk,0.0_rk,1.0_rk, 1.0_rk,0.0_rk,1.0_rk, 1.0_rk,1.0_rk,1.0_rk, 0.0_rk,1.0_rk,1.0_rk],[3,8])
    do a=1,8
        u(3*a-2)=0.01_rk*x(1,a)+0.02_rk*x(2,a)
        u(3*a-1)=0.03_rk*x(2,a)+0.04_rk*x(3,a)
        u(3*a)=0.05_rk*x(3,a)+0.06_rk*x(1,a)
    end do
    expected=[0.01_rk,0.03_rk,0.05_rk,0.02_rk,0.04_rk,0.06_rk]
    call hex8_recover(x,m,u,strain,stress,points,status)
    call assert_true(status%is_ok(),"hex recovery")
    do p=1,size(strain,2)
        call assert_close(maxval(abs(strain(:,p)-expected)),0.0_rk,1.0e-12_rk,0.0_rk,"hex constant strain")
    end do
    write(*,'(A)') "PASS PATCH-V050-003 HEX8 recovery"
end program patch_v050_003_hex8
