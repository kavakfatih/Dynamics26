program test_coordinate_frames
    use fem_kinds, only : rk, id_kind, index_kind
    use fem_coordinate_frames, only : coordinate_frame_t, coordinate_frame_registry_t
    use fem_status, only : status_t
    use test_support, only : assert_true, assert_equal_index
    implicit none

    type(coordinate_frame_t) :: frame
    type(coordinate_frame_registry_t) :: registry
    type(status_t) :: status
    real(rk) :: identity(3,3), bad_axes(3,3)

    identity = 0.0_rk
    identity(1,1) = 1.0_rk
    identity(2,2) = 1.0_rk
    identity(3,3) = 1.0_rk
    call frame%set(10_id_kind, [1.0_rk, 2.0_rk, 3.0_rk], identity, status)
    call assert_true(status%is_ok(), "identity local frame kabul edilmeli")
    call assert_true(frame%is_orthonormal(), "identity frame ortonormal")
    call registry%add(10_id_kind, [1.0_rk, 2.0_rk, 3.0_rk], identity, status)
    call assert_true(status%is_ok(), "frame registry ekleme")
    call assert_equal_index(registry%find_position(10_id_kind), 1_index_kind, "frame ID lookup")
    call registry%add(10_id_kind, [0.0_rk, 0.0_rk, 0.0_rk], identity, status)
    call assert_true(.not. status%is_ok(), "duplicate frame ID reddedilmeli")

    bad_axes = identity
    bad_axes(:,2) = bad_axes(:,1)
    call frame%set(11_id_kind, [0.0_rk, 0.0_rk, 0.0_rk], bad_axes, status)
    call assert_true(.not. status%is_ok(), "lineer bagimli eksenler reddedilmeli")

    bad_axes = identity
    bad_axes(:,3) = -bad_axes(:,3)
    call frame%set(12_id_kind, [0.0_rk, 0.0_rk, 0.0_rk], bad_axes, status)
    call assert_true(.not. status%is_ok(), "sol-el frame reddedilmeli")

    write(*, '(A)') "PASS unit_coordinate_frames"
end program test_coordinate_frames
