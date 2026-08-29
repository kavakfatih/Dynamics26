module fem_linear_results
    !! Lineer sonuclardan scalar equivalent stress uretimi.
    use fem_kinds, only : rk
    implicit none
    private
    public :: von_mises_3d, von_mises_plane, von_mises_plane_strain, von_mises_axisymmetric
contains
    pure real(rk) function von_mises_3d(stress) result(vm)
        real(rk), intent(in) :: stress(6)
        vm=sqrt(0.5_rk*((stress(1)-stress(2))**2+(stress(2)-stress(3))**2+ &
           (stress(3)-stress(1))**2)+3.0_rk*(stress(4)**2+stress(5)**2+stress(6)**2))
    end function von_mises_3d
    pure real(rk) function von_mises_plane(stress) result(vm)
        real(rk), intent(in) :: stress(3)
        vm=sqrt(stress(1)**2-stress(1)*stress(2)+stress(2)**2+3.0_rk*stress(3)**2)
    end function von_mises_plane

    pure real(rk) function von_mises_plane_strain(stress) result(vm)
        !! Plane-strain stress sirasi [xx,yy,tau_xy,zz].
        !! eps_zz=0 olmasi sigma_zz=0 anlamina gelmez; bu nedenle klasik
        !! plane-stress von Mises ifadesi burada kullanilamaz.
        real(rk), intent(in) :: stress(4)
        vm=sqrt(0.5_rk*((stress(1)-stress(2))**2+(stress(2)-stress(4))**2+ &
           (stress(4)-stress(1))**2)+3.0_rk*stress(3)**2)
    end function von_mises_plane_strain

    pure real(rk) function von_mises_axisymmetric(stress) result(vm)
        !! stress sirasi [rr,zz,tau_rz,tt].
        real(rk), intent(in) :: stress(4)
        vm=sqrt(0.5_rk*((stress(1)-stress(2))**2+(stress(2)-stress(4))**2+ &
           (stress(4)-stress(1))**2)+3.0_rk*stress(3)**2)
    end function von_mises_axisymmetric
end module fem_linear_results
