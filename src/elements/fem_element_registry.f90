module fem_element_registry
    !! Topology ile formulation'i birbirinden ayiran element formulation registry'si.
    !!
    !! QUAD4 tek basina bir element formulation'i degildir. Plane-strain,
    !! plane-stress, axisymmetric ve ileride mixed u-p ayni QUAD4 topolojisini
    !! farkli formulation kimlikleriyle kullanabilir.
    use fem_kinds,    only : id_kind, index_kind
    use fem_ids,      only : INVALID_ID, id_is_valid
    use fem_topology, only : TOPOLOGY_BAR2, TOPOLOGY_QUAD4, TOPOLOGY_HEX8
    use fem_status,   only : status_t, FEM_STATUS_INVALID_ARGUMENT
    implicit none
    private

    integer(id_kind), parameter, public :: ELEMENT_TRUSS2          = 1101_id_kind
    integer(id_kind), parameter, public :: ELEMENT_PLANE_STRESS_QUAD4 = 2101_id_kind
    integer(id_kind), parameter, public :: ELEMENT_AXISYM_QUAD4       = 2102_id_kind
    integer(id_kind), parameter, public :: ELEMENT_PLANE_STRAIN_QUAD4 = 2104_id_kind
    integer(id_kind), parameter, public :: ELEMENT_SOLID_HEX8     = 3101_id_kind
    integer(id_kind), parameter, public :: ELEMENT_TOTAL_LAGRANGIAN_HEX8 = 3102_id_kind
    integer(id_kind), parameter, public :: ELEMENT_MIXED_UP_HEX8_P0 = 3103_id_kind
    integer(id_kind), parameter, public :: ELEMENT_BEAM2_LINEAR_2D = 1102_id_kind
    integer(id_kind), parameter, public :: ELEMENT_BEAM2_PROTOTYPE = ELEMENT_BEAM2_LINEAR_2D
    integer(id_kind), parameter, public :: ELEMENT_SHELL_QUAD4_PROTOTYPE = 2103_id_kind

    integer, parameter, public :: ELEMENT_STATE_PROTOTYPE = 1
    integer, parameter, public :: ELEMENT_STATE_KERNEL_READY = 2

    integer, parameter, public :: PRESSURE_INTERPOLATION_NONE = 0
    integer, parameter, public :: PRESSURE_INTERPOLATION_P0 = 1
    integer, parameter, public :: PRESSURE_INTERPOLATION_Q1 = 2

    integer, parameter :: ELEMENT_NAME_LENGTH = 48

    type, public :: element_definition_t
        integer(id_kind) :: id = INVALID_ID
        character(len=ELEMENT_NAME_LENGTH) :: name = ""
        integer(id_kind) :: topology_id = INVALID_ID
        integer :: spatial_dimension = 0
        integer :: displacement_components = 0
        integer :: rotation_components = 0
        integer :: pressure_components = 0
        integer :: pressure_interpolation = PRESSURE_INTERPOLATION_NONE
        integer :: quadrature_order = 0
        integer :: implementation_state = ELEMENT_STATE_PROTOTYPE
        logical :: axisymmetric = .false.
        logical :: requires_section = .false.
    end type element_definition_t

    type, public :: element_registry_t
        type(element_definition_t), allocatable :: elements(:)
    contains
        procedure :: clear => element_registry_clear
        procedure :: count => element_registry_count
        procedure :: add => element_registry_add
        procedure :: find_position => element_registry_find_position
        procedure :: register_standard => element_registry_register_standard
    end type element_registry_t

