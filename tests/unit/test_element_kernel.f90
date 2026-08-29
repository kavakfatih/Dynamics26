program test_element_kernel
    use fem_kinds, only : rk, id_kind
    use fem_constants, only : FEM_PI
    use fem_topology, only : TOPOLOGY_QUAD4, TOPOLOGY_HEX8
    use fem_element_kernel, only : element_geometry_point_t, evaluate_geometry_point, &
        evaluate_element_integration_geometry
    use fem_element_results, only : element_result_t
    use fem_status, only : status_t
    use test_support, only : assert_true, assert_close
    implicit none

    real(rk) :: quad(2,4), hex(3,8)
    type(element_geometry_point_t) :: point
    type(element_result_t) :: result
    type(status_t) :: status

    quad(:,1) = [1.0_rk, 0.0_rk]
    quad(:,2) = [2.0_rk, 0.0_rk]
    quad(:,3) = [2.0_rk, 3.0_rk]
    quad(:,4) = [1.0_rk, 3.0_rk]

    call evaluate_geometry_point(TOPOLOGY_QUAD4, quad, [0.0_rk,0.0_rk], 1.0_rk, point, status)
    call assert_true(status%is_ok(), "quad center geometry")
    call assert_close(point%physical_coordinate(1), 1.5_rk, 1.0e-14_rk, 1.0e-14_rk, "mapped x")
    call assert_close(point%physical_coordinate(2), 1.5_rk, 1.0e-14_rk, 1.0e-14_rk, "mapped y")
    call assert_close(point%det_jacobian, 0.75_rk, 1.0e-14_rk, 1.0e-14_rk, "quad detJ")

    call evaluate_element_integration_geometry(900_id_kind, TOPOLOGY_QUAD4, quad, result, status)
    call assert_true(status%is_ok(), "quad integration geometry")
    call assert_close(sum(result%integration_measure), 3.0_rk, 1.0e-13_rk, 1.0e-13_rk, "quad area")

    call evaluate_element_integration_geometry(901_id_kind, TOPOLOGY_QUAD4, quad, result, status, .true.)
    call assert_true(status%is_ok(), "axisym integration geometry")
    call assert_close(sum(result%integration_measure), 9.0_rk*FEM_PI, 1.0e-12_rk, 1.0e-12_rk, "axisym annular volume")

    hex(:,1) = [0.0_rk,0.0_rk,0.0_rk]
    hex(:,2) = [2.0_rk,0.0_rk,0.0_rk]
    hex(:,3) = [2.0_rk,3.0_rk,0.0_rk]
    hex(:,4) = [0.0_rk,3.0_rk,0.0_rk]
    hex(:,5) = [0.0_rk,0.0_rk,4.0_rk]
    hex(:,6) = [2.0_rk,0.0_rk,4.0_rk]
    hex(:,7) = [2.0_rk,3.0_rk,4.0_rk]
    hex(:,8) = [0.0_rk,3.0_rk,4.0_rk]
    call evaluate_element_integration_geometry(902_id_kind, TOPOLOGY_HEX8, hex, result, status)
    call assert_true(status%is_ok(), "hex integration geometry")
    call assert_close(sum(result%integration_measure), 24.0_rk, 1.0e-12_rk, 1.0e-12_rk, "hex volume")

    write(*,'(A)') "PASS unit_element_kernel"
end program test_element_kernel
