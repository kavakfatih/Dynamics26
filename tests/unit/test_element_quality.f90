program test_element_quality
    use fem_kinds, only : rk
    use fem_topology, only : TOPOLOGY_QUAD4
    use fem_element_kernel, only : element_quality_t, assess_element_quality
    use fem_status, only : status_t
    use test_support, only : assert_true, assert_close
    implicit none

    real(rk) :: regular(2,4), inverted(2,4), degenerate(2,4)
    type(element_quality_t) :: quality
    type(status_t) :: status

    regular(:,1) = [0.0_rk,0.0_rk]
    regular(:,2) = [2.0_rk,0.0_rk]
    regular(:,3) = [2.0_rk,1.0_rk]
    regular(:,4) = [0.0_rk,1.0_rk]
    call assess_element_quality(TOPOLOGY_QUAD4, regular, quality, status)
    call assert_true(status%is_ok(), "regular quad quality")
    call assert_true(.not. quality%is_inverted, "regular not inverted")
    call assert_close(quality%jacobian_ratio, 1.0_rk, 1.0e-14_rk, 1.0e-14_rk, "regular ratio")

    inverted(:,1) = regular(:,1)
    inverted(:,2) = regular(:,4)
    inverted(:,3) = regular(:,3)
    inverted(:,4) = regular(:,2)
    call assess_element_quality(TOPOLOGY_QUAD4, inverted, quality, status)
    call assert_true(.not. status%is_ok(), "clockwise quad rejected")
    call assert_true(quality%is_inverted, "inverted flag")

    degenerate(:,1) = [0.0_rk,0.0_rk]
    degenerate(:,2) = [1.0_rk,0.0_rk]
    degenerate(:,3) = [2.0_rk,0.0_rk]
    degenerate(:,4) = [3.0_rk,0.0_rk]
    call assess_element_quality(TOPOLOGY_QUAD4, degenerate, quality, status)
    call assert_true(.not. status%is_ok(), "degenerate quad rejected")
    call assert_true(quality%is_degenerate, "degenerate flag")

    write(*,'(A)') "PASS unit_element_quality"
end program test_element_quality
