program patch_v050_002_axisymmetric
    use fem_kinds, only : rk,id_kind
    use fem_linear_elastic_material, only : linear_elastic_material_t
    use fem_linear_continuum, only : quad4_recover_axisymmetric
    use fem_status, only : status_t
    use test_support, only : assert_true,assert_close
    implicit none
    type(linear_elastic_material_t) :: m
    type(status_t) :: status
    real(rk) :: x(2,4),u(8),expected(4)
    real(rk), allocatable :: strain(:,:),stress(:,:),points(:,:)
    integer :: a,p
    m=linear_elastic_material_t(id=1_id_kind,name="mat",young_modulus=5.0e6_rk,poisson_ratio=0.3_rk)
    x=reshape([1.0_rk,0.0_rk, 2.0_rk,0.0_rk, 2.0_rk,1.0_rk, 1.0_rk,1.0_rk],[2,4])
    do a=1,4
        u(2*a-1)=0.02_rk*x(1,a)
        u(2*a)=0.03_rk*x(2,a)
    end do
    expected=[0.02_rk,0.03_rk,0.0_rk,0.02_rk]
    call quad4_recover_axisymmetric(x,m,u,strain,stress,points,status)
    call assert_true(status%is_ok(),"axisym recovery")
    do p=1,size(strain,2)
        call assert_close(maxval(abs(strain(:,p)-expected)),0.0_rk,1.0e-12_rk,0.0_rk,"axisym constant strain")
    end do
    write(*,'(A)') "PASS PATCH-V050-002 axisymmetric recovery"
end program patch_v050_002_axisymmetric
