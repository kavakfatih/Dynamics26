module fem_sparsity_graph
    !! Global sparse pattern'i element equation baglantilarindan olusturur.
    !!
    !! Pattern CSR (Compressed Sparse Row) semantigi tasir:
    !!   row_ptr(1) = 1
    !!   row_ptr(n+1) = nnz + 1
    !!   col_ind(:) = 0-tabanli equation ID
    !!
    !! Graph kurulumunda N x N dense adjacency matrisi kullanilmaz. Elementlerden
    !! gelen (row,column) ciftleri toplanir, siralanir ve unique hale getirilir.
    use fem_kinds, only : id_kind, index_kind
    use fem_ids, only : INVALID_ID
    use fem_status, only : status_t, FEM_STATUS_INVALID_ARGUMENT
    use fem_element_dof_map, only : element_dof_map_t
    implicit none
    private

    type, public :: sparsity_graph_t
        integer(index_kind) :: equation_count = 0_index_kind
        integer(index_kind), allocatable :: row_ptr(:)
        integer(id_kind), allocatable :: col_ind(:)
    contains
        procedure :: clear => sparsity_graph_clear
        procedure :: nnz => sparsity_graph_nnz
        procedure :: build => sparsity_graph_build
        procedure :: contains => sparsity_graph_contains
    end type sparsity_graph_t

