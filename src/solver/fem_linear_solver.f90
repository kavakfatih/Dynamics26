module fem_linear_solver
    !! Backend-bagimsiz global linear solver facade'i.
    !!
    !! Assembly katmani sadece CSR matrix + RHS uretir. Hangi backend'in
    !! kullanildigi bu modulde secilir; element/assembly kodu Accelerate API'sini
    !! veya iterative solver ayrintilarini bilmez.
    use fem_kinds, only : rk
    use fem_status, only : status_t, FEM_STATUS_INVALID_ARGUMENT
    use fem_sparse_matrix, only : csr_matrix_t
    use fem_dense_reference_solver, only : solve_dense_reference
    use fem_sparse_cg_solver, only : cg_options_t, cg_statistics_t, solve_sparse_cg
    use fem_accelerate_bridge, only : accelerate_sparse_available, solve_accelerate_sparse_direct
    implicit none
    private

    integer, parameter, public :: LINEAR_SOLVER_DENSE_REFERENCE = 1
    integer, parameter, public :: LINEAR_SOLVER_SPARSE_CG = 2
    integer, parameter, public :: LINEAR_SOLVER_ACCELERATE_SPARSE_DIRECT = 3

    type, public :: linear_solver_options_t
        integer :: backend = LINEAR_SOLVER_DENSE_REFERENCE
        type(cg_options_t) :: cg
    end type linear_solver_options_t

    type, public :: linear_solver_statistics_t
        integer :: backend_used = 0
        integer :: iterations = 0
        real(rk) :: residual_norm = 0.0_rk
        logical :: converged = .false.
    end type linear_solver_statistics_t

    public :: solve_linear_system, linear_solver_backend_available

contains

    subroutine solve_linear_system(matrix, rhs, solution, options, statistics, status)
        type(csr_matrix_t), intent(in) :: matrix
        real(rk), intent(in) :: rhs(:)
        real(rk), allocatable, intent(out) :: solution(:)
        type(linear_solver_options_t), intent(in) :: options
        type(linear_solver_statistics_t), intent(out) :: statistics
        type(status_t), intent(out) :: status
        type(cg_statistics_t) :: cg_stats
        real(rk), allocatable :: ax(:)

        call status%clear()
        statistics = linear_solver_statistics_t()
        statistics%backend_used = options%backend

        select case (options%backend)
        case (LINEAR_SOLVER_DENSE_REFERENCE)
            call solve_dense_reference(matrix, rhs, solution, status)
            if (.not. status%is_ok()) return
            call matrix%matvec(solution, ax, status)
            if (.not. status%is_ok()) return
            statistics%residual_norm = sqrt(dot_product(ax-rhs, ax-rhs))
            statistics%converged = .true.
        case (LINEAR_SOLVER_SPARSE_CG)
            call solve_sparse_cg(matrix, rhs, solution, options%cg, cg_stats, status)
            statistics%iterations = cg_stats%iterations
            statistics%residual_norm = cg_stats%residual_norm
            statistics%converged = cg_stats%converged
        case (LINEAR_SOLVER_ACCELERATE_SPARSE_DIRECT)
            call solve_accelerate_sparse_direct(matrix, rhs, solution, status)
            if (.not. status%is_ok()) return
            call matrix%matvec(solution, ax, status)
            if (.not. status%is_ok()) return
            statistics%residual_norm = sqrt(dot_product(ax-rhs, ax-rhs))
            statistics%converged = .true.
        case default
            allocate(solution(0))
            call status%set_error(FEM_STATUS_INVALID_ARGUMENT, "Bilinmeyen LinearSolver backend ID.")
        end select
    end subroutine solve_linear_system

    logical function linear_solver_backend_available(backend)
        integer, intent(in) :: backend
        select case (backend)
        case (LINEAR_SOLVER_DENSE_REFERENCE, LINEAR_SOLVER_SPARSE_CG)
            linear_solver_backend_available = .true.
        case (LINEAR_SOLVER_ACCELERATE_SPARSE_DIRECT)
            linear_solver_backend_available = accelerate_sparse_available()
        case default
            linear_solver_backend_available = .false.
        end select
    end function linear_solver_backend_available

end module fem_linear_solver
