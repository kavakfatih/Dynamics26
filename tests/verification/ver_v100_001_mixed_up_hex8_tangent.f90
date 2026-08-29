program ver_v100_001_mixed_up_hex8_tangent
    use fem_kinds, only : rk,id_kind
    use fem_mixed_up_hex8, only : mixed_up_hex8_result_t,evaluate_mixed_up_hex8
    use fem_hyperelastic_material, only : hyperelastic_material_t,HYPER_NEO_HOOKEAN
    use fem_status, only : status_t
    use test_support, only : assert_true,assert_close
    implicit none
    real(rk) :: x(3,8),u(3,8),hmat(3,3),up(3,8),um(3,8),pressure,h_u,h_p
    real(rk) :: fd_kuu(24,24),fd_kpu(24),fd_kup(24),fd_kpp,err,scale
    type(hyperelastic_material_t) :: mat
    type(mixed_up_hex8_result_t) :: base,plus_result,minus_result
    type(status_t) :: status
    integer :: a,j,node,comp

    call unit_cube(x)
    hmat=reshape([0.035_rk,-0.012_rk,0.004_rk, &
                  0.009_rk,0.018_rk,0.006_rk, &
                  0.003_rk,-0.005_rk,-0.014_rk],[3,3])
    do a=1,8;u(:,a)=matmul(hmat,x(:,a));end do
    pressure=0.23e6_rk
    mat=hyperelastic_material_t(id=10_id_kind,name='Mixed NH',model=HYPER_NEO_HOOKEAN, &
        bulk_modulus=2.0e9_rk,c10=0.85e6_rk)

    call evaluate_mixed_up_hex8(x,u,pressure,mat,base,status)
    call assert_true(status%is_ok(),'mixed base evaluation')
    call assert_true(base%minimum_j>0.0_rk,'mixed J positive')

    h_u=1.e-7_rk
    do j=1,24
        node=(j-1)/3+1;comp=mod(j-1,3)+1;up=u;um=u
        up(comp,node)=up(comp,node)+h_u;um(comp,node)=um(comp,node)-h_u
        call evaluate_mixed_up_hex8(x,up,pressure,mat,plus_result,status);call assert_true(status%is_ok(),'mixed plus u')
        call evaluate_mixed_up_hex8(x,um,pressure,mat,minus_result,status);call assert_true(status%is_ok(),'mixed minus u')
        fd_kuu(:,j)=(plus_result%internal_force(1:24)-minus_result%internal_force(1:24))/(2._rk*h_u)
        fd_kpu(j)=(plus_result%internal_force(25)-minus_result%internal_force(25))/(2._rk*h_u)
    end do
    scale=max(1._rk,sqrt(sum(fd_kuu*fd_kuu)));err=sqrt(sum((base%kuu-fd_kuu)**2))/scale
    if(err>5.e-6_rk)write(*,'(A,ES12.4)')'mixed Kuu tangent rel err=',err
    call assert_true(err<5.e-6_rk,'mixed Kuu finite-difference tangent')
    scale=max(1._rk,sqrt(sum(fd_kpu*fd_kpu)));err=sqrt(sum((base%kpu(1,:)-fd_kpu)**2))/scale
    call assert_true(err<2.e-7_rk,'mixed Kpu finite-difference tangent')

    h_p=10.0_rk
    call evaluate_mixed_up_hex8(x,u,pressure+h_p,mat,plus_result,status);call assert_true(status%is_ok(),'mixed plus p')
    call evaluate_mixed_up_hex8(x,u,pressure-h_p,mat,minus_result,status);call assert_true(status%is_ok(),'mixed minus p')
    fd_kup=(plus_result%internal_force(1:24)-minus_result%internal_force(1:24))/(2._rk*h_p)
    fd_kpp=(plus_result%internal_force(25)-minus_result%internal_force(25))/(2._rk*h_p)
    scale=max(1._rk,sqrt(sum(fd_kup*fd_kup)));err=sqrt(sum((base%kup(:,1)-fd_kup)**2))/scale
    call assert_true(err<2.e-8_rk,'mixed Kup finite-difference tangent')
    call assert_close(base%kpp(1,1),fd_kpp,1.e-13_rk,2.e-8_rk,'mixed Kpp finite-difference tangent')

    scale=max(1._rk,maxval(abs(base%kup)))
    call assert_true(maxval(abs(base%kup(:,1)-base%kpu(1,:)))<1.e-10_rk*scale,'mixed Kup/Kpu variational symmetry')
    call assert_true(maxval(abs(base%tangent-transpose(base%tangent)))<1.e-10_rk*max(1._rk,maxval(abs(base%tangent))), &
                     'mixed full tangent symmetric')

    write(*,'(A)')'PASS VER-V100-001 mixed HEX8/P0 consistent tangent'
contains
    subroutine unit_cube(coords)
        real(rk),intent(out)::coords(3,8)
        coords(:,1)=[0._rk,0._rk,0._rk];coords(:,2)=[1._rk,0._rk,0._rk]
        coords(:,3)=[1._rk,1._rk,0._rk];coords(:,4)=[0._rk,1._rk,0._rk]
        coords(:,5)=[0._rk,0._rk,1._rk];coords(:,6)=[1._rk,0._rk,1._rk]
        coords(:,7)=[1._rk,1._rk,1._rk];coords(:,8)=[0._rk,1._rk,1._rk]
    end subroutine unit_cube
end program ver_v100_001_mixed_up_hex8_tangent
