module fem_sparse_matrix
    !! CSR sparse matrix storage ve element scatter islemleri.
    !!
    !! Global FEM matrisleri dense saklanmaz. Pattern bir kez graph katmaninda
    !! kurulur; sayisal assembly yalnizca values(:) dizisini gunceller.
    use fem_kinds, only : rk, id_kind, index_kind
    use fem_status, only : status_t, FEM_STATUS_INVALID_ARGUMENT, FEM_STATUS_SIZE_MISMATCH
    use fem_sparsity_graph, only : sparsity_graph_t
    implicit none
    private

    integer, parameter, public :: MATRIX_SYMMETRY_GENERAL = 0
    integer, parameter, public :: MATRIX_SYMMETRY_SYMMETRIC = 1
    integer, parameter, public :: MATRIX_DEFINITENESS_UNKNOWN = 0
    integer, parameter, public :: MATRIX_DEFINITENESS_SPD_EXPECTED = 1

    type, public :: matrix_properties_t
        integer :: symmetry = MATRIX_SYMMETRY_GENERAL
        integer :: definiteness = MATRIX_DEFINITENESS_UNKNOWN
    end type matrix_properties_t

    type, public :: csr_matrix_t
        integer(index_kind) :: row_count = 0_index_kind
        integer(index_kind) :: column_count = 0_index_kind
        integer(index_kind), allocatable :: row_ptr(:)
        integer(id_kind), allocatable :: col_ind(:)
        real(rk), allocatable :: values(:)
        type(matrix_properties_t) :: properties
    contains
        procedure :: clear => csr_matrix_clear
        procedure :: initialize_from_graph => csr_matrix_initialize_from_graph
        procedure :: zero => csr_matrix_zero
        procedure :: nnz => csr_matrix_nnz
        procedure :: add_value => csr_matrix_add_value
        procedure :: value_at => csr_matrix_value_at
        procedure :: matvec => csr_matrix_matvec
        procedure :: to_dense => csr_matrix_to_dense
        procedure :: is_symmetric => csr_matrix_is_symmetric
    end type csr_matrix_t

