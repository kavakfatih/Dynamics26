module fem_linear_beam
    !! XY duzleminde Euler-Bernoulli iki dugumlu frame/beam elemani.
    !! Global DOF sirasi: [u1,v1,rz1,u2,v2,rz2].
    use fem_kinds, only : rk
    use fem_status, only : status_t, FEM_STATUS_INVALID_ARGUMENT, FEM_STATUS_NUMERICAL_FAILURE
    implicit none
    private
    public :: beam2_frame_stiffness_2d
contains
    subroutine beam2_frame_stiffness_2d(x1,x2,young_modulus,area,iz,ke,status)
        real(rk), intent(in) :: x1(2),x2(2),young_modulus,area,iz
        real(rk), intent(out) :: ke(6,6)
        type(status_t), intent(out) :: status
        real(rk) :: dx,dy,l,c,s,ea,ei,k_local(6,6),t(6,6)
        call status%clear(); ke=0.0_rk
        if (young_modulus<=0.0_rk .or. area<=0.0_rk .or. iz<=0.0_rk) then
            call status%set_error(FEM_STATUS_INVALID_ARGUMENT,"Beam E, A ve Iz pozitif olmali."); return
        end if
        dx=x2(1)-x1(1); dy=x2(2)-x1(2); l=sqrt(dx*dx+dy*dy)
        if (l<=sqrt(tiny(1.0_rk))) then
            call status%set_error(FEM_STATUS_NUMERICAL_FAILURE,"Beam uzunlugu sifira cok yakin."); return
        end if
        c=dx/l; s=dy/l; ea=young_modulus*area; ei=young_modulus*iz
        k_local=0.0_rk
        k_local(1,1)=ea/l; k_local(1,4)=-ea/l; k_local(4,1)=-ea/l; k_local(4,4)=ea/l
        k_local(2,2)=12.0_rk*ei/l**3; k_local(2,3)=6.0_rk*ei/l**2
        k_local(2,5)=-12.0_rk*ei/l**3; k_local(2,6)=6.0_rk*ei/l**2
        k_local(3,2)=k_local(2,3); k_local(3,3)=4.0_rk*ei/l
        k_local(3,5)=-6.0_rk*ei/l**2; k_local(3,6)=2.0_rk*ei/l
        k_local(5,2)=k_local(2,5); k_local(5,3)=k_local(3,5)
        k_local(5,5)=12.0_rk*ei/l**3; k_local(5,6)=-6.0_rk*ei/l**2
        k_local(6,2)=k_local(2,6); k_local(6,3)=k_local(3,6)
        k_local(6,5)=k_local(5,6); k_local(6,6)=4.0_rk*ei/l
        t=0.0_rk
        t(1,1)=c; t(1,2)=s; t(2,1)=-s; t(2,2)=c; t(3,3)=1.0_rk
        t(4,4)=c; t(4,5)=s; t(5,4)=-s; t(5,5)=c; t(6,6)=1.0_rk
        ke=matmul(transpose(t),matmul(k_local,t))
    end subroutine beam2_frame_stiffness_2d
end module fem_linear_beam
