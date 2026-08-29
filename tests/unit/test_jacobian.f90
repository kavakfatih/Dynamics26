program test_jacobian
    use fem_kinds, only : rk
    use fem_topology, only : TOPOLOGY_QUAD4, TOPOLOGY_BAR2
    use fem_shape_functions, only : evaluate_shape_functions
    use fem_jacobian, only : compute_square_jacobian, map_shape_gradients, compute_embedded_line_metric
    use fem_status, only : status_t
    use test_support, only : assert_true, assert_close
    implicit none

    real(rk) :: quad(2,4), scaled_quad(2,4), line(3,2)
    real(rk), allocatable :: shape(:), dshape(:,:), jac(:,:), invjac(:,:), dshape_dx(:,:)
    real(rk), allocatable :: tangent(:), direction(:), dshape_ds(:)
    real(rk) :: detj, metric
    type(status_t) :: status

    quad(:,1) = [0.0_rk, 0.0_rk]
    quad(:,2) = [2.0_rk, 0.0_rk]
    quad(:,3) = [2.0_rk, 1.0_rk]
    quad(:,4) = [0.0_rk, 1.0_rk]
    call evaluate_shape_functions(TOPOLOGY_QUAD4, [0.0_rk,0.0_rk], shape, dshape, status)
    call assert_true(status%is_ok(), "quad shape")
    call compute_square_jacobian(quad, dshape, jac, invjac, detj, status)
    call assert_true(status%is_ok(), "quad jacobian")
    call assert_close(jac(1,1), 1.0_rk, 1.0e-14_rk, 1.0e-14_rk, "J11")
    call assert_close(jac(2,2), 0.5_rk, 1.0e-14_rk, 1.0e-14_rk, "J22")
    call assert_close(detj, 0.5_rk, 1.0e-14_rk, 1.0e-14_rk, "detJ")
    call assert_close(invjac(1,1), 1.0_rk, 1.0e-14_rk, 1.0e-14_rk, "invJ11")
    call assert_close(invjac(2,2), 2.0_rk, 1.0e-14_rk, 1.0e-14_rk, "invJ22")
    call map_shape_gradients(dshape, invjac, dshape_dx, status)
    call assert_true(status%is_ok(), "map gradient")
    call assert_close(sum(dshape_dx(1,:)), 0.0_rk, 1.0e-14_rk, 1.0e-14_rk, "physical dx sum")
    call assert_close(sum(dshape_dx(2,:)), 0.0_rk, 1.0e-14_rk, 1.0e-14_rk, "physical dy sum")

    ! Singularity karari modelin m/mm/um gibi uzunluk olceginden bagimsiz
    ! kalmalidir. Ayni regular element cok kucuk olcekte de reddedilmemeli.
    scaled_quad = 1.0e-12_rk * quad
    call compute_square_jacobian(scaled_quad, dshape, jac, invjac, detj, status)
    call assert_true(status%is_ok(), "scale-independent Jacobian singularity check")
    call assert_true(detj > 0.0_rk, "scaled regular element positive detJ")

    line(:,1) = [0.0_rk, 0.0_rk, 0.0_rk]
    line(:,2) = [2.0_rk, 2.0_rk, 1.0_rk]
    call evaluate_shape_functions(TOPOLOGY_BAR2, [0.0_rk], shape, dshape, status)
    call compute_embedded_line_metric(line, dshape, tangent, metric, direction, dshape_ds, status)
    call assert_true(status%is_ok(), "embedded line metric")
    call assert_close(metric, 1.5_rk, 1.0e-14_rk, 1.0e-14_rk, "line L/2 metric")
    call assert_close(sqrt(dot_product(direction,direction)), 1.0_rk, 1.0e-14_rk, 1.0e-14_rk, "line direction unit")
    call assert_close(sum(dshape_ds), 0.0_rk, 1.0e-14_rk, 1.0e-14_rk, "line derivative sum")

    write(*,'(A)') "PASS unit_jacobian"
end program test_jacobian
