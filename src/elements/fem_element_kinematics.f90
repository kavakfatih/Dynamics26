module fem_element_kinematics
    !! Small-strain element B-matrix yardimcilari.
    !!
    !! Bu surumde amac constitutive model veya stiffness kurmak degil; shape
    !! gradient'lerinin affine displacement field'larini dogru strain'e
    !! donusturdugunu patch test ile dogrulamaktir.
    use fem_kinds,  only : rk
    use fem_status, only : status_t, FEM_STATUS_INVALID_ARGUMENT, FEM_STATUS_SIZE_MISMATCH
    implicit none
    private

    public :: build_plane_b_matrix, build_solid_b_matrix, build_axisymmetric_b_matrix

contains

    subroutine build_plane_b_matrix(dshape_dx, b, status)
        real(rk), intent(in) :: dshape_dx(:, :)
        real(rk), allocatable, intent(out) :: b(:, :)
        type(status_t), intent(out) :: status
        integer :: a, nn, col

        call status%clear()
        if (size(dshape_dx,1) /= 2) then
            allocate(b(0,0))
            call status%set_error(FEM_STATUS_SIZE_MISMATCH, "Plane B matrix 2B physical gradient gerektirir.")
            return
        end if

        nn = size(dshape_dx,2)
        allocate(b(3, 2*nn))
        b = 0.0_rk
        do a = 1, nn
            col = 2*(a-1) + 1
            b(1,col)   = dshape_dx(1,a)
            b(2,col+1) = dshape_dx(2,a)
            b(3,col)   = dshape_dx(2,a)
            b(3,col+1) = dshape_dx(1,a)
        end do
    end subroutine build_plane_b_matrix

    subroutine build_solid_b_matrix(dshape_dx, b, status)
        real(rk), intent(in) :: dshape_dx(:, :)
        real(rk), allocatable, intent(out) :: b(:, :)
        type(status_t), intent(out) :: status
        integer :: a, nn, col

        call status%clear()
        if (size(dshape_dx,1) /= 3) then
            allocate(b(0,0))
            call status%set_error(FEM_STATUS_SIZE_MISMATCH, "Solid B matrix 3B physical gradient gerektirir.")
            return
        end if

        nn = size(dshape_dx,2)
        allocate(b(6, 3*nn))
        b = 0.0_rk
        do a = 1, nn
            col = 3*(a-1) + 1
            b(1,col)   = dshape_dx(1,a)
            b(2,col+1) = dshape_dx(2,a)
            b(3,col+2) = dshape_dx(3,a)
            b(4,col)   = dshape_dx(2,a)
            b(4,col+1) = dshape_dx(1,a)
            b(5,col+1) = dshape_dx(3,a)
            b(5,col+2) = dshape_dx(2,a)
            b(6,col)   = dshape_dx(3,a)
            b(6,col+2) = dshape_dx(1,a)
        end do
    end subroutine build_solid_b_matrix

    subroutine build_axisymmetric_b_matrix(shape, dshape_dr_z, radius, b, status)
        !! Axisymmetric strain sirasi: [eps_rr, eps_zz, gamma_rz, eps_tt].
        !! u_r ve u_z nodal DOF sirasi kullanilir.
        real(rk), intent(in) :: shape(:)
        real(rk), intent(in) :: dshape_dr_z(:, :)
        real(rk), intent(in) :: radius
        real(rk), allocatable, intent(out) :: b(:, :)
        type(status_t), intent(out) :: status
        integer :: a, nn, col

        call status%clear()
        if (size(dshape_dr_z,1) /= 2 .or. size(dshape_dr_z,2) /= size(shape)) then
            allocate(b(0,0))
            call status%set_error(FEM_STATUS_SIZE_MISMATCH, "Axisymmetric B matrix shape/gradient boyutlari uyusmuyor.")
            return
        end if
        if (radius <= 0.0_rk) then
            allocate(b(0,0))
            call status%set_error(FEM_STATUS_INVALID_ARGUMENT, &
                "Axisymmetric hoop strain icin integration radius sifirdan buyuk olmali.")
            return
        end if

        nn = size(shape)
        allocate(b(4, 2*nn))
        b = 0.0_rk
        do a = 1, nn
            col = 2*(a-1) + 1
            b(1,col)   = dshape_dr_z(1,a)
            b(2,col+1) = dshape_dr_z(2,a)
            b(3,col)   = dshape_dr_z(2,a)
            b(3,col+1) = dshape_dr_z(1,a)
            b(4,col)   = shape(a) / radius
        end do
    end subroutine build_axisymmetric_b_matrix

end module fem_element_kinematics
