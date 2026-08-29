module fem_matrix_math
    !! V0.1.0 icin kucuk matris yardimcilari.
    !! Buyuk global lineer sistem cozumleri burada uygulanmaz; onlar V0.4.0'da
    !! sparse backend arayuzu uzerinden gelecektir.
    use fem_kinds, only : rk
    use fem_status, only : status_t, FEM_STATUS_SIZE_MISMATCH
    implicit none
    private

    public :: matrix_vector_product, determinant_2x2, determinant_3x3
    public :: identity_matrix_3x3

contains

    subroutine matrix_vector_product(a, x, y, status)
        real(rk), intent(in) :: a(:, :), x(:)
        real(rk), allocatable, intent(out) :: y(:)
        type(status_t), intent(out) :: status

        call status%clear()
        if (size(a, 2) /= size(x)) then
            allocate(y(0))
            call status%set_error(FEM_STATUS_SIZE_MISMATCH, &
                "Matris sutun sayisi ile vector boyutu uyusmuyor.")
            return
        end if
        y = matmul(a, x)
    end subroutine matrix_vector_product

    pure real(rk) function determinant_2x2(a) result(det)
        real(rk), intent(in) :: a(2, 2)
        det = a(1,1) * a(2,2) - a(1,2) * a(2,1)
    end function determinant_2x2

    pure real(rk) function determinant_3x3(a) result(det)
        real(rk), intent(in) :: a(3, 3)
        det = a(1,1) * (a(2,2)*a(3,3) - a(2,3)*a(3,2)) &
            - a(1,2) * (a(2,1)*a(3,3) - a(2,3)*a(3,1)) &
            + a(1,3) * (a(2,1)*a(3,2) - a(2,2)*a(3,1))
    end function determinant_3x3

    pure function identity_matrix_3x3() result(identity)
        real(rk) :: identity(3, 3)
        integer :: i

        identity = 0.0_rk
        do i = 1, 3
            identity(i, i) = 1.0_rk
        end do
    end function identity_matrix_3x3

end module fem_matrix_math
