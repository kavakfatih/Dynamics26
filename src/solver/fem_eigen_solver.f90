module fem_eigen_solver
    !! Backend-bagimsiz generalized symmetric eigensolver facade.
    use fem_kinds,only:rk
    use fem_status,only:status_t,FEM_STATUS_INVALID_ARGUMENT
    use fem_dense_eigen_solver,only:solve_dense_generalized_eigen
    use fem_arpack_eigen_bridge,only:arpack_eigen_available,solve_arpack_generalized_eigen
    use fem_accelerate_eigen_bridge,only:accelerate_eigen_available,solve_accelerate_generalized_eigen
    implicit none; private
    integer,parameter,public :: EIGEN_SOLVER_DENSE_REFERENCE=1
    integer,parameter,public :: EIGEN_SOLVER_ARPACK_NG=2
    integer,parameter,public :: EIGEN_SOLVER_ACCELERATE_LAPACK=3
    type,public :: eigen_solver_options_t
        integer :: backend=EIGEN_SOLVER_DENSE_REFERENCE
        integer :: requested_modes=6
    end type
    public :: solve_generalized_eigen,eigen_solver_backend_available
contains
    subroutine solve_generalized_eigen(k,m,options,values,modes,status)
        real(rk),intent(in)::k(:,:),m(:,:); type(eigen_solver_options_t),intent(in)::options
        real(rk),allocatable,intent(out)::values(:),modes(:,:); type(status_t),intent(out)::status
        select case(options%backend)
        case(EIGEN_SOLVER_DENSE_REFERENCE); call solve_dense_generalized_eigen(k,m,options%requested_modes,values,modes,status)
        case(EIGEN_SOLVER_ARPACK_NG); call solve_arpack_generalized_eigen(k,m,options%requested_modes,values,modes,status)
        case(EIGEN_SOLVER_ACCELERATE_LAPACK); call solve_accelerate_generalized_eigen(k,m,options%requested_modes,values,modes,status)
        case default
            allocate(values(0),modes(0,0)); call status%set_error(FEM_STATUS_INVALID_ARGUMENT,"Bilinmeyen generalized eigensolver backend ID.")
        end select
    end subroutine
    logical function eigen_solver_backend_available(backend)
        integer,intent(in)::backend
        select case(backend)
        case(EIGEN_SOLVER_DENSE_REFERENCE); eigen_solver_backend_available=.true.
        case(EIGEN_SOLVER_ARPACK_NG); eigen_solver_backend_available=arpack_eigen_available()
        case(EIGEN_SOLVER_ACCELERATE_LAPACK); eigen_solver_backend_available=accelerate_eigen_available()
        case default; eigen_solver_backend_available=.false.
        end select
    end function
end module fem_eigen_solver
