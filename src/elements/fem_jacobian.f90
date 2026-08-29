module fem_jacobian
    !! Isoparametric mapping, Jacobian ve physical shape-gradient donusumu.
    !!
    !! Sozlesme:
    !!   J(i,j) = dx_i / dxi_j
    !!   dNdx(i,a) = sum_j dN_dxi(j,a) * dxi_j/dx_i
    !!
    !! Square reference mappings (QUAD4 2B, HEX8 3B) icin inverse Jacobian
    !! hesaplanir. Embedded BAR2 icin ayri line metric yordamı kullanilir.
    use fem_kinds,       only : rk
    use fem_status,      only : status_t, FEM_STATUS_SIZE_MISMATCH, &
                                FEM_STATUS_INVALID_ARGUMENT, FEM_STATUS_NUMERICAL_FAILURE
    use fem_tolerances,  only : tolerance_set_t
    use fem_matrix_math, only : determinant_2x2, determinant_3x3
    implicit none
    private

    public :: compute_square_jacobian, map_shape_gradients
    public :: compute_embedded_line_metric

contains

    subroutine compute_square_jacobian(node_coordinates, dshape_dnatural, jacobian, &
                                       inverse_jacobian, determinant, status)
        real(rk), intent(in) :: node_coordinates(:, :)
        real(rk), intent(in) :: dshape_dnatural(:, :)
        real(rk), allocatable, intent(out) :: jacobian(:, :)
        real(rk), allocatable, intent(out) :: inverse_jacobian(:, :)
        real(rk), intent(out) :: determinant
        type(status_t), intent(out) :: status
        type(tolerance_set_t) :: tol
        integer :: dim
        real(rk) :: jacobian_scale, scaled_determinant

        call status%clear()
        determinant = 0.0_rk

        if (size(node_coordinates, 2) /= size(dshape_dnatural, 2)) then
            allocate(jacobian(0,0), inverse_jacobian(0,0))
            call status%set_error(FEM_STATUS_SIZE_MISMATCH, &
                "Jacobian icin coordinate ve shape node sayilari uyusmuyor.")
            return
        end if

        dim = size(dshape_dnatural, 1)
        if (size(node_coordinates, 1) /= dim .or. (dim /= 2 .and. dim /= 3)) then
            allocate(jacobian(0,0), inverse_jacobian(0,0))
            call status%set_error(FEM_STATUS_INVALID_ARGUMENT, &
                "Square Jacobian yalnizca 2B veya 3B square mapping icin kullanilir.")
            return
        end if

        allocate(jacobian(dim, dim), inverse_jacobian(dim, dim))
        jacobian = matmul(node_coordinates, transpose(dshape_dnatural))

        jacobian_scale = maxval(abs(jacobian))
        if (jacobian_scale <= tiny(1.0_rk)) then
            inverse_jacobian = 0.0_rk
            call status%set_error(FEM_STATUS_NUMERICAL_FAILURE, "Jacobian sifir; element dejenere.")
            return
        end if

        select case (dim)
        case (2)
            determinant = determinant_2x2(jacobian)
            scaled_determinant = abs(determinant) / (jacobian_scale**2)
            if (scaled_determinant <= tol%singular) then
                inverse_jacobian = 0.0_rk
                call status%set_error(FEM_STATUS_NUMERICAL_FAILURE, &
                    "2B Jacobian olcekten bagimsiz singularity kontrolunu gecemedi.")
                return
            end if
            inverse_jacobian(1,1) =  jacobian(2,2) / determinant
            inverse_jacobian(1,2) = -jacobian(1,2) / determinant
            inverse_jacobian(2,1) = -jacobian(2,1) / determinant
            inverse_jacobian(2,2) =  jacobian(1,1) / determinant

        case (3)
            determinant = determinant_3x3(jacobian)
            scaled_determinant = abs(determinant) / (jacobian_scale**3)
            if (scaled_determinant <= tol%singular) then
                inverse_jacobian = 0.0_rk
                call status%set_error(FEM_STATUS_NUMERICAL_FAILURE, &
                    "3B Jacobian olcekten bagimsiz singularity kontrolunu gecemedi.")
                return
            end if
            call inverse_3x3(jacobian, determinant, inverse_jacobian)
        end select
    end subroutine compute_square_jacobian

    subroutine map_shape_gradients(dshape_dnatural, inverse_jacobian, dshape_dphysical, status)
        real(rk), intent(in) :: dshape_dnatural(:, :)
        real(rk), intent(in) :: inverse_jacobian(:, :)
        real(rk), allocatable, intent(out) :: dshape_dphysical(:, :)
        type(status_t), intent(out) :: status
        integer :: dim

        call status%clear()
        dim = size(dshape_dnatural, 1)
        if (size(inverse_jacobian,1) /= dim .or. size(inverse_jacobian,2) /= dim) then
            allocate(dshape_dphysical(0,0))
            call status%set_error(FEM_STATUS_SIZE_MISMATCH, &
                "Inverse Jacobian boyutu shape gradient boyutuyla uyusmuyor.")
            return
        end if

        allocate(dshape_dphysical(dim, size(dshape_dnatural,2)))
        dshape_dphysical = matmul(transpose(inverse_jacobian), dshape_dnatural)
    end subroutine map_shape_gradients

    subroutine compute_embedded_line_metric(node_coordinates, dshape_dnatural, tangent, &
                                            jacobian_metric, direction, dshape_ds, status)
        real(rk), intent(in) :: node_coordinates(:, :)
        real(rk), intent(in) :: dshape_dnatural(:, :)
        real(rk), allocatable, intent(out) :: tangent(:)
        real(rk), intent(out) :: jacobian_metric
        real(rk), allocatable, intent(out) :: direction(:)
        real(rk), allocatable, intent(out) :: dshape_ds(:)
        type(status_t), intent(out) :: status
        call status%clear()
        jacobian_metric = 0.0_rk

        if (size(dshape_dnatural,1) /= 1 .or. &
            size(node_coordinates,2) /= size(dshape_dnatural,2)) then
            allocate(tangent(0), direction(0), dshape_ds(0))
            call status%set_error(FEM_STATUS_SIZE_MISMATCH, &
                "Embedded line mapping boyutlari uyusmuyor.")
            return
        end if

        allocate(tangent(size(node_coordinates,1)))
        allocate(direction(size(node_coordinates,1)))
        allocate(dshape_ds(size(dshape_dnatural,2)))

        tangent = matmul(node_coordinates, dshape_dnatural(1,:))
        jacobian_metric = sqrt(dot_product(tangent, tangent))
        if (jacobian_metric <= tiny(1.0_rk)) then
            direction = 0.0_rk
            dshape_ds = 0.0_rk
            call status%set_error(FEM_STATUS_NUMERICAL_FAILURE, &
                "BAR2 line Jacobian sifir; element dejenere.")
            return
        end if

        direction = tangent / jacobian_metric
        dshape_ds = dshape_dnatural(1,:) / jacobian_metric
    end subroutine compute_embedded_line_metric

    pure subroutine inverse_3x3(a, determinant, inverse)
        real(rk), intent(in) :: a(3,3), determinant
        real(rk), intent(out) :: inverse(3,3)

        inverse(1,1) =  (a(2,2)*a(3,3) - a(2,3)*a(3,2)) / determinant
        inverse(1,2) = -(a(1,2)*a(3,3) - a(1,3)*a(3,2)) / determinant
        inverse(1,3) =  (a(1,2)*a(2,3) - a(1,3)*a(2,2)) / determinant
        inverse(2,1) = -(a(2,1)*a(3,3) - a(2,3)*a(3,1)) / determinant
        inverse(2,2) =  (a(1,1)*a(3,3) - a(1,3)*a(3,1)) / determinant
        inverse(2,3) = -(a(1,1)*a(2,3) - a(1,3)*a(2,1)) / determinant
        inverse(3,1) =  (a(2,1)*a(3,2) - a(2,2)*a(3,1)) / determinant
        inverse(3,2) = -(a(1,1)*a(3,2) - a(1,2)*a(3,1)) / determinant
        inverse(3,3) =  (a(1,1)*a(2,2) - a(1,2)*a(2,1)) / determinant
    end subroutine inverse_3x3

end module fem_jacobian