contains

    subroutine element_registry_clear(this)
        class(element_registry_t), intent(inout) :: this
        if (allocated(this%elements)) deallocate(this%elements)
    end subroutine element_registry_clear

    pure integer(index_kind) function element_registry_count(this)
        class(element_registry_t), intent(in) :: this
        if (allocated(this%elements)) then
            element_registry_count = int(size(this%elements), index_kind)
        else
            element_registry_count = 0_index_kind
        end if
    end function element_registry_count

    pure integer(index_kind) function element_registry_find_position(this, element_id)
        class(element_registry_t), intent(in) :: this
        integer(id_kind), intent(in) :: element_id
        integer :: i

        element_registry_find_position = 0_index_kind
        if (.not. allocated(this%elements)) return
        do i = 1, size(this%elements)
            if (this%elements(i)%id == element_id) then
                element_registry_find_position = int(i, index_kind)
                return
            end if
        end do
    end function element_registry_find_position

    subroutine element_registry_add(this, definition, status)
        class(element_registry_t), intent(inout) :: this
        type(element_definition_t), intent(in) :: definition
        type(status_t), intent(out) :: status
        type(element_definition_t), allocatable :: tmp(:)
        integer :: old_size

        call status%clear()
        if (.not. id_is_valid(definition%id) .or. .not. id_is_valid(definition%topology_id) .or. &
            len_trim(definition%name) == 0 .or. definition%spatial_dimension < 1 .or. &
            definition%spatial_dimension > 3 .or. definition%displacement_components < 1 .or. &
            definition%rotation_components < 0 .or. definition%pressure_components < 0 .or. &
            definition%pressure_interpolation < PRESSURE_INTERPOLATION_NONE .or. &
            definition%pressure_interpolation > PRESSURE_INTERPOLATION_Q1 .or. definition%quadrature_order < 1) then
            call status%set_error(FEM_STATUS_INVALID_ARGUMENT, "Element formulation tanimi gecersiz.")
            return
        end if
        if (this%find_position(definition%id) /= 0_index_kind) then
            call status%set_error(FEM_STATUS_INVALID_ARGUMENT, "Duplicate element formulation ID reddedildi.")
            return
        end if

        if (.not. allocated(this%elements)) then
            allocate(this%elements(1))
            old_size = 0
        else
            old_size = size(this%elements)
            allocate(tmp(old_size + 1))
            tmp(1:old_size) = this%elements
            call move_alloc(tmp, this%elements)
        end if
        this%elements(old_size + 1) = definition
    end subroutine element_registry_add

    subroutine element_registry_register_standard(this, status)
        class(element_registry_t), intent(inout) :: this
        type(status_t), intent(out) :: status
        type(element_definition_t) :: def

        call status%clear()

        def = element_definition_t(id=ELEMENT_TRUSS2, name="TRUSS2", &
            topology_id=TOPOLOGY_BAR2, spatial_dimension=3, displacement_components=3, rotation_components=0, &
            quadrature_order=2, implementation_state=ELEMENT_STATE_KERNEL_READY, axisymmetric=.false., requires_section=.true.)
        call this%add(def, status)
        if (.not. status%is_ok()) return

        def = element_definition_t(id=ELEMENT_PLANE_STRESS_QUAD4, name="PLANE_STRESS_QUAD4", &
            topology_id=TOPOLOGY_QUAD4, spatial_dimension=2, displacement_components=2, rotation_components=0, &
            quadrature_order=2, implementation_state=ELEMENT_STATE_KERNEL_READY, axisymmetric=.false., requires_section=.true.)
        call this%add(def, status)
        if (.not. status%is_ok()) return

        def = element_definition_t(id=ELEMENT_PLANE_STRAIN_QUAD4, name="PLANE_STRAIN_QUAD4", &
            topology_id=TOPOLOGY_QUAD4, spatial_dimension=2, displacement_components=2, rotation_components=0, &
            quadrature_order=2, implementation_state=ELEMENT_STATE_KERNEL_READY, axisymmetric=.false., requires_section=.false.)
        call this%add(def, status)
        if (.not. status%is_ok()) return

        def = element_definition_t(id=ELEMENT_AXISYM_QUAD4, name="AXISYM_QUAD4", &
            topology_id=TOPOLOGY_QUAD4, spatial_dimension=2, displacement_components=2, rotation_components=0, &
            quadrature_order=2, implementation_state=ELEMENT_STATE_KERNEL_READY, axisymmetric=.true., requires_section=.false.)
        call this%add(def, status)
        if (.not. status%is_ok()) return

        def = element_definition_t(id=ELEMENT_SOLID_HEX8, name="SOLID_HEX8", &
            topology_id=TOPOLOGY_HEX8, spatial_dimension=3, displacement_components=3, rotation_components=0, &
            quadrature_order=2, implementation_state=ELEMENT_STATE_KERNEL_READY, axisymmetric=.false., requires_section=.false.)
        call this%add(def, status)
        if (.not. status%is_ok()) return

        def = element_definition_t(id=ELEMENT_TOTAL_LAGRANGIAN_HEX8, name="TOTAL_LAGRANGIAN_HEX8", &
            topology_id=TOPOLOGY_HEX8, spatial_dimension=3, displacement_components=3, rotation_components=0, &
            quadrature_order=2, implementation_state=ELEMENT_STATE_KERNEL_READY, axisymmetric=.false., requires_section=.false.)
        call this%add(def, status)
        if (.not. status%is_ok()) return

        ! V0.10 baseline: trilinear displacement (Q1) + element-bazli sabit pressure (P0).
        ! Pressure DOF nodal degil, element-associated FIELD_ID_PRESSURE_P0 ile eslenir.
        def = element_definition_t(id=ELEMENT_MIXED_UP_HEX8_P0, name="MIXED_UP_HEX8_P0", &
            topology_id=TOPOLOGY_HEX8, spatial_dimension=3, displacement_components=3, rotation_components=0, &
            pressure_components=1, pressure_interpolation=PRESSURE_INTERPOLATION_P0, &
            quadrature_order=2, implementation_state=ELEMENT_STATE_KERNEL_READY, axisymmetric=.false., requires_section=.false.)
        call this%add(def, status)
        if (.not. status%is_ok()) return

        ! Beam V0.5 ile 2B Euler-Bernoulli olarak aktif olur. Shell prototipi V0.6'ya saklanir.
        def = element_definition_t(id=ELEMENT_BEAM2_LINEAR_2D, name="BEAM2_LINEAR_2D", &
            topology_id=TOPOLOGY_BAR2, spatial_dimension=2, displacement_components=2, &
            rotation_components=1, quadrature_order=2, implementation_state=ELEMENT_STATE_KERNEL_READY, &
            axisymmetric=.false., requires_section=.true.)
        call this%add(def, status)
        if (.not. status%is_ok()) return

        def = element_definition_t(id=ELEMENT_SHELL_QUAD4_PROTOTYPE, name="SHELL_QUAD4_PROTOTYPE", &
            topology_id=TOPOLOGY_QUAD4, spatial_dimension=3, displacement_components=3, &
            rotation_components=3, quadrature_order=2, implementation_state=ELEMENT_STATE_PROTOTYPE, &
            axisymmetric=.false., requires_section=.true.)
        call this%add(def, status)
    end subroutine element_registry_register_standard

end module fem_element_registry
