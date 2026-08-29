module fem_element_kernel
    !! V0.3.0 element-local geometry kernel'i.
    !!
    !! Bu katman material/stiffness bilmez. Gorevi:
    !!   natural point -> shape -> isoparametric map -> Jacobian -> physical gradient
    !! zincirini tek bir test edilebilir sozlesmede toplamaktir.
    use fem_kinds,              only : rk, id_kind
    use fem_constants,          only : FEM_PI
    use fem_topology,           only : TOPOLOGY_BAR2, TOPOLOGY_QUAD4, TOPOLOGY_HEX8
    use fem_reference_elements, only : reference_dimension, reference_node_count
    use fem_shape_functions,    only : evaluate_shape_functions
    use fem_quadrature,         only : quadrature_rule_t, standard_quadrature_rule
    use fem_jacobian,           only : compute_square_jacobian, map_shape_gradients, &
                                       compute_embedded_line_metric
    use fem_element_results,    only : element_result_t
    use fem_status,             only : status_t, FEM_STATUS_INVALID_ARGUMENT, &
                                       FEM_STATUS_SIZE_MISMATCH, FEM_STATUS_NUMERICAL_FAILURE
    implicit none
    private

    type, public :: element_geometry_point_t
        real(rk), allocatable :: shape(:)
        real(rk), allocatable :: dshape_dnatural(:, :)
        real(rk), allocatable :: dshape_dphysical(:, :)
        real(rk), allocatable :: physical_coordinate(:)
        real(rk), allocatable :: jacobian(:, :)
        real(rk), allocatable :: inverse_jacobian(:, :)
        real(rk), allocatable :: tangent(:)
        real(rk), allocatable :: direction(:)
        real(rk) :: det_jacobian = 0.0_rk
        real(rk) :: integration_measure = 0.0_rk
        real(rk) :: radius = 0.0_rk
    contains
        procedure :: clear => geometry_point_clear
    end type element_geometry_point_t

    type, public :: element_quality_t
        real(rk) :: min_det_jacobian = 0.0_rk
        real(rk) :: max_det_jacobian = 0.0_rk
        real(rk) :: jacobian_ratio = 0.0_rk
        logical :: is_degenerate = .false.
        logical :: is_inverted = .false.
    end type element_quality_t

    public :: evaluate_geometry_point
    public :: evaluate_element_integration_geometry
    public :: assess_element_quality