contains

    subroutine sparsity_graph_clear(this)
        class(sparsity_graph_t), intent(inout) :: this
        this%equation_count = 0_index_kind
        if (allocated(this%row_ptr)) deallocate(this%row_ptr)
        if (allocated(this%col_ind)) deallocate(this%col_ind)
    end subroutine sparsity_graph_clear

    pure integer(index_kind) function sparsity_graph_nnz(this)
        class(sparsity_graph_t), intent(in) :: this
        if (allocated(this%col_ind)) then
            sparsity_graph_nnz = int(size(this%col_ind), index_kind)
        else
            sparsity_graph_nnz = 0_index_kind
        end if
    end function sparsity_graph_nnz

    subroutine sparsity_graph_build(this, equation_count, maps, status)
        class(sparsity_graph_t), intent(inout) :: this
        integer(index_kind), intent(in) :: equation_count
        type(element_dof_map_t), intent(in) :: maps(:)
        type(status_t), intent(out) :: status
        integer(id_kind), allocatable :: rows(:), cols(:), unique_rows(:), unique_cols(:)
        integer(index_kind) :: capacity, used, unique_count
        integer :: m, a, b, i
        integer(id_kind) :: row_eq, col_eq

        call status%clear()
        call this%clear()
        if (equation_count < 0_index_kind) then
            call status%set_error(FEM_STATUS_INVALID_ARGUMENT, "Equation count negatif olamaz.")
            return
        end if
        this%equation_count = equation_count
        allocate(this%row_ptr(int(equation_count) + 1))
        this%row_ptr = 1_index_kind
        if (equation_count == 0_index_kind) then
            allocate(this%col_ind(0))
            return
        end if

        capacity = equation_count
        do m = 1, size(maps)
            if (allocated(maps(m)%equation_ids)) then
                capacity = capacity + int(size(maps(m)%equation_ids), index_kind)**2
            end if
        end do
        allocate(rows(int(capacity)), cols(int(capacity)))
        used = 0_index_kind

        ! Her aktif equation icin diagonal structurally mevcut tutulur. Bu,
        ! element baglantisi olmayan bir DOF'un singularligini pattern asamasinda
        ! gizlemek yerine solver asamasinda acikca yakalamamizi saglar.
        do i = 0, int(equation_count) - 1
            used = used + 1_index_kind
            rows(int(used)) = int(i, id_kind)
            cols(int(used)) = int(i, id_kind)
        end do

        do m = 1, size(maps)
            if (.not. allocated(maps(m)%equation_ids)) cycle
            do a = 1, size(maps(m)%equation_ids)
                row_eq = maps(m)%equation_ids(a)
                if (row_eq == INVALID_ID) cycle
                if (row_eq < 0_id_kind .or. row_eq >= int(equation_count, id_kind)) then
                    call this%clear()
                    call status%set_error(FEM_STATUS_INVALID_ARGUMENT, "Element map row equation ID global aralik disinda.")
                    return
                end if
                do b = 1, size(maps(m)%equation_ids)
                    col_eq = maps(m)%equation_ids(b)
                    if (col_eq == INVALID_ID) cycle
                    if (col_eq < 0_id_kind .or. col_eq >= int(equation_count, id_kind)) then
                        call this%clear()
                        call status%set_error(FEM_STATUS_INVALID_ARGUMENT, "Element map column equation ID global aralik disinda.")
                        return
                    end if
                    used = used + 1_index_kind
                    rows(int(used)) = row_eq
                    cols(int(used)) = col_eq
                end do
            end do
        end do

        call sort_pairs(rows, cols, 1, int(used))
        allocate(unique_rows(int(used)), unique_cols(int(used)))
        unique_count = 0_index_kind
        do i = 1, int(used)
            if (i == 1) then
                unique_count = unique_count + 1_index_kind
                unique_rows(int(unique_count)) = rows(i)
                unique_cols(int(unique_count)) = cols(i)
            else if (rows(i) /= rows(i-1) .or. cols(i) /= cols(i-1)) then
                unique_count = unique_count + 1_index_kind
                unique_rows(int(unique_count)) = rows(i)
                unique_cols(int(unique_count)) = cols(i)
            end if
        end do

        allocate(this%col_ind(int(unique_count)))
        this%col_ind = unique_cols(1:int(unique_count))
        this%row_ptr = 1_index_kind
        i = 1
        do m = 0, int(equation_count) - 1
            this%row_ptr(m + 1) = int(i, index_kind)
            do while (i <= int(unique_count))
                if (unique_rows(i) /= int(m, id_kind)) exit
                i = i + 1
            end do
        end do
        this%row_ptr(int(equation_count) + 1) = unique_count + 1_index_kind
    end subroutine sparsity_graph_build

    pure logical function sparsity_graph_contains(this, row_eq, col_eq)
        class(sparsity_graph_t), intent(in) :: this
        integer(id_kind), intent(in) :: row_eq, col_eq
        integer(index_kind) :: first, last, mid

        sparsity_graph_contains = .false.
        if (.not. allocated(this%row_ptr) .or. .not. allocated(this%col_ind)) return
        if (row_eq < 0_id_kind .or. row_eq >= int(this%equation_count, id_kind)) return
        first = this%row_ptr(int(row_eq) + 1)
        last = this%row_ptr(int(row_eq) + 2) - 1_index_kind
        do while (first <= last)
            mid = (first + last) / 2_index_kind
            if (this%col_ind(int(mid)) == col_eq) then
                sparsity_graph_contains = .true.
                return
            else if (this%col_ind(int(mid)) < col_eq) then
                first = mid + 1_index_kind
            else
                last = mid - 1_index_kind
            end if
        end do
    end function sparsity_graph_contains

    recursive subroutine sort_pairs(rows, cols, left, right)
        integer(id_kind), intent(inout) :: rows(:), cols(:)
        integer, intent(in) :: left, right
        integer :: i, j
        integer(id_kind) :: pivot_row, pivot_col, tmp

        if (left >= right) return
        i = left
        j = right
        pivot_row = rows((left + right) / 2)
        pivot_col = cols((left + right) / 2)

        do
            do while (pair_less(rows(i), cols(i), pivot_row, pivot_col))
                i = i + 1
            end do
            do while (pair_less(pivot_row, pivot_col, rows(j), cols(j)))
                j = j - 1
            end do
            if (i <= j) then
                tmp = rows(i); rows(i) = rows(j); rows(j) = tmp
                tmp = cols(i); cols(i) = cols(j); cols(j) = tmp
                i = i + 1
                j = j - 1
            end if
            if (i > j) exit
        end do
        if (left < j) call sort_pairs(rows, cols, left, j)
        if (i < right) call sort_pairs(rows, cols, i, right)
    end subroutine sort_pairs

    pure logical function pair_less(row_a, col_a, row_b, col_b)
        integer(id_kind), intent(in) :: row_a, col_a, row_b, col_b
        pair_less = row_a < row_b .or. (row_a == row_b .and. col_a < col_b)
    end function pair_less

end module fem_sparsity_graph
