program test_sections_loads
    use fem_kinds, only : rk,id_kind,index_kind
    use fem_sections, only : section_t, section_registry_t, SECTION_TRUSS, SECTION_PLANE, SECTION_BEAM
    use fem_loads, only : nodal_load_set_t
    use fem_status, only : status_t
    use test_support, only : assert_true,assert_equal_index,assert_equal_id,assert_close
    implicit none
    type(section_registry_t) :: reg
    type(section_t) :: s
    type(nodal_load_set_t) :: loads
    type(status_t) :: status
    integer(id_kind) :: lid
    s=section_t(id=1_id_kind,name="bar",kind=SECTION_TRUSS,area=1.0e-4_rk)
    call reg%add(s,status); call assert_true(status%is_ok(),"truss section")
    s=section_t(id=2_id_kind,name="sheet",kind=SECTION_PLANE,thickness=0.01_rk)
    call reg%add(s,status); call assert_true(status%is_ok(),"plane section")
    s=section_t(id=3_id_kind,name="beam",kind=SECTION_BEAM,area=2.0e-3_rk,iz=8.0e-6_rk)
    call reg%add(s,status); call assert_true(status%is_ok(),"beam section")
    call assert_equal_index(reg%count(),3_index_kind,"section count")
    call loads%add(77_id_kind,-125.0_rk,lid,status); call assert_true(status%is_ok(),"nodal load")
    call assert_equal_id(lid,0_id_kind,"load id zero based")
    call assert_close(loads%loads(1)%value,-125.0_rk,1.0e-12_rk,0.0_rk,"load value")
    write(*,'(A)') "PASS unit_sections_loads"
end program test_sections_loads
