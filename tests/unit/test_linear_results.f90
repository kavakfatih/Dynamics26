program test_linear_results
    use fem_kinds, only : rk
    use fem_linear_results, only : von_mises_3d,von_mises_plane,von_mises_plane_strain,von_mises_axisymmetric
    use test_support, only : assert_close
    implicit none
    call assert_close(von_mises_3d([100.0_rk,0.0_rk,0.0_rk,0.0_rk,0.0_rk,0.0_rk]),100.0_rk,1.0e-12_rk,0.0_rk,"uniaxial VM")
    call assert_close(von_mises_plane([100.0_rk,0.0_rk,0.0_rk]),100.0_rk,1.0e-12_rk,0.0_rk,"plane-stress VM")
    call assert_close(von_mises_plane_strain([100.0_rk,0.0_rk,0.0_rk,40.0_rk]), &
        sqrt(0.5_rk*(100.0_rk**2+40.0_rk**2+60.0_rk**2)),1.0e-12_rk,0.0_rk,"plane-strain VM includes sigma zz")
    call assert_close(von_mises_axisymmetric([100.0_rk,0.0_rk,0.0_rk,0.0_rk]),100.0_rk,1.0e-12_rk,0.0_rk,"axisym VM")
    call assert_close(von_mises_3d([50.0_rk,50.0_rk,50.0_rk,0.0_rk,0.0_rk,0.0_rk]),0.0_rk,1.0e-12_rk,0.0_rk,"hydrostatic VM zero")
    write(*,'(A)') "PASS unit_linear_results"
end program test_linear_results
