module fem_sparse_cg_solver
    !! Symmetric positive-definite sparse sistemler icin Jacobi preconditioned
    !! Conjugate Gradient referans iterative solver'i.
    !!
    !! Bu backend production performans hedefi degil; LinearSolver arayuzunun
    !! sparse iterative semantigini ve matrix-free olmayan CSR akisinin testidir.
    use fem_kinds, only : rk, id_kind
    use fem_status, only : status_t, FEM_STATUS_INVALID_ARGUMENT, FEM_STATUS_SIZE_MISMATCH, &
                           FEM_STATUS_NUMERICAL_FAILURE
    use fem_sparse_matrix, only : csr_matrix_t
    implicit none
    private

    type, public :: cg_options_t
        integer :: max_iterations = 1000
        real(rk) :: relative_tolerance = 1.0e-10_rk
        real(rk) :: absolute_tolerance = 1.0e-12_rk
    end type cg_options_t

    type, public :: cg_statistics_t
        integer :: iterations = 0
        real(rk) :: residual_norm = huge(1.0_rk)
        logical :: converged = .false.
    end type cg_statistics_t

    public :: solve_sparse_cg

contains

    subroutine solve_sparse_cg(matrix, rhs, solution, options, statistics, status)
        type(csr_matrix_t), intent(in) :: matrix
        real(rk), intent(in) :: rhs(:)
        real(rk), allocatable, intent(out) :: solution(:)
        type(cg_options_t), intent(in) :: options
        type(cg_statistics_t), intent(out) :: statistics
        type(status_t), intent(out) :: status
        real(rk), allocatable :: r(:), z(:), p(:), ap(:), diagonal(:)
        real(rk) :: rz_old, rz_new, alpha, beta, denom, target, rhs_norm
        integer :: n, i, iter

        call status%clear()
        statistics = cg_statistics_t()
        n = int(matrix%row_count)
        if (matrix%row_count /= matrix%column_count .or. size(rhs) /= n) then
            allocate(solution(0))
            call status%set_error(FEM_STATUS_SIZE_MISMATCH, "CG kare matrix ve uyumlu RHS gerektirir.")
            return
        end if
        if (options%max_iterations < 1 .or. options%relative_tolerance <= 0.0_rk .or. &
            options%absolute_tolerance <= 0.0_rk) then
            allocate(solution(0))
            call status%set_error(FEM_STATUS_INVALID_ARGUMENT, "CG solver options gecersiz.")
            return
        end if
        allocate(solution(n), r(n), z(n), p(n), diagonal(n))
        solution = 0.0_rk
        r = rhs
        do i = 1, n
            diagonal(i) = matrix%value_at(int(i-1, id_kind), int(i-1, id_kind))
            if (diagonal(i) <= 0.0_rk) then
                deallocate(solution)
                allocate(solution(0))
                call status%set_error(FEM_STATUS_NUMERICAL_FAILURE, "CG/Jacobi pozitif diagonal gerektirir.")
                return
            end if
        end do
        z = r / diagonal
        p = z
        rz_old = dot_product(r, z)
        rhs_norm = sqrt(dot_product(rhs, rhs))
        target = max(options%absolute_tolerance, options%relative_tolerance * rhs_norm)
        statistics%residual_norm = sqrt(dot_product(r, r))
        if (statistics%residual_norm <= target) then
            statistics%converged = .true.
            return
        end if

        do iter = 1, options%max_iterations
            call matrix%matvec(p, ap, status)
            if (.not. status%is_ok()) return
            denom = dot_product(p, ap)
            if (denom <= 0.0_rk .or. abs(denom) <= tiny(1.0_rk)) then
                call status%set_error(FEM_STATUS_NUMERICAL_FAILURE, "CG SPD kosulu bozuldu veya denominator sifira yaklasti.")
                return
            end if
            alpha = rz_old / denom
            solution = solution + alpha * p
            r = r - alpha * ap
            statistics%iterations = iter
            statistics%residual_norm = sqrt(dot_product(r, r))
            if (statistics%residual_norm <= target) then
                statistics%converged = .true.
                return
            end if
            z = r / diagonal
            rz_new = dot_product(r, z)
            if (abs(rz_old) <= tiny(1.0_rk)) then
                call status%set_error(FEM_STATUS_NUMERICAL_FAILURE, "CG recurrence denominator sifira yaklasti.")
                return
            end if
            beta = rz_new / rz_old
            p = z + beta * p
            rz_old = rz_new
        end do
        call status%set_error(FEM_STATUS_NUMERICAL_FAILURE, "CG maksimum iterasyonda convergence saglayamadi.")
    end subroutine solve_sparse_cg

end module fem_sparse_cg_solver
