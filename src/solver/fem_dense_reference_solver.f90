module fem_dense_reference_solver
    !! Kucuk sistemler ve verification icin bagimsiz dense referans solver.
    !! Partial pivoting'li Gaussian elimination kullanir. Production sparse
    !! backend'in dogrulama referansidir; buyuk modeller icin hedef backend degildir.
    use fem_kinds, only : rk
    use fem_status, only : status_t, FEM_STATUS_SIZE_MISMATCH, FEM_STATUS_NUMERICAL_FAILURE
    use fem_sparse_matrix, only : csr_matrix_t
    implicit none
    private

    public :: solve_dense_reference

contains

    subroutine solve_dense_reference(matrix, rhs, solution, status)
        type(csr_matrix_t), intent(in) :: matrix
        real(rk), intent(in) :: rhs(:)
        real(rk), allocatable, intent(out) :: solution(:)
        type(status_t), intent(out) :: status
        real(rk), allocatable :: a(:, :), b(:)
        real(rk) :: pivot_value, factor, scale, tmp
        real(rk), allocatable :: tmp_row(:)
        integer :: n, i, j, k, pivot

        call status%clear()
        n = int(matrix%row_count)
        if (matrix%row_count /= matrix%column_count .or. size(rhs) /= n) then
            allocate(solution(0))
            call status%set_error(FEM_STATUS_SIZE_MISMATCH, "Dense reference solver kare matrix ve uyumlu RHS gerektirir.")
            return
        end if
        if (n == 0) then
            allocate(solution(0))
            return
        end if
        call matrix%to_dense(a, status)
        if (.not. status%is_ok()) then
            allocate(solution(0))
            return
        end if
        allocate(b(n), solution(n), tmp_row(n))
        b = rhs
        solution = 0.0_rk

        do k = 1, n - 1
            pivot = k
            pivot_value = abs(a(k,k))
            do i = k + 1, n
                if (abs(a(i,k)) > pivot_value) then
                    pivot = i
                    pivot_value = abs(a(i,k))
                end if
            end do
            scale = max(1.0_rk, maxval(abs(a(k:n,k:n))))
            if (pivot_value <= 100.0_rk * epsilon(1.0_rk) * scale) then
                deallocate(solution)
                allocate(solution(0))
                call status%set_error(FEM_STATUS_NUMERICAL_FAILURE, "Dense reference solver singular/ill-conditioned pivot buldu.")
                return
            end if
            if (pivot /= k) then
                tmp_row = a(k,:)
                a(k,:) = a(pivot,:)
                a(pivot,:) = tmp_row
                tmp = b(k); b(k) = b(pivot); b(pivot) = tmp
            end if
            do i = k + 1, n
                factor = a(i,k) / a(k,k)
                a(i,k) = 0.0_rk
                do j = k + 1, n
                    a(i,j) = a(i,j) - factor * a(k,j)
                end do
                b(i) = b(i) - factor * b(k)
            end do
        end do

        scale = max(1.0_rk, maxval(abs(a)))
        if (abs(a(n,n)) <= 100.0_rk * epsilon(1.0_rk) * scale) then
            deallocate(solution)
            allocate(solution(0))
            call status%set_error(FEM_STATUS_NUMERICAL_FAILURE, "Dense reference solver son pivotta singular sistem buldu.")
            return
        end if
        do i = n, 1, -1
            tmp = b(i)
            do j = i + 1, n
                tmp = tmp - a(i,j) * solution(j)
            end do
            solution(i) = tmp / a(i,i)
        end do
    end subroutine solve_dense_reference

end module fem_dense_reference_solver
