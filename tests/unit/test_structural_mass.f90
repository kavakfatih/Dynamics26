program test_structural_mass
    use fem_kinds,only:rk
    use fem_structural_mass,only:MASS_CONSISTENT,MASS_LUMPED,truss2_mass_3d,quad4_mass_plane,hex8_mass
    use fem_status,only:status_t
    use test_support,only:assert_true,assert_close
    implicit none
    type(status_t)::status
    real(rk)::m6(6,6),m8(8,8),m24(24,24)
    real(rk)::q(2,4),h(3,8),mass
    call truss2_mass_3d([0.0_rk,0.0_rk,0.0_rk],[2.0_rk,0.0_rk,0.0_rk],1000.0_rk,0.01_rk,MASS_CONSISTENT,m6,status)
    call assert_true(status%is_ok(),"TRUSS2 consistent mass")
    mass=20.0_rk
    call assert_close(sum(m6([1,4],[1,4])),mass,1e-12_rk,1e-12_rk,"TRUSS x component total mass")
    call truss2_mass_3d([0.0_rk,0.0_rk,0.0_rk],[2.0_rk,0.0_rk,0.0_rk],1000.0_rk,0.01_rk,MASS_LUMPED,m6,status)
    call assert_true(status%is_ok(),"TRUSS2 lumped mass")
    call assert_close(m6(1,1),10.0_rk,1e-12_rk,1e-12_rk,"TRUSS lump node mass")
    call assert_close(sum(abs(m6-diagonal_only(m6))),0.0_rk,1e-12_rk,0.0_rk,"TRUSS lump diagonal")

    q=reshape([0.0_rk,0.0_rk, 2.0_rk,0.0_rk, 2.0_rk,1.0_rk, 0.0_rk,1.0_rk],[2,4])
    call quad4_mass_plane(q,10.0_rk,0.5_rk,MASS_CONSISTENT,m8,status)
    call assert_true(status%is_ok(),"QUAD4 consistent mass")
    call assert_close(sum(m8(1:8:2,1:8:2)),10.0_rk,1e-11_rk,1e-11_rk,"QUAD4 x translational total mass")
    call quad4_mass_plane(q,10.0_rk,0.5_rk,MASS_LUMPED,m8,status)
    call assert_true(status%is_ok(),"QUAD4 lumped mass")
    call assert_close(sum(abs(m8-diagonal_only(m8))),0.0_rk,1e-12_rk,0.0_rk,"QUAD4 lump diagonal")

    h(:,1)=[0.0_rk,0.0_rk,0.0_rk];h(:,2)=[1.0_rk,0.0_rk,0.0_rk]
    h(:,3)=[1.0_rk,1.0_rk,0.0_rk];h(:,4)=[0.0_rk,1.0_rk,0.0_rk]
    h(:,5)=[0.0_rk,0.0_rk,1.0_rk];h(:,6)=[1.0_rk,0.0_rk,1.0_rk]
    h(:,7)=[1.0_rk,1.0_rk,1.0_rk];h(:,8)=[0.0_rk,1.0_rk,1.0_rk]
    call hex8_mass(h,2.0_rk,MASS_CONSISTENT,m24,status)
    call assert_true(status%is_ok(),"HEX8 consistent mass")
    call assert_close(sum(m24(1:24:3,1:24:3)),2.0_rk,1e-11_rk,1e-11_rk,"HEX8 x translational total mass")
contains
    pure function diagonal_only(a) result(d)
        real(rk),intent(in)::a(:,:);real(rk)::d(size(a,1),size(a,2));integer::i
        d=0.0_rk;do i=1,min(size(a,1),size(a,2));d(i,i)=a(i,i);end do
    end function
end program
