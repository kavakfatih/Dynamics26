program ver_v090_004_hyperelastic_hex8_tangent
    use fem_kinds, only : rk,id_kind
    use fem_total_lagrangian_hex8, only : total_lagrangian_hex8_result_t,evaluate_total_lagrangian_hex8
    use fem_hyperelastic_material, only : hyperelastic_material_t,HYPER_MOONEY_RIVLIN
    use fem_status, only : status_t
    use test_support, only : assert_true
    implicit none
    real(rk) :: x(3,8),u(3,8),hmat(3,3),up(3,8),um(3,8),fd(24,24),err,scale,h
    type(hyperelastic_material_t) :: mat
    type(total_lagrangian_hex8_result_t) :: result,plus_result,minus_result
    type(status_t) :: status
    integer :: a,j,node,comp

    call unit_cube(x)
    hmat=reshape([0.06_rk,-0.01_rk,0.004_rk, &
                  0.018_rk,0.03_rk,0.009_rk, &
                  0.007_rk,-0.005_rk,-0.015_rk],[3,3])
    do a=1,8;u(:,a)=matmul(hmat,x(:,a));end do
    mat=hyperelastic_material_t(id=1_id_kind,name='MR HEX8',model=HYPER_MOONEY_RIVLIN, &
        bulk_modulus=25.e6_rk,c10=0.8e6_rk,c01=0.35e6_rk)
    call evaluate_total_lagrangian_hex8(x,u,mat,result,status)
    call assert_true(status%is_ok(),'hyperelastic base tangent evaluation')

    h=1.e-7_rk
    do j=1,24
        node=(j-1)/3+1;comp=mod(j-1,3)+1;up=u;um=u
        up(comp,node)=up(comp,node)+h;um(comp,node)=um(comp,node)-h
        call evaluate_total_lagrangian_hex8(x,up,mat,plus_result,status);call assert_true(status%is_ok(),'hyper plus')
        call evaluate_total_lagrangian_hex8(x,um,mat,minus_result,status);call assert_true(status%is_ok(),'hyper minus')
        fd(:,j)=(plus_result%internal_force-minus_result%internal_force)/(2._rk*h)
    end do
    scale=max(1._rk,sqrt(sum(fd*fd)));err=sqrt(sum((result%tangent-fd)**2))/scale
    if(err>2.e-6_rk)write(*,'(A,ES12.4)')'hyper HEX8 tangent rel err=',err
    call assert_true(err<2.e-6_rk,'Hyperelastic HEX8 tangent FD Jacobian')
    call assert_true(maxval(abs(result%tangent-transpose(result%tangent))) < &
        1.e-9_rk*max(1._rk,maxval(abs(result%tangent))),'Hyperelastic element tangent symmetric')
contains
    subroutine unit_cube(coords)
        real(rk),intent(out)::coords(3,8)
        coords(:,1)=[0._rk,0._rk,0._rk];coords(:,2)=[1._rk,0._rk,0._rk]
        coords(:,3)=[1._rk,1._rk,0._rk];coords(:,4)=[0._rk,1._rk,0._rk]
        coords(:,5)=[0._rk,0._rk,1._rk];coords(:,6)=[1._rk,0._rk,1._rk]
        coords(:,7)=[1._rk,1._rk,1._rk];coords(:,8)=[0._rk,1._rk,1._rk]
    end subroutine unit_cube
end program ver_v090_004_hyperelastic_hex8_tangent
