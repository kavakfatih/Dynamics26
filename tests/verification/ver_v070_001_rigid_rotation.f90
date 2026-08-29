program ver_v070_001_rigid_rotation
    use fem_kinds, only : rk,id_kind
    use fem_total_lagrangian_hex8, only : total_lagrangian_hex8_result_t,evaluate_total_lagrangian_hex8
    use fem_linear_elastic_material, only : linear_elastic_material_t
    use fem_status, only : status_t
    use test_support, only : assert_true,assert_close
    implicit none
    real(rk) :: x(3,8),u(3,8),q(3,3),theta
    type(linear_elastic_material_t) :: mat
    type(total_lagrangian_hex8_result_t) :: result
    type(status_t) :: status
    integer :: a

    call unit_cube(x)
    theta=0.63_rk
    q=0.0_rk
    q(1,1)=cos(theta);q(1,2)=-sin(theta)
    q(2,1)=sin(theta);q(2,2)=cos(theta)
    q(3,3)=1._rk
    do a=1,8
        u(:,a)=matmul(q,x(:,a))-x(:,a)
    end do
    mat=linear_elastic_material_t(id=1_id_kind,name="objectivity",young_modulus=2.e6_rk,poisson_ratio=0.3_rk)
    call evaluate_total_lagrangian_hex8(x,u,mat,result,status)
    call assert_true(status%is_ok(),"rigid rotation evaluation")
    call assert_true(maxval(abs(result%green_lagrange))<2.e-14_rk,"rigid rotation E zero")
    call assert_true(maxval(abs(result%second_pk))<1.e-7_rk,"rigid rotation S zero")
    call assert_true(maxval(abs(result%internal_force))<1.e-7_rk,"rigid rotation internal force zero")
    call assert_close(minval(result%j),1.0_rk,2.e-14_rk,2.e-14_rk,"rigid rotation J")
    call assert_close(result%strain_energy,0.0_rk,1.e-8_rk,1.e-8_rk,"rigid rotation strain energy")
contains
    subroutine unit_cube(coords)
        real(rk),intent(out)::coords(3,8)
        coords(:,1)=[0._rk,0._rk,0._rk]; coords(:,2)=[1._rk,0._rk,0._rk]
        coords(:,3)=[1._rk,1._rk,0._rk]; coords(:,4)=[0._rk,1._rk,0._rk]
        coords(:,5)=[0._rk,0._rk,1._rk]; coords(:,6)=[1._rk,0._rk,1._rk]
        coords(:,7)=[1._rk,1._rk,1._rk]; coords(:,8)=[0._rk,1._rk,1._rk]
    end subroutine unit_cube
end program ver_v070_001_rigid_rotation