contains

    subroutine geometry_point_clear(this)
        class(element_geometry_point_t), intent(inout) :: this
        if (allocated(this%shape)) deallocate(this%shape)
        if (allocated(this%dshape_dnatural)) deallocate(this%dshape_dnatural)
        if (allocated(this%dshape_dphysical)) deallocate(this%dshape_dphysical)
        if (allocated(this%physical_coordinate)) deallocate(this%physical_coordinate)
        if (allocated(this%jacobian)) deallocate(this%jacobian)
        if (allocated(this%inverse_jacobian)) deallocate(this%inverse_jacobian)
        if (allocated(this%tangent)) deallocate(this%tangent)
        if (allocated(this%direction)) deallocate(this%direction)
        this%det_jacobian = 0.0_rk
        this%integration_measure = 0.0_rk
        this%radius = 0.0_rk
    end subroutine geometry_point_clear

    subroutine evaluate_geometry_point(topology_id, node_coordinates, natural_coordinates, &
                                       quadrature_weight, point, status, axisymmetric)
        integer(id_kind), intent(in) :: topology_id
        real(rk), intent(in) :: node_coordinates(:, :)
        real(rk), intent(in) :: natural_coordinates(:)
        real(rk), intent(in) :: quadrature_weight
        type(element_geometry_point_t), intent(inout) :: point
        type(status_t), intent(out) :: status
        logical, intent(in), optional :: axisymmetric
        logical :: use_axisymmetric
        integer :: dim, nn
        real(rk), allocatable :: dshape_ds(:)

        call status%clear()
        call point%clear()
        use_axisymmetric = .false.
        if (present(axisymmetric)) use_axisymmetric = axisymmetric

        dim = reference_dimension(topology_id)
        nn = reference_node_count(topology_id)
        if (dim == 0 .or. nn == 0) then
            call status%set_error(FEM_STATUS_INVALID_ARGUMENT, "Geometry kernel desteklenmeyen topology ID aldi.")
            return
        end if
        if (size(node_coordinates,2) /= nn) then
            call status%set_error(FEM_STATUS_SIZE_MISMATCH, "Geometry kernel node sayisi topology ile uyusmuyor.")
            return
        end if
        if (size(natural_coordinates) /= dim) then
            call status%set_error(FEM_STATUS_SIZE_MISMATCH, "Geometry kernel natural-coordinate boyutu uyusmuyor.")
            return
        end if
        if (use_axisymmetric .and. topology_id /= TOPOLOGY_QUAD4) then
            call status%set_error(FEM_STATUS_INVALID_ARGUMENT, "Axisymmetric prototype yalnizca QUAD4 topolojisini kullanir.")
            return
        end if

        call evaluate_shape_functions(topology_id, natural_coordinates, point%shape, &
                                      point%dshape_dnatural, status)
        if (.not. status%is_ok()) return

        allocate(point%physical_coordinate(size(node_coordinates,1)))
        point%physical_coordinate = matmul(node_coordinates, point%shape)

        select case (topology_id)
        case (TOPOLOGY_BAR2)
            call compute_embedded_line_metric(node_coordinates, point%dshape_dnatural, &
                point%tangent, point%det_jacobian, point%direction, dshape_ds, status)
            if (.not. status%is_ok()) return

            allocate(point%jacobian(1,1), point%inverse_jacobian(1,1))
            allocate(point%dshape_dphysical(1,nn))
            point%jacobian(1,1) = point%det_jacobian
            point%inverse_jacobian(1,1) = 1.0_rk / point%det_jacobian
            point%dshape_dphysical(1,:) = dshape_ds
            point%integration_measure = quadrature_weight * point%det_jacobian

        case (TOPOLOGY_QUAD4, TOPOLOGY_HEX8)
            if (size(node_coordinates,1) /= dim) then
                call status%set_error(FEM_STATUS_SIZE_MISMATCH, &
                    "QUAD4/HEX8 square mapping physical ve natural boyutlari esit olmali.")
                return
            end if
            call compute_square_jacobian(node_coordinates, point%dshape_dnatural, &
                point%jacobian, point%inverse_jacobian, point%det_jacobian, status)
            if (.not. status%is_ok()) return
            if (point%det_jacobian <= 0.0_rk) then
                call status%set_error(FEM_STATUS_NUMERICAL_FAILURE, &
                    "Element Jacobian determinant pozitif degil; inverted/dejenere element.")
                return
            end if
            call map_shape_gradients(point%dshape_dnatural, point%inverse_jacobian, &
                                     point%dshape_dphysical, status)
            if (.not. status%is_ok()) return

            point%integration_measure = quadrature_weight * point%det_jacobian
            if (use_axisymmetric) then
                point%radius = point%physical_coordinate(1)
                if (point%radius <= 0.0_rk) then
                    call status%set_error(FEM_STATUS_NUMERICAL_FAILURE, &
                        "Axisymmetric integration noktasi pozitif radius gerektirir.")
                    return
                end if
                point%integration_measure = point%integration_measure * 2.0_rk * FEM_PI * point%radius
            end if
        end select
    end subroutine evaluate_geometry_point

    subroutine evaluate_element_integration_geometry(element_id, topology_id, node_coordinates, &
                                                     result, status, axisymmetric)
        integer(id_kind), intent(in) :: element_id, topology_id
        real(rk), intent(in) :: node_coordinates(:, :)
        type(element_result_t), intent(inout) :: result
        type(status_t), intent(out) :: status
        logical, intent(in), optional :: axisymmetric
        type(quadrature_rule_t) :: rule
        type(element_geometry_point_t) :: point
        logical :: use_axisymmetric
        integer :: p

        call status%clear()
        use_axisymmetric = .false.
        if (present(axisymmetric)) use_axisymmetric = axisymmetric

        call standard_quadrature_rule(topology_id, rule, status)
        if (.not. status%is_ok()) return
        call result%initialize(element_id, size(node_coordinates,1), rule%point_count, status)
        if (.not. status%is_ok()) return

        do p = 1, rule%point_count
            call evaluate_geometry_point(topology_id, node_coordinates, rule%points(:,p), &
                rule%weights(p), point, status, use_axisymmetric)
            if (.not. status%is_ok()) return
            result%physical_points(:,p) = point%physical_coordinate
            result%det_jacobian(p) = point%det_jacobian
            result%integration_measure(p) = point%integration_measure
        end do
    end subroutine evaluate_element_integration_geometry

    subroutine assess_element_quality(topology_id, node_coordinates, quality, status)
        integer(id_kind), intent(in) :: topology_id
        real(rk), intent(in) :: node_coordinates(:, :)
        type(element_quality_t), intent(out) :: quality
        type(status_t), intent(out) :: status
        type(quadrature_rule_t) :: rule
        real(rk), allocatable :: shape(:), dshape(:,:), jac(:,:), inv_jac(:,:)
        real(rk), allocatable :: tangent(:), direction(:), dshape_ds(:)
        real(rk) :: detj
        real(rk), allocatable :: determinants(:)
        integer :: p, dim, nn

        call status%clear()
        quality = element_quality_t()
        dim = reference_dimension(topology_id)
        nn = reference_node_count(topology_id)
        if (dim == 0 .or. size(node_coordinates,2) /= nn) then
            call status%set_error(FEM_STATUS_INVALID_ARGUMENT, "Element quality topology/coordinate girdisi gecersiz.")
            return
        end if

        call standard_quadrature_rule(topology_id, rule, status)
        if (.not. status%is_ok()) return
        allocate(determinants(rule%point_count))

        do p = 1, rule%point_count
            call evaluate_shape_functions(topology_id, rule%points(:,p), shape, dshape, status)
            if (.not. status%is_ok()) return

            if (topology_id == TOPOLOGY_BAR2) then
                call compute_embedded_line_metric(node_coordinates, dshape, tangent, detj, &
                    direction, dshape_ds, status)
                if (.not. status%is_ok()) then
                    quality%is_degenerate = .true.
                    return
                end if
            else
                if (size(node_coordinates,1) /= dim) then
                    call status%set_error(FEM_STATUS_SIZE_MISMATCH, &
                        "Quality square mapping physical ve natural boyutlari esit olmali.")
                    return
                end if
                call compute_square_jacobian(node_coordinates, dshape, jac, inv_jac, detj, status)
                if (.not. status%is_ok()) then
                    quality%is_degenerate = .true.
                    return
                end if
            end if
            determinants(p) = detj
        end do

        quality%min_det_jacobian = minval(determinants)
        quality%max_det_jacobian = maxval(determinants)
        quality%is_degenerate = .false.
        if (topology_id /= TOPOLOGY_BAR2) then
            quality%is_inverted = any(determinants < 0.0_rk)
        end if

        if (maxval(abs(determinants)) > 0.0_rk) then
            quality%jacobian_ratio = minval(abs(determinants)) / maxval(abs(determinants))
        end if

        if (quality%is_degenerate) then
            call status%set_error(FEM_STATUS_NUMERICAL_FAILURE, "Element quality: dejenere Jacobian tespit edildi.")
        else if (quality%is_inverted) then
            call status%set_error(FEM_STATUS_NUMERICAL_FAILURE, "Element quality: inverted/orientation hatali element tespit edildi.")
        end if
    end subroutine assess_element_quality

end module fem_element_kernel
