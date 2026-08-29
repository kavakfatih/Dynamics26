module fem_element_results
    !! Element-local geometri/integration sonucunu global result sisteminden ayiran
    !! hafif container. Stress/strain gibi formulation sonuc alanlari sonraki
    !! surumlerde bu katmanin ustune eklenecektir.
    use fem_kinds, only : rk, id_kind
    use fem_ids,   only : INVALID_ID, id_is_valid
    use fem_status, only : status_t, FEM_STATUS_INVALID_ARGUMENT
    implicit none
    private

    type, public :: element_result_t
        integer(id_kind) :: element_id = INVALID_ID
        integer :: spatial_dimension = 0
        integer :: integration_point_count = 0
        real(rk), allocatable :: physical_points(:, :)
        real(rk), allocatable :: det_jacobian(:)
        real(rk), allocatable :: integration_measure(:)
    contains
        procedure :: clear => element_result_clear
        procedure :: initialize => element_result_initialize
    end type element_result_t

contains

    subroutine element_result_clear(this)
        class(element_result_t), intent(inout) :: this
        if (allocated(this%physical_points)) deallocate(this%physical_points)
        if (allocated(this%det_jacobian)) deallocate(this%det_jacobian)
        if (allocated(this%integration_measure)) deallocate(this%integration_measure)
        this%element_id = INVALID_ID
        this%spatial_dimension = 0
        this%integration_point_count = 0
    end subroutine element_result_clear

    subroutine element_result_initialize(this, element_id, spatial_dimension, point_count, status)
        class(element_result_t), intent(inout) :: this
        integer(id_kind), intent(in) :: element_id
        integer, intent(in) :: spatial_dimension, point_count
        type(status_t), intent(out) :: status

        call status%clear()
        call this%clear()
        if (.not. id_is_valid(element_id) .or. spatial_dimension < 1 .or. spatial_dimension > 3 .or. point_count < 1) then
            call status%set_error(FEM_STATUS_INVALID_ARGUMENT, "Element result container boyutlari gecersiz.")
            return
        end if

        this%element_id = element_id
        this%spatial_dimension = spatial_dimension
        this%integration_point_count = point_count
        allocate(this%physical_points(spatial_dimension, point_count))
        allocate(this%det_jacobian(point_count))
        allocate(this%integration_measure(point_count))
        this%physical_points = 0.0_rk
        this%det_jacobian = 0.0_rk
        this%integration_measure = 0.0_rk
    end subroutine element_result_initialize

end module fem_element_results
