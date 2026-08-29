module fem_coordinate_frames
    !! Yerel koordinat sistemi metadata'si.
    !!
    !! axes(:,1:3), global koordinatlarda ifade edilen yerel e1/e2/e3 birim
    !! vektorleridir. Kabul edilen frame ortonormal ve sag-el (det > 0) olmak
    !! zorundadir. V0.2.0'da frame yalnizca veri/validasyon katmanidir.
    use fem_kinds,      only : rk, id_kind, index_kind
    use fem_ids,        only : INVALID_ID, id_is_valid
    use fem_status,     only : status_t, FEM_STATUS_INVALID_ARGUMENT
    use fem_tolerances, only : tolerance_set_t
    implicit none
    private

    type, public :: coordinate_frame_t
        integer(id_kind) :: id = INVALID_ID
        real(rk) :: origin(3) = 0.0_rk
        real(rk) :: axes(3,3) = reshape([ &
            1.0_rk, 0.0_rk, 0.0_rk, &
            0.0_rk, 1.0_rk, 0.0_rk, &
            0.0_rk, 0.0_rk, 1.0_rk ], [3,3])
    contains
        procedure :: set => coordinate_frame_set
        procedure :: is_orthonormal => coordinate_frame_is_orthonormal
    end type coordinate_frame_t

    type, public :: coordinate_frame_registry_t
        type(coordinate_frame_t), allocatable :: frames(:)
    contains
        procedure :: clear => frame_registry_clear
        procedure :: count => frame_registry_count
        procedure :: add => frame_registry_add
        procedure :: find_position => frame_registry_find_position
    end type coordinate_frame_registry_t

contains

    subroutine frame_registry_clear(this)
        class(coordinate_frame_registry_t), intent(inout) :: this
        if (allocated(this%frames)) deallocate(this%frames)
    end subroutine frame_registry_clear

    pure integer(index_kind) function frame_registry_count(this)
        class(coordinate_frame_registry_t), intent(in) :: this
        if (allocated(this%frames)) then
            frame_registry_count = int(size(this%frames), index_kind)
        else
            frame_registry_count = 0_index_kind
        end if
    end function frame_registry_count

    pure integer(index_kind) function frame_registry_find_position(this, frame_id)
        class(coordinate_frame_registry_t), intent(in) :: this
        integer(id_kind), intent(in) :: frame_id
        integer :: i

        frame_registry_find_position = 0_index_kind
        if (.not. allocated(this%frames)) return
        do i = 1, size(this%frames)
            if (this%frames(i)%id == frame_id) then
                frame_registry_find_position = int(i, index_kind)
                return
            end if
        end do
    end function frame_registry_find_position

    subroutine frame_registry_add(this, frame_id, origin, axes, status, tolerances)
        class(coordinate_frame_registry_t), intent(inout) :: this
        integer(id_kind), intent(in) :: frame_id
        real(rk), intent(in) :: origin(3), axes(3,3)
        type(status_t), intent(out) :: status
        type(tolerance_set_t), intent(in), optional :: tolerances
        type(coordinate_frame_t) :: frame
        type(coordinate_frame_t), allocatable :: tmp(:)
        integer :: old_size

        call status%clear()
        if (this%find_position(frame_id) /= 0_index_kind) then
            call status%set_error(FEM_STATUS_INVALID_ARGUMENT, "Ayni coordinate frame ID ikinci kez kaydedilemez.")
            return
        end if
        if (present(tolerances)) then
            call frame%set(frame_id, origin, axes, status, tolerances)
        else
            call frame%set(frame_id, origin, axes, status)
        end if
        if (.not. status%is_ok()) return

        if (.not. allocated(this%frames)) then
            allocate(this%frames(1))
            old_size = 0
        else
            old_size = size(this%frames)
            allocate(tmp(old_size + 1))
            tmp(1:old_size) = this%frames
            call move_alloc(tmp, this%frames)
        end if
        this%frames(old_size + 1) = frame
    end subroutine frame_registry_add

    subroutine coordinate_frame_set(this, frame_id, origin, axes, status, tolerances)
        class(coordinate_frame_t), intent(inout) :: this
        integer(id_kind), intent(in) :: frame_id
        real(rk), intent(in) :: origin(3), axes(3,3)
        type(status_t), intent(out) :: status
        type(tolerance_set_t), intent(in), optional :: tolerances
        type(tolerance_set_t) :: tol

        call status%clear()
        if (present(tolerances)) tol = tolerances
        if (.not. id_is_valid(frame_id)) then
            call status%set_error(FEM_STATUS_INVALID_ARGUMENT, "Coordinate frame ID gecersiz.")
            return
        end if
        if (.not. frame_axes_valid(axes, tol%geometry)) then
            call status%set_error(FEM_STATUS_INVALID_ARGUMENT, "Coordinate frame eksenleri ortonormal ve sag-el olmali.")
            return
        end if

        this%id = frame_id
        this%origin = origin
        this%axes = axes
    end subroutine coordinate_frame_set

    pure logical function coordinate_frame_is_orthonormal(this, tolerance)
        class(coordinate_frame_t), intent(in) :: this
        real(rk), intent(in), optional :: tolerance
        real(rk) :: tol

        tol = 1.0e-10_rk
        if (present(tolerance)) tol = tolerance
        coordinate_frame_is_orthonormal = frame_axes_valid(this%axes, tol)
    end function coordinate_frame_is_orthonormal

    pure logical function frame_axes_valid(axes, tolerance)
        real(rk), intent(in) :: axes(3,3), tolerance
        real(rk) :: gram(3,3), determinant
        integer :: i, j

        gram = matmul(transpose(axes), axes)
        frame_axes_valid = .true.
        do j = 1, 3
            do i = 1, 3
                if (i == j) then
                    if (abs(gram(i,j) - 1.0_rk) > tolerance) frame_axes_valid = .false.
                else
                    if (abs(gram(i,j)) > tolerance) frame_axes_valid = .false.
                end if
            end do
        end do

        determinant = axes(1,1) * (axes(2,2)*axes(3,3) - axes(2,3)*axes(3,2)) &
                    - axes(1,2) * (axes(2,1)*axes(3,3) - axes(2,3)*axes(3,1)) &
                    + axes(1,3) * (axes(2,1)*axes(3,2) - axes(2,2)*axes(3,1))
        if (determinant <= tolerance) frame_axes_valid = .false.
    end function frame_axes_valid

end module fem_coordinate_frames
