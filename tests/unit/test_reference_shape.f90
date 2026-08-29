program test_reference_shape
    use fem_kinds, only : rk, id_kind
    use fem_topology, only : TOPOLOGY_BAR2, TOPOLOGY_QUAD4, TOPOLOGY_HEX8
    use fem_reference_elements, only : reference_node_coordinates
    use fem_shape_functions, only : evaluate_shape_functions
    use fem_status, only : status_t
    use test_support, only : assert_true, assert_close
    implicit none

    integer(id_kind), parameter :: topologies(3) = [TOPOLOGY_BAR2, TOPOLOGY_QUAD4, TOPOLOGY_HEX8]
    real(rk), allocatable :: nodes(:,:), shape(:), dshape(:,:)
    type(status_t) :: status
    integer :: t, a, b

    do t = 1, size(topologies)
        call reference_node_coordinates(topologies(t), nodes, status)
        call assert_true(status%is_ok(), "reference nodes status")
        do a = 1, size(nodes,2)
            call evaluate_shape_functions(topologies(t), nodes(:,a), shape, dshape, status)
            call assert_true(status%is_ok(), "shape at reference node")
            call assert_close(sum(shape), 1.0_rk, 1.0e-13_rk, 1.0e-13_rk, "partition of unity")
            do b = 1, size(shape)
                if (a == b) then
                    call assert_close(shape(b), 1.0_rk, 1.0e-13_rk, 1.0e-13_rk, "Kronecker diagonal")
                else
                    call assert_close(shape(b), 0.0_rk, 1.0e-13_rk, 1.0e-13_rk, "Kronecker off-diagonal")
                end if
            end do
        end do

        call evaluate_shape_functions(topologies(t), 0.0_rk*nodes(:,1), shape, dshape, status)
        call assert_true(status%is_ok(), "shape at center")
        call assert_close(sum(shape), 1.0_rk, 1.0e-13_rk, 1.0e-13_rk, "center partition")
        do a = 1, size(dshape,1)
            call assert_close(sum(dshape(a,:)), 0.0_rk, 1.0e-13_rk, 1.0e-13_rk, "derivative sum zero")
        end do
    end do

    write(*,'(A)') "PASS unit_reference_shape"
end program test_reference_shape
