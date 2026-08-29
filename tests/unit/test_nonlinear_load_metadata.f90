program test_nonlinear_load_metadata
    use fem_nonlinear_loads,only:nonlinear_load_metadata_t,LOAD_CONFIGURATION_REFERENCE,LOAD_CONFIGURATION_CURRENT
    use fem_status,only:status_t
    use test_support,only:assert_true
    implicit none
    type(nonlinear_load_metadata_t)::meta
    type(status_t)::status
    meta=nonlinear_load_metadata_t(configuration=LOAD_CONFIGURATION_REFERENCE, &
        follows_deformation=.false.,contributes_external_tangent=.false.)
    call meta%validate(status);call assert_true(status%is_ok(),"reference dead-load metadata")
    meta=nonlinear_load_metadata_t(configuration=LOAD_CONFIGURATION_CURRENT, &
        follows_deformation=.true.,contributes_external_tangent=.true.)
    call meta%validate(status);call assert_true(status%is_ok(),"current follower-load metadata")
    meta%contributes_external_tangent=.false.
    call meta%validate(status);call assert_true(.not.status%is_ok(),"follower load without external tangent rejected")
end program test_nonlinear_load_metadata
