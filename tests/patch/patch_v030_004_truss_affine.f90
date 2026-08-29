program patch_v030_004_truss_affine
    use fem_kinds, only : rk
    use fem_topology, only : TOPOLOGY_BAR2
    use fem_element_kernel, only : element_geometry_point_t, evaluate_geometry_point
    use fem_status, only : status_t
    use test_support, only : assert_true, assert_close
    implicit none

    real(rk) :: x(3,2), displacement(3,2), axial_strain
    real(rk), parameter :: alpha=0.025_rk
    type(element_geometry_point_t) :: point
    type(status_t) :: status
    integer :: a

    x(:,1)=[1.0_rk,-2.0_rk,0.5_rk]
    x(:,2)=[3.0_rk, 2.0_rk,1.5_rk]
    call evaluate_geometry_point(TOPOLOGY_BAR2, x, [0.37_rk], 1.0_rk, point, status)
    call assert_true(status%is_ok(), "3D TRUSS2 geometry")

    do a=1,2
        displacement(:,a)=alpha*dot_product(x(:,a),point%direction)*point%direction
    end do
    axial_strain=0.0_rk
    do a=1,2
        axial_strain=axial_strain + point%dshape_dphysical(1,a) * &
            dot_product(displacement(:,a),point%direction)
    end do
    call assert_close(axial_strain,alpha,1.0e-12_rk,1.0e-12_rk,"constant truss axial strain")

    write(*,'(A)') "PASS patch_v030_004_truss_affine"
end program patch_v030_004_truss_affine
