program test_fields
    use fem_kinds,  only : id_kind, index_kind
    use fem_fields, only : field_registry_t, FIELD_ID_DISPLACEMENT, FIELD_ID_PRESSURE, FIELD_ID_ROTATION, FIELD_ID_PRESSURE_P0, &
                           FIELD_ASSOCIATION_NODE, FIELD_ASSOCIATION_ELEMENT
    use fem_status, only : status_t
    use test_support, only : assert_true, assert_equal_int, assert_equal_index
    implicit none

    type(field_registry_t) :: fields
    type(status_t) :: status

    call fields%register_standard_structural(status)
    call assert_true(status%is_ok(), "standard structural fields kaydedilmeli")
    call assert_equal_index(fields%count(), 4_index_kind, "field count")
    call assert_equal_int(fields%get_component_count(FIELD_ID_DISPLACEMENT), 3, "displacement 3 component")
    call assert_equal_int(fields%get_component_count(FIELD_ID_PRESSURE), 1, "pressure 1 component")
    call assert_equal_int(fields%get_component_count(FIELD_ID_ROTATION), 3, "rotation 3 component")
    call assert_equal_int(fields%get_component_count(FIELD_ID_PRESSURE_P0), 1, "P0 pressure 1 component")
    call assert_true(fields%fields(fields%find_position(FIELD_ID_PRESSURE_P0))%association == FIELD_ASSOCIATION_ELEMENT, &
                     "P0 pressure element-associated")

    call fields%add(50_id_kind, "temperature_custom", 1, FIELD_ASSOCIATION_NODE, status)
    call assert_true(status%is_ok(), "gelecekte custom field eklenebilmeli")
    call fields%add(50_id_kind, "duplicate", 1, FIELD_ASSOCIATION_NODE, status)
    call assert_true(.not. status%is_ok(), "duplicate Field ID reddedilmeli")

    write(*, '(A)') "PASS unit_fields"
end program test_fields
