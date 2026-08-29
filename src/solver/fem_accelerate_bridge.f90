module fem_accelerate_bridge
    !! Apple Accelerate Sparse Solvers icin dar ISO_C_BINDING adapter'i.
    !! Vendor-specific API assembly ve element koduna sizmaz.
    use, intrinsic :: iso_c_binding, only : c_int, c_int32_t, c_int64_t, c_double
    use fem_kinds, only : rk
    use fem_status, only : status_t, FEM_STATUS_SIZE_MISMATCH, FEM_STATUS_NUMERICAL_FAILURE, &
                           FEM_STATUS_NOT_INITIALIZED
    use fem_sparse_matrix, only : csr_matrix_t
    implicit none
    private

    public :: accelerate_sparse_available, solve_accelerate_sparse_direct

    interface
        function c_accelerate_available() bind(C, name="fem_accelerate_sparse_available") result(value)
            import :: c_int
            integer(c_int) :: value
        end function c_accelerate_available

        function c_accelerate_solve(n, nnz, row_ptr, col_ind, values, rhs, solution) &
            bind(C, name="fem_accelerate_sparse_solve") result(code)
            import :: c_int, c_int32_t, c_int64_t, c_double
            integer(c_int32_t), value :: n
            integer(c_int64_t), value :: nnz
            integer(c_int64_t), intent(in) :: row_ptr(*)
            integer(c_int64_t), intent(in) :: col_ind(*)
            real(c_double), intent(in) :: values(*)
            real(c_double), intent(in) :: rhs(*)
            real(c_double), intent(out) :: solution(*)
            integer(c_int) :: code
        end function c_accelerate_solve
    end interface

contains

    logical function accelerate_sparse_available()
        accelerate_sparse_available = c_accelerate_available() /= 0_c_int
    end function accelerate_sparse_available

    subroutine solve_accelerate_sparse_direct(matrix, rhs, solution, status)
        type(csr_matrix_t), intent(in) :: matrix
        real(rk), intent(in) :: rhs(:)
        real(rk), allocatable, intent(out) :: solution(:)
        type(status_t), intent(out) :: status
        integer(c_int64_t), allocatable :: row_ptr_c(:), col_ind_c(:)
        real(c_double), allocatable :: values_c(:), rhs_c(:), solution_c(:)
        integer(c_int) :: code
        integer :: n

        call status%clear()
        n = int(matrix%row_count)
        if (matrix%row_count /= matrix%column_count .or. size(rhs) /= n) then
            allocate(solution(0))
            call status%set_error(FEM_STATUS_SIZE_MISMATCH, "Accelerate sparse solver kare matrix ve uyumlu RHS gerektirir.")
            return
        end if
        if (.not. accelerate_sparse_available()) then
            allocate(solution(0))
            call status%set_error(FEM_STATUS_NOT_INITIALIZED, "Apple Accelerate sparse backend bu platformda kullanilabilir degil.")
            return
        end if
        allocate(row_ptr_c(size(matrix%row_ptr)), col_ind_c(size(matrix%col_ind)))
        allocate(values_c(size(matrix%values)), rhs_c(n), solution_c(n))
        row_ptr_c = int(matrix%row_ptr, c_int64_t)
        col_ind_c = int(matrix%col_ind, c_int64_t)
        values_c = real(matrix%values, c_double)
        rhs_c = real(rhs, c_double)
        code = c_accelerate_solve(int(n, c_int32_t), int(size(values_c), c_int64_t), &
                                  row_ptr_c, col_ind_c, values_c, rhs_c, solution_c)
        if (code /= 0_c_int) then
            allocate(solution(0))
            call status%set_error(FEM_STATUS_NUMERICAL_FAILURE, "Apple Accelerate sparse direct solve basarisiz.")
            return
        end if
        allocate(solution(n))
        solution = real(solution_c, rk)
    end subroutine solve_accelerate_sparse_direct

end module fem_accelerate_bridge
