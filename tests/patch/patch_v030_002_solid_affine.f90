program patch_v030_002_solid_affine
    use fem_kinds, only : rk
    use fem_topology, only : TOPOLOGY_HEX8
    use fem_element_kernel, only : element_geometry_point_t, evaluate_geometry_point
    use fem_element_kinematics, only : build_solid_b_matrix
    use fem_status, only : status_t
    use test_support, only : assert_true, assert_close
    implicit none

    real(rk) :: x(3,8), q(3,8), u_nodes(24), strain(6), a(3,3), origin(3)
    real(rk), allocatable :: b(:,:)
    type(element_geometry_point_t) :: point
    type(status_t) :: status
    integer :: n

    q(:,1)=[0.0_rk,0.0_rk,0.0_rk]; q(:,2)=[1.0_rk,0.0_rk,0.0_rk]
    q(:,3)=[1.0_rk,1.0_rk,0.0_rk]; q(:,4)=[0.0_rk,1.0_rk,0.0_rk]
    q(:,5)=[0.0_rk,0.0_rk,1.0_rk]; q(:,6)=[1.0_rk,0.0_rk,1.0_rk]
    q(:,7)=[1.0_rk,1.0_rk,1.0_rk]; q(:,8)=[0.0_rk,1.0_rk,1.0_rk]

    a(:,1) = [2.0_rk, 0.2_rk, 0.1_rk]
    a(:,2) = [0.3_rk, 1.5_rk, 0.2_rk]
    a(:,3) = [0.1_rk, 0.4_rk, 1.2_rk]
    origin = [0.4_rk,-0.2_rk,0.7_rk]
    do n=1,8
        x(:,n)=origin + matmul(a,q(:,n))
    end do

    do n=1,8
        u_nodes(3*n-2) = 0.1_rk + 0.11_rk*x(1,n) - 0.03_rk*x(2,n) + 0.04_rk*x(3,n)
        u_nodes(3*n-1) =-0.2_rk + 0.07_rk*x(1,n) + 0.13_rk*x(2,n) - 0.02_rk*x(3,n)
        u_nodes(3*n)   = 0.3_rk - 0.05_rk*x(1,n) + 0.06_rk*x(2,n) + 0.17_rk*x(3,n)
    end do

    call evaluate_geometry_point(TOPOLOGY_HEX8, x, [0.21_rk,-0.17_rk,0.33_rk], 1.0_rk, point, status)
    call assert_true(status%is_ok(), "parallelepiped HEX8 geometry")
    call build_solid_b_matrix(point%dshape_dphysical, b, status)
    call assert_true(status%is_ok(), "solid B")
    strain = matmul(b,u_nodes)

    call assert_close(strain(1), 0.11_rk, 2.0e-12_rk, 2.0e-12_rk, "eps_xx")
    call assert_close(strain(2), 0.13_rk, 2.0e-12_rk, 2.0e-12_rk, "eps_yy")
    call assert_close(strain(3), 0.17_rk, 2.0e-12_rk, 2.0e-12_rk, "eps_zz")
    call assert_close(strain(4), 0.04_rk, 2.0e-12_rk, 2.0e-12_rk, "gamma_xy")
    call assert_close(strain(5), 0.04_rk, 2.0e-12_rk, 2.0e-12_rk, "gamma_yz")
    call assert_close(strain(6),-0.01_rk, 2.0e-12_rk, 2.0e-12_rk, "gamma_xz")

    write(*,'(A)') "PASS patch_v030_002_solid_affine"
end program patch_v030_002_solid_affine
