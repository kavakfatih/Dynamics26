program patch_v070_001_hex8_affine_finite_strain
    use fem_kinds, only : rk, id_kind
    use fem_total_lagrangian_hex8, only : total_lagrangian_hex8_result_t, evaluate_total_lagrangian_hex8
    use fem_linear_elastic_material, only : linear_elastic_material_t
    use fem_finite_strain_kinematics, only : green_lagrange_strain
    use fem_tensor_notation, only : strain_tensor_to_voigt
    use fem_status, only : status_t
    use test_support, only : assert_true, assert_close
    implicit none
    real(rk) :: x(3,8), u(3,8), h(3,3), f(3,3), e(3,3), ev(6)
    type(linear_elastic_material_t) :: mat
    type(total_lagrangian_hex8_result_t) :: result
    type(status_t) :: status
    integer :: a, p, i

    call unit_cube(x)
    h = reshape([0.10_rk,-0.02_rk,0.01_rk, &
                 0.03_rk, 0.05_rk,0.02_rk, &
                 0.00_rk, 0.01_rk,-0.03_rk], [3,3])
    do a=1,8
        u(:,a)=matmul(h,x(:,a))
    end do
    f=0.0_rk
    do i=1,3
        f(i,i)=1.0_rk
    end do
    f=f+h
    call green_lagrange_strain(f,e)
    call strain_tensor_to_voigt(e,ev)
    mat=linear_elastic_material_t(id=1_id_kind,name="patch",young_modulus=10.e6_rk,poisson_ratio=0.3_rk)
    call evaluate_total_lagrangian_hex8(x,u,mat,result,status)
    call assert_true(status%is_ok(), "hex8 affine finite-strain evaluation")
    do p=1,size(result%j)
        call assert_close(result%green_lagrange(1,p),ev(1),1.e-12_rk,1.e-12_rk,"constant E11")
        call assert_close(result%green_lagrange(4,p),ev(4),1.e-12_rk,1.e-12_rk,"constant E12 engineering")
        call assert_close(result%deformation_gradient(1,1,p),f(1,1),1.e-12_rk,1.e-12_rk,"constant F11")
    end do
    call assert_true(minval(result%j)>0.0_rk,"positive J")
contains
    subroutine unit_cube(coords)
        real(rk),intent(out)::coords(3,8)
        coords(:,1)=[0._rk,0._rk,0._rk]; coords(:,2)=[1._rk,0._rk,0._rk]
        coords(:,3)=[1._rk,1._rk,0._rk]; coords(:,4)=[0._rk,1._rk,0._rk]
        coords(:,5)=[0._rk,0._rk,1._rk]; coords(:,6)=[1._rk,0._rk,1._rk]
        coords(:,7)=[1._rk,1._rk,1._rk]; coords(:,8)=[0._rk,1._rk,1._rk]
    end subroutine unit_cube
end program patch_v070_001_hex8_affine_finite_strain
