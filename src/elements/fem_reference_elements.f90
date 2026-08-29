module fem_reference_elements
    !! Reference element ve natural-coordinate tanimlari.
    !!
    !! Bu modul geometrik topolojinin referans uzaydaki dugum konumlarini tek
    !! yerde kilitler. Eleman formulation'lari bu koordinatlari yeniden
    !! tanimlamaz; boylece shape-function ve connectivity sirasi arasindaki
    !! sozlesme acik ve test edilebilir kalir.
    use fem_kinds,    only : rk, id_kind
    use fem_topology, only : TOPOLOGY_BAR2, TOPOLOGY_QUAD4, TOPOLOGY_HEX8
    use fem_status,   only : status_t, FEM_STATUS_INVALID_ARGUMENT
    implicit none
    private

    public :: reference_dimension, reference_node_count
    public :: reference_node_coordinates

contains

    pure integer function reference_dimension(topology_id) result(dimension)
        integer(id_kind), intent(in) :: topology_id

        select case (topology_id)
        case (TOPOLOGY_BAR2)
            dimension = 1
        case (TOPOLOGY_QUAD4)
            dimension = 2
        case (TOPOLOGY_HEX8)
            dimension = 3
        case default
            dimension = 0
        end select
    end function reference_dimension

    pure integer function reference_node_count(topology_id) result(node_count)
        integer(id_kind), intent(in) :: topology_id

        select case (topology_id)
        case (TOPOLOGY_BAR2)
            node_count = 2
        case (TOPOLOGY_QUAD4)
            node_count = 4
        case (TOPOLOGY_HEX8)
            node_count = 8
        case default
            node_count = 0
        end select
    end function reference_node_count

    subroutine reference_node_coordinates(topology_id, coordinates, status)
        integer(id_kind), intent(in) :: topology_id
        real(rk), allocatable, intent(out) :: coordinates(:, :)
        type(status_t), intent(out) :: status

        call status%clear()

        select case (topology_id)
        case (TOPOLOGY_BAR2)
            allocate(coordinates(1, 2))
            coordinates(1, :) = [-1.0_rk, 1.0_rk]

        case (TOPOLOGY_QUAD4)
            allocate(coordinates(2, 4))
            coordinates(:, 1) = [-1.0_rk, -1.0_rk]
            coordinates(:, 2) = [ 1.0_rk, -1.0_rk]
            coordinates(:, 3) = [ 1.0_rk,  1.0_rk]
            coordinates(:, 4) = [-1.0_rk,  1.0_rk]

        case (TOPOLOGY_HEX8)
            allocate(coordinates(3, 8))
            coordinates(:, 1) = [-1.0_rk, -1.0_rk, -1.0_rk]
            coordinates(:, 2) = [ 1.0_rk, -1.0_rk, -1.0_rk]
            coordinates(:, 3) = [ 1.0_rk,  1.0_rk, -1.0_rk]
            coordinates(:, 4) = [-1.0_rk,  1.0_rk, -1.0_rk]
            coordinates(:, 5) = [-1.0_rk, -1.0_rk,  1.0_rk]
            coordinates(:, 6) = [ 1.0_rk, -1.0_rk,  1.0_rk]
            coordinates(:, 7) = [ 1.0_rk,  1.0_rk,  1.0_rk]
            coordinates(:, 8) = [-1.0_rk,  1.0_rk,  1.0_rk]

        case default
            allocate(coordinates(0, 0))
            call status%set_error(FEM_STATUS_INVALID_ARGUMENT, &
                "Reference coordinate icin desteklenmeyen topology ID.")
        end select
    end subroutine reference_node_coordinates

end module fem_reference_elements