contains

    subroutine csr_matrix_clear(this)
        class(csr_matrix_t), intent(inout) :: this
        this%row_count = 0_index_kind
        this%column_count = 0_index_kind
        if (allocated(this%row_ptr)) deallocate(this%row_ptr)
        if (allocated(this%col_ind)) deallocate(this%col_ind)
        if (allocated(this%values)) deallocate(this%values)
        this%properties = matrix_properties_t()
    end subroutine csr_matrix_clear

    subroutine csr_matrix_initialize_from_graph(this, graph, properties, status)
        class(csr_matrix_t), intent(inout) :: this
        type(sparsity_graph_t), intent(in) :: graph
        type(matrix_properties_t), intent(in), optional :: properties
        type(status_t), intent(out) :: status

        call status%clear()
        call this%clear()
        if (.not. allocated(graph%row_ptr) .or. .not. allocated(graph%col_ind)) then
            call status%set_error(FEM_STATUS_INVALID_ARGUMENT, "CSR matrix icin sparsity graph initialize edilmemis.")
            return
        end if
        this%row_count = graph%equation_count
        this%column_count = graph%equation_count
        allocate(this%row_ptr(size(graph%row_ptr)))
        allocate(this%col_ind(size(graph%col_ind)))
        allocate(this%values(size(graph%col_ind)))
        this%row_ptr = graph%row_ptr
        this%col_ind = graph%col_ind
        this%values = 0.0_rk
        if (present(properties)) this%properties = properties
    end subroutine csr_matrix_initialize_from_graph

    subroutine csr_matrix_zero(this)
        class(csr_matrix_t), intent(inout) :: this
        if (allocated(this%values)) this%values = 0.0_rk
    end subroutine csr_matrix_zero

    pure integer(index_kind) function csr_matrix_nnz(this)
        class(csr_matrix_t), intent(in) :: this
        if (allocated(this%values)) then
            csr_matrix_nnz = int(size(this%values), index_kind)
        else
            csr_matrix_nnz = 0_index_kind
        end if
    end function csr_matrix_nnz

    subroutine csr_matrix_add_value(this, row_eq, col_eq, value, status)
        class(csr_matrix_t), intent(inout) :: this
        integer(id_kind), intent(in) :: row_eq, col_eq
        real(rk), intent(in) :: value
        type(status_t), intent(out) :: status
        integer(index_kind) :: position

        call status%clear()
        position = find_position(this, row_eq, col_eq)
        if (position == 0_index_kind) then
            call status%set_error(FEM_STATUS_INVALID_ARGUMENT, "Assembly girdisi sparse pattern icinde bulunamadi.")
            return
        end if
        this%values(int(position)) = this%values(int(position)) + value
    end subroutine csr_matrix_add_value

    pure real(rk) function csr_matrix_value_at(this, row_eq, col_eq)
        class(csr_matrix_t), intent(in) :: this
        integer(id_kind), intent(in) :: row_eq, col_eq
        integer(index_kind) :: position

        position = find_position(this, row_eq, col_eq)
        if (position == 0_index_kind) then
            csr_matrix_value_at = 0.0_rk
        else
            csr_matrix_value_at = this%values(int(position))
        end if
    end function csr_matrix_value_at

    subroutine csr_matrix_matvec(this, x, y, status)
        class(csr_matrix_t), intent(in) :: this
        real(rk), intent(in) :: x(:)
        real(rk), allocatable, intent(out) :: y(:)
        type(status_t), intent(out) :: status
        integer :: row, p

        call status%clear()
        if (size(x) /= int(this%column_count)) then
            allocate(y(0))
            call status%set_error(FEM_STATUS_SIZE_MISMATCH, "CSR matvec vector boyutu global matrisle uyusmuyor.")
            return
        end if
        allocate(y(int(this%row_count)))
        y = 0.0_rk
        do row = 0, int(this%row_count) - 1
            do p = int(this%row_ptr(row + 1)), int(this%row_ptr(row + 2) - 1_index_kind)
                y(row + 1) = y(row + 1) + this%values(p) * x(int(this%col_ind(p)) + 1)
            end do
        end do
    end subroutine csr_matrix_matvec

    subroutine csr_matrix_to_dense(this, dense, status)
        class(csr_matrix_t), intent(in) :: this
        real(rk), allocatable, intent(out) :: dense(:, :)
        type(status_t), intent(out) :: status
        integer :: row, p

        call status%clear()
        allocate(dense(int(this%row_count), int(this%column_count)))
        dense = 0.0_rk
        do row = 0, int(this%row_count) - 1
            do p = int(this%row_ptr(row + 1)), int(this%row_ptr(row + 2) - 1_index_kind)
                dense(row + 1, int(this%col_ind(p)) + 1) = this%values(p)
            end do
        end do
    end subroutine csr_matrix_to_dense

    logical function csr_matrix_is_symmetric(this, tolerance)
        class(csr_matrix_t), intent(in) :: this
        real(rk), intent(in) :: tolerance
        integer :: row, p
        real(rk) :: aij, aji, scale

        csr_matrix_is_symmetric = .false.
        if (this%row_count /= this%column_count) return
        do row = 0, int(this%row_count) - 1
            do p = int(this%row_ptr(row + 1)), int(this%row_ptr(row + 2) - 1_index_kind)
                aij = this%values(p)
                aji = this%value_at(this%col_ind(p), int(row, id_kind))
                scale = max(1.0_rk, abs(aij), abs(aji))
                if (abs(aij - aji) > tolerance * scale) return
            end do
        end do
        csr_matrix_is_symmetric = .true.
    end function csr_matrix_is_symmetric

    pure integer(index_kind) function find_position(this, row_eq, col_eq)
        class(csr_matrix_t), intent(in) :: this
        integer(id_kind), intent(in) :: row_eq, col_eq
        integer(index_kind) :: first, last, mid

        find_position = 0_index_kind
        if (.not. allocated(this%row_ptr) .or. .not. allocated(this%col_ind)) return
        if (row_eq < 0_id_kind .or. row_eq >= int(this%row_count, id_kind)) return
        first = this%row_ptr(int(row_eq) + 1)
        last = this%row_ptr(int(row_eq) + 2) - 1_index_kind
        do while (first <= last)
            mid = (first + last) / 2_index_kind
            if (this%col_ind(int(mid)) == col_eq) then
                find_position = mid
                return
            else if (this%col_ind(int(mid)) < col_eq) then
                first = mid + 1_index_kind
            else
                last = mid - 1_index_kind
            end if
        end do
    end function find_position

end module fem_sparse_matrix
