program ver_v070_004_superposed_rotation
    use fem_kinds,only:rk,id_kind
    use fem_total_lagrangian_hex8,only:total_lagrangian_hex8_result_t,evaluate_total_lagrangian_hex8
    use fem_linear_elastic_material,only:linear_elastic_material_t
    use fem_status,only:status_t
    use test_support,only:assert_true
    implicit none
    real(rk)::x(3,8),u1(3,8),u2(3,8),f0(3,3),q(3,3),theta,current1(3,8),current2(3,8),expected(3,3),s1(3,3),s2(3,3)
    type(linear_elastic_material_t)::mat
    type(total_lagrangian_hex8_result_t)::r1,r2
    type(status_t)::status
    integer::p
    call unit_cube(x)
    f0=0._rk;f0(1,1)=1.15_rk;f0(2,2)=0.92_rk;f0(3,3)=1.04_rk;f0(1,2)=0.08_rk
    current1=matmul(f0,x);u1=current1-x
    theta=0.47_rk;q=0._rk;q(1,1)=cos(theta);q(1,2)=-sin(theta);q(2,1)=sin(theta);q(2,2)=cos(theta);q(3,3)=1._rk
    current2=matmul(q,current1);u2=current2-x
    mat=linear_elastic_material_t(id=1_id_kind,name="superposed rotation",young_modulus=4.e6_rk,poisson_ratio=0.31_rk)
    call evaluate_total_lagrangian_hex8(x,u1,mat,r1,status);call assert_true(status%is_ok(),"base deformation")
    call evaluate_total_lagrangian_hex8(x,u2,mat,r2,status);call assert_true(status%is_ok(),"rotated deformation")
    call assert_true(maxval(abs(r1%green_lagrange-r2%green_lagrange))<2.e-13_rk,"E invariant under superposed rotation")
    call assert_true(maxval(abs(r1%second_pk-r2%second_pk))<1.e-6_rk,"S invariant under superposed rotation")
    do p=1,size(r1%j)
        call voigt_stress_to_tensor(r1%cauchy(:,p),s1)
        call voigt_stress_to_tensor(r2%cauchy(:,p),s2)
        expected=matmul(q,matmul(s1,transpose(q)))
        call assert_true(maxval(abs(s2-expected))<1.e-7_rk*max(1._rk,maxval(abs(expected))),"Cauchy stress rotates objectively")
    end do
contains
    subroutine unit_cube(coords)
        real(rk),intent(out)::coords(3,8)
        coords(:,1)=[0._rk,0._rk,0._rk];coords(:,2)=[1._rk,0._rk,0._rk]
        coords(:,3)=[1._rk,1._rk,0._rk];coords(:,4)=[0._rk,1._rk,0._rk]
        coords(:,5)=[0._rk,0._rk,1._rk];coords(:,6)=[1._rk,0._rk,1._rk]
        coords(:,7)=[1._rk,1._rk,1._rk];coords(:,8)=[0._rk,1._rk,1._rk]
    end subroutine unit_cube
    subroutine voigt_stress_to_tensor(v,t)
        real(rk),intent(in)::v(6);real(rk),intent(out)::t(3,3)
        t=0._rk;t(1,1)=v(1);t(2,2)=v(2);t(3,3)=v(3)
        t(1,2)=v(4);t(2,1)=v(4);t(2,3)=v(5);t(3,2)=v(5);t(1,3)=v(6);t(3,1)=v(6)
    end subroutine voigt_stress_to_tensor
end program ver_v070_004_superposed_rotation
