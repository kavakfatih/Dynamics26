program patch_v030_003_axisymmetric_affine
    use fem_kinds, only : rk
    use fem_topology, only : TOPOLOGY_QUAD4
    use fem_element_kernel, only : element_geometry_point_t, evaluate_geometry_point
    use fem_element_kinematics, only : build_axisymmetric_b_matrix
    use fem_status, only : status_t
    use test_support, only : assert_true, assert_close
    implicit none

    real(rk) :: x(2,4), u_nodes(8), strain(4)
    real(rk), allocatable :: b(:,:)
    real(rk), parameter :: alpha=0.08_rk, beta=0.13_rk, c=0.02_rk, d=-0.01_rk
    type(element_geometry_point_t) :: point
    type(status_t) :: status
    integer :: n

    x(:,1)=[1.0_rk,0.0_rk]; x(:,2)=[2.0_rk,0.0_rk]
    x(:,3)=[2.0_rk,3.0_rk]; x(:,4)=[1.0_rk,3.0_rk]
    do n=1,4
        u_nodes(2*n-1) = alpha*x(1,n) + c*x(2,n)
        u_nodes(2*n)   = d*x(1,n) + beta*x(2,n)
    end do

    call evaluate_geometry_point(TOPOLOGY_QUAD4, x, [0.2_rk,-0.4_rk], 1.0_rk, point, status, .true.)
    call assert_true(status%is_ok(), "axisymmetric geometry")
    call build_axisymmetric_b_matrix(point%shape, point%dshape_dphysical, point%radius, b, status)
    call assert_true(status%is_ok(), "axisymmetric B")
    strain=matmul(b,u_nodes)

    call assert_close(strain(1), alpha, 1.0e-12_rk, 1.0e-12_rk, "eps_rr")
    call assert_close(strain(2), beta, 1.0e-12_rk, 1.0e-12_rk, "eps_zz")
    call assert_close(strain(3), c+d, 1.0e-12_rk, 1.0e-12_rk, "gamma_rz")
    call assert_close(strain(4), alpha + c*point%physical_coordinate(2)/point%radius, &
        1.0e-12_rk, 1.0e-12_rk, "eps_tt = u_r/r")

    write(*,'(A)') "PASS patch_v030_003_axisymmetric_affine"
end program patch_v030_003_axisymmetric_affine
