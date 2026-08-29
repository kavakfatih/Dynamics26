module fem_accelerate_eigen_bridge
    use fem_kinds,only:rk
    use fem_status,only:status_t,FEM_STATUS_NOT_INITIALIZED
    implicit none; private
    public :: accelerate_eigen_available,solve_accelerate_generalized_eigen
contains
    logical function accelerate_eigen_available(); accelerate_eigen_available=.false.; end function
    subroutine solve_accelerate_generalized_eigen(k,m,requested_modes,values,modes,status)
        real(rk),intent(in)::k(:,:),m(:,:); integer,intent(in)::requested_modes
        real(rk),allocatable,intent(out)::values(:),modes(:,:); type(status_t),intent(out)::status
        if (size(k,1)<0 .or. size(m,1)<0 .or. requested_modes<0) error stop "unreachable"
        allocate(values(0),modes(0,0)); call status%set_error(FEM_STATUS_NOT_INITIALIZED,"Accelerate LAPACK eigen backend macOS disinda kullanilamaz.")
    end subroutine
end module fem_accelerate_eigen_bridge
