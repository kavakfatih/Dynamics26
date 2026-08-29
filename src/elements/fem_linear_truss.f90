module fem_linear_truss
    !! Kucuk-sekil-degistirme 3B iki dugumlu truss stiffness matrisi.
    !!
    !! k_e = (E A / L) * [ n n^T  -n n^T ; -n n^T  n n^T ]
    !!
    !! Bu yordam V0.4 assembled solver verification'i icin lineer element-local
    !! stiffness uretir. Genel material/section modelleme V0.5 kapsamindadir.
    use fem_kinds, only : rk
    use fem_status, only : status_t, FEM_STATUS_INVALID_ARGUMENT, FEM_STATUS_NUMERICAL_FAILURE
    implicit none
    private

    public :: truss2_stiffness_3d

contains

    subroutine truss2_stiffness_3d(x1, x2, young_modulus, area, stiffness, status)
        real(rk), intent(in) :: x1(3), x2(3)
        real(rk), intent(in) :: young_modulus, area
        real(rk), intent(out) :: stiffness(6,6)
        type(status_t), intent(out) :: status
        real(rk) :: direction(3), length, scale, projector(3,3)
        integer :: i, j

        call status%clear()
        stiffness = 0.0_rk
        if (young_modulus <= 0.0_rk .or. area <= 0.0_rk) then
            call status%set_error(FEM_STATUS_INVALID_ARGUMENT, "Truss E ve A pozitif olmali.")
            return
        end if
        direction = x2 - x1
        length = sqrt(dot_product(direction, direction))
        if (length <= sqrt(tiny(1.0_rk))) then
            call status%set_error(FEM_STATUS_NUMERICAL_FAILURE, "Truss uzunlugu sifira cok yakin.")
            return
        end if
        direction = direction / length
        scale = young_modulus * area / length
        do i = 1, 3
            do j = 1, 3
                projector(i,j) = direction(i) * direction(j)
            end do
        end do
        stiffness(1:3,1:3) = scale * projector
        stiffness(1:3,4:6) = -scale * projector
        stiffness(4:6,1:3) = -scale * projector
        stiffness(4:6,4:6) = scale * projector
    end subroutine truss2_stiffness_3d

end module fem_linear_truss
