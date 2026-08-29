module fem_quadrature
    !! Gauss-Legendre quadrature ve tensor-product integration kurallari.
    !!
    !! V0.3.0 icin 1 ve 2 nokta/yon desteklenir. BAR2, QUAD4 ve HEX8'in
    !! standart full-integration kurali 2 nokta/yon olarak secilir.
    use fem_kinds,              only : rk, id_kind
    use fem_topology,           only : TOPOLOGY_BAR2, TOPOLOGY_QUAD4, TOPOLOGY_HEX8
    use fem_reference_elements, only : reference_dimension
    use fem_status,             only : status_t, FEM_STATUS_INVALID_ARGUMENT
    implicit none
    private

    type, public :: quadrature_rule_t
        integer :: dimension = 0
        integer :: order_per_axis = 0
        integer :: point_count = 0
        real(rk), allocatable :: points(:, :)
        real(rk), allocatable :: weights(:)
    contains
        procedure :: clear => quadrature_rule_clear
    end type quadrature_rule_t

    public :: create_gauss_rule, standard_quadrature_rule

contains

    subroutine quadrature_rule_clear(this)
        class(quadrature_rule_t), intent(inout) :: this
        if (allocated(this%points)) deallocate(this%points)
        if (allocated(this%weights)) deallocate(this%weights)
        this%dimension = 0
        this%order_per_axis = 0
        this%point_count = 0
    end subroutine quadrature_rule_clear

    subroutine create_gauss_rule(dimension, order_per_axis, rule, status)
        integer, intent(in) :: dimension, order_per_axis
        type(quadrature_rule_t), intent(out) :: rule
        type(status_t), intent(out) :: status
        real(rk) :: x1d(2), w1d(2)
        integer :: n1d, i, j, k, p

        call status%clear()
        call rule%clear()

        if (dimension < 1 .or. dimension > 3) then
            call status%set_error(FEM_STATUS_INVALID_ARGUMENT, &
                "Gauss quadrature boyutu 1, 2 veya 3 olmali.")
            return
        end if

        select case (order_per_axis)
        case (1)
            n1d = 1
            x1d = 0.0_rk
            w1d = 0.0_rk
            x1d(1) = 0.0_rk
            w1d(1) = 2.0_rk
        case (2)
            n1d = 2
            x1d(1) = -1.0_rk / sqrt(3.0_rk)
            x1d(2) =  1.0_rk / sqrt(3.0_rk)
            w1d(1:2) = 1.0_rk
        case default
            call status%set_error(FEM_STATUS_INVALID_ARGUMENT, &
                "Gauss quadrature bu element ailesinde yalnizca 1 veya 2 nokta/yon destekler.")
            return
        end select

        rule%dimension = dimension
        rule%order_per_axis = order_per_axis
        rule%point_count = n1d ** dimension
        allocate(rule%points(dimension, rule%point_count))
        allocate(rule%weights(rule%point_count))

        p = 0
        select case (dimension)
        case (1)
            do i = 1, n1d
                p = p + 1
                rule%points(1, p) = x1d(i)
                rule%weights(p) = w1d(i)
            end do
        case (2)
            do j = 1, n1d
                do i = 1, n1d
                    p = p + 1
                    rule%points(:, p) = [x1d(i), x1d(j)]
                    rule%weights(p) = w1d(i) * w1d(j)
                end do
            end do
        case (3)
            do k = 1, n1d
                do j = 1, n1d
                    do i = 1, n1d
                        p = p + 1
                        rule%points(:, p) = [x1d(i), x1d(j), x1d(k)]
                        rule%weights(p) = w1d(i) * w1d(j) * w1d(k)
                    end do
                end do
            end do
        end select
    end subroutine create_gauss_rule

    subroutine standard_quadrature_rule(topology_id, rule, status)
        integer(id_kind), intent(in) :: topology_id
        type(quadrature_rule_t), intent(out) :: rule
        type(status_t), intent(out) :: status
        integer :: dim

        call status%clear()
        dim = reference_dimension(topology_id)
        if (dim == 0) then
            call rule%clear()
            call status%set_error(FEM_STATUS_INVALID_ARGUMENT, &
                "Standart quadrature icin desteklenmeyen topology ID.")
            return
        end if

        select case (topology_id)
        case (TOPOLOGY_BAR2, TOPOLOGY_QUAD4, TOPOLOGY_HEX8)
            call create_gauss_rule(dim, 2, rule, status)
        case default
            call rule%clear()
            call status%set_error(FEM_STATUS_INVALID_ARGUMENT, &
                "Standart quadrature icin desteklenmeyen topology ID.")
        end select
    end subroutine standard_quadrature_rule

end module fem_quadrature
