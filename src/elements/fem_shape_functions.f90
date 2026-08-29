module fem_shape_functions
    !! BAR2, QUAD4 ve HEX8 icin Lagrange shape function cekirdegi.
    !!
    !! dshape_dnatural(j,a) = dN_a / dxi_j sozlesmesi kullanilir.
    !! Bu matris duzeni Jacobian ve B-matrix kodunda proje genelinde ayni
    !! kalacaktir.
    use fem_kinds,              only : rk, id_kind
    use fem_topology,           only : TOPOLOGY_BAR2, TOPOLOGY_QUAD4, TOPOLOGY_HEX8
    use fem_reference_elements, only : reference_dimension, reference_node_count
    use fem_status,             only : status_t, FEM_STATUS_INVALID_ARGUMENT, FEM_STATUS_SIZE_MISMATCH
    implicit none
    private

    public :: evaluate_shape_functions

contains

    subroutine evaluate_shape_functions(topology_id, natural_coordinates, shape, dshape_dnatural, status)
        integer(id_kind), intent(in) :: topology_id
        real(rk), intent(in) :: natural_coordinates(:)
        real(rk), allocatable, intent(out) :: shape(:)
        real(rk), allocatable, intent(out) :: dshape_dnatural(:, :)
        type(status_t), intent(out) :: status
        integer :: dim, nn
        real(rk) :: xi, eta, zeta

        call status%clear()
        dim = reference_dimension(topology_id)
        nn = reference_node_count(topology_id)

        if (dim == 0 .or. nn == 0) then
            allocate(shape(0), dshape_dnatural(0, 0))
            call status%set_error(FEM_STATUS_INVALID_ARGUMENT, &
                "Shape function icin desteklenmeyen topology ID.")
            return
        end if

        if (size(natural_coordinates) /= dim) then
            allocate(shape(0), dshape_dnatural(0, 0))
            call status%set_error(FEM_STATUS_SIZE_MISMATCH, &
                "Natural-coordinate boyutu topology boyutuyla uyusmuyor.")
            return
        end if

        allocate(shape(nn), dshape_dnatural(dim, nn))

        select case (topology_id)
        case (TOPOLOGY_BAR2)
            xi = natural_coordinates(1)
            shape(1) = 0.5_rk * (1.0_rk - xi)
            shape(2) = 0.5_rk * (1.0_rk + xi)
            dshape_dnatural(1, :) = [-0.5_rk, 0.5_rk]

        case (TOPOLOGY_QUAD4)
            xi  = natural_coordinates(1)
            eta = natural_coordinates(2)

            shape(1) = 0.25_rk * (1.0_rk - xi) * (1.0_rk - eta)
            shape(2) = 0.25_rk * (1.0_rk + xi) * (1.0_rk - eta)
            shape(3) = 0.25_rk * (1.0_rk + xi) * (1.0_rk + eta)
            shape(4) = 0.25_rk * (1.0_rk - xi) * (1.0_rk + eta)

            dshape_dnatural(1, 1) = -0.25_rk * (1.0_rk - eta)
            dshape_dnatural(1, 2) =  0.25_rk * (1.0_rk - eta)
            dshape_dnatural(1, 3) =  0.25_rk * (1.0_rk + eta)
            dshape_dnatural(1, 4) = -0.25_rk * (1.0_rk + eta)

            dshape_dnatural(2, 1) = -0.25_rk * (1.0_rk - xi)
            dshape_dnatural(2, 2) = -0.25_rk * (1.0_rk + xi)
            dshape_dnatural(2, 3) =  0.25_rk * (1.0_rk + xi)
            dshape_dnatural(2, 4) =  0.25_rk * (1.0_rk - xi)

        case (TOPOLOGY_HEX8)
            xi   = natural_coordinates(1)
            eta  = natural_coordinates(2)
            zeta = natural_coordinates(3)

            call evaluate_hex8(xi, eta, zeta, shape, dshape_dnatural)
        end select
    end subroutine evaluate_shape_functions

    pure subroutine evaluate_hex8(xi, eta, zeta, shape, dshape)
        real(rk), intent(in) :: xi, eta, zeta
        real(rk), intent(out) :: shape(8)
        real(rk), intent(out) :: dshape(3, 8)
        real(rk), parameter :: sx(8) = [-1.0_rk, 1.0_rk, 1.0_rk, -1.0_rk, &
                                        -1.0_rk, 1.0_rk, 1.0_rk, -1.0_rk]
        real(rk), parameter :: se(8) = [-1.0_rk, -1.0_rk, 1.0_rk, 1.0_rk, &
                                        -1.0_rk, -1.0_rk, 1.0_rk, 1.0_rk]
        real(rk), parameter :: sz(8) = [-1.0_rk, -1.0_rk, -1.0_rk, -1.0_rk, &
                                         1.0_rk,  1.0_rk,  1.0_rk,  1.0_rk]
        integer :: a

        do a = 1, 8
            shape(a) = 0.125_rk * (1.0_rk + sx(a)*xi) * &
                                  (1.0_rk + se(a)*eta) * &
                                  (1.0_rk + sz(a)*zeta)
            dshape(1,a) = 0.125_rk * sx(a) * &
                          (1.0_rk + se(a)*eta) * (1.0_rk + sz(a)*zeta)
            dshape(2,a) = 0.125_rk * se(a) * &
                          (1.0_rk + sx(a)*xi) * (1.0_rk + sz(a)*zeta)
            dshape(3,a) = 0.125_rk * sz(a) * &
                          (1.0_rk + sx(a)*xi) * (1.0_rk + se(a)*eta)
        end do
    end subroutine evaluate_hex8

end module fem_shape_functions
