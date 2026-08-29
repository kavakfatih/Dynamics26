program ver_v070_005_finite_stretch
    use fem_kinds,only:rk,id_kind
    use fem_total_lagrangian_hex8,only:total_lagrangian_hex8_result_t,evaluate_total_lagrangian_hex8
    use fem_linear_elastic_material,only:linear_elastic_material_t
    use fem_status,only:status_t
    use test_support,only:assert_true,assert_close
    implicit none
    real(rk)::x(3,8),u(3,8),stretch,e11,s11,s22,sigma11,sigma22,lambda_lame,g
    type(linear_elastic_material_t)::mat
    type(total_lagrangian_hex8_result_t)::result
    type(status_t)::status
    integer::a
    call unit_cube(x)
    stretch=1.25_rk
    u=0._rk
    do a=1,8
        u(1,a)=(stretch-1._rk)*x(1,a)
    end do
    mat=linear_elastic_material_t(id=1_id_kind,name="finite stretch",young_modulus=6.e6_rk,poisson_ratio=0.29_rk)
    call evaluate_total_lagrangian_hex8(x,u,mat,result,status);call assert_true(status%is_ok(),"finite stretch evaluation")
    e11=0.5_rk*(stretch*stretch-1._rk);lambda_lame=mat%lame_lambda();g=mat%shear_modulus()
    s11=(lambda_lame+2._rk*g)*e11;s22=lambda_lame*e11
    sigma11=stretch*s11;sigma22=s22/stretch
    call assert_close(result%green_lagrange(1,1),e11,1.e-13_rk,1.e-13_rk,"finite stretch E11")
    call assert_close(result%second_pk(1,1),s11,1.e-7_rk,1.e-12_rk,"finite stretch S11")
    call assert_close(result%second_pk(2,1),s22,1.e-7_rk,1.e-12_rk,"finite stretch S22")
    call assert_close(result%cauchy(1,1),sigma11,1.e-7_rk,1.e-12_rk,"finite stretch Cauchy 11")
    call assert_close(result%cauchy(2,1),sigma22,1.e-7_rk,1.e-12_rk,"finite stretch Cauchy 22")
contains
    subroutine unit_cube(coords)
        real(rk),intent(out)::coords(3,8)
        coords(:,1)=[0._rk,0._rk,0._rk];coords(:,2)=[1._rk,0._rk,0._rk]
        coords(:,3)=[1._rk,1._rk,0._rk];coords(:,4)=[0._rk,1._rk,0._rk]
        coords(:,5)=[0._rk,0._rk,1._rk];coords(:,6)=[1._rk,0._rk,1._rk]
        coords(:,7)=[1._rk,1._rk,1._rk];coords(:,8)=[0._rk,1._rk,1._rk]
    end subroutine unit_cube
end program ver_v070_005_finite_stretch
