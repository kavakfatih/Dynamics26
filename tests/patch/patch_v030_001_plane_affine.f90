program patch_v030_001_plane_affine
    use fem_kinds, only : rk
    use fem_topology, only : TOPOLOGY_QUAD4
    use fem_element_kernel, only : element_geometry_point_t, evaluate_geometry_point
    use fem_element_kinematics, only : build_plane_b_matrix
    use fem_status, only : status_t
    use test_support, only : assert_true, assert_close
    implicit none

    real(rk) :: x(2,4), u_nodes(8), strain(3)
    real(rk), allocatable :: b(:,:)
    real(rk), parameter :: a0=0.7_rk, bx=0.12_rk, cy=-0.08_rk
    real(rk), parameter :: d0=-0.2_rk, ex=0.05_rk, fy=0.21_rk
    type(element_geometry_point_t) :: point
    type(status_t) :: status
    integer :: n

    x(:,1) = [ 0.0_rk, 0.0_rk]
    x(:,2) = [ 2.0_rk, 0.2_rk]
    x(:,3) = [ 2.3_rk, 1.5_rk]
    x(:,4) = [-0.1_rk, 1.0_rk]

    do n = 1, 4
        u_nodes(2*n-1) = a0 + bx*x(1,n) + cy*x(2,n)
        u_nodes(2*n)   = d0 + ex*x(1,n) + fy*x(2,n)
    end do

    call evaluate_geometry_point(TOPOLOGY_QUAD4, x, [0.31_rk,-0.27_rk], 1.0_rk, point, status)
    call assert_true(status%is_ok(), "distorted QUAD4 geometry")
    call build_plane_b_matrix(point%dshape_dphysical, b, status)
    call assert_true(status%is_ok(), "plane B")
    strain = matmul(b, u_nodes)

    call assert_close(strain(1), bx, 1.0e-12_rk, 1.0e-12_rk, "constant eps_xx patch")
    call assert_close(strain(2), fy, 1.0e-12_rk, 1.0e-12_rk, "constant eps_yy patch")
    call assert_close(strain(3), cy+ex, 1.0e-12_rk, 1.0e-12_rk, "constant gamma_xy patch")

    write(*,'(A)') "PASS patch_v030_001_plane_affine"
end program patch_v030_001_plane_affine
