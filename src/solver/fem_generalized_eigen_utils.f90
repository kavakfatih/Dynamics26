module fem_generalized_eigen_utils
    !! Symmetric generalized eigenproblem K phi = lambda M phi icin ortak donusumlar.
    use fem_kinds, only : rk
    use fem_status, only : status_t, FEM_STATUS_INVALID_ARGUMENT, FEM_STATUS_NUMERICAL_FAILURE, FEM_STATUS_SIZE_MISMATCH
    implicit none
    private
    public :: generalized_to_standard, recover_generalized_modes
    public :: sort_eigenpairs, mass_normalize_modes, compute_modal_residuals, estimate_matrix_nullity
contains
    subroutine generalized_to_standard(k,m,a,linv,status)
        real(rk), intent(in) :: k(:,:),m(:,:)
        real(rk), allocatable, intent(out) :: a(:,:),linv(:,:)
        type(status_t), intent(out) :: status
        real(rk), allocatable :: l(:,:)
        integer :: n,i,j,p
        real(rk) :: sumv,scale
        call status%clear()
        n=size(k,1)
        if (n==0 .or. size(k,2)/=n .or. size(m,1)/=n .or. size(m,2)/=n) then
            allocate(a(0,0),linv(0,0)); call status%set_error(FEM_STATUS_SIZE_MISMATCH,"Generalized eigen K/M boyutlari square ve esit olmali."); return
        end if
        scale=max(1.0_rk,maxval(abs(m)))
        if (maxval(abs(m-transpose(m))) > 1.0e-10_rk*scale .or. &
            maxval(abs(k-transpose(k))) > 1.0e-10_rk*max(1.0_rk,maxval(abs(k)))) then
            allocate(a(0,0),linv(0,0)); call status%set_error(FEM_STATUS_INVALID_ARGUMENT,"Generalized eigen K ve M symmetric olmali."); return
        end if
        allocate(l(n,n)); l=0.0_rk
        do i=1,n
            do j=1,i
                sumv=m(i,j)
                do p=1,j-1
                    sumv=sumv-l(i,p)*l(j,p)
                end do
                if (i==j) then
                    if (sumv <= 1.0e-14_rk*scale) then
                        allocate(a(0,0),linv(0,0)); call status%set_error(FEM_STATUS_NUMERICAL_FAILURE,"Mass matrix SPD degil; Cholesky factorization basarisiz."); return
                    end if
                    l(i,j)=sqrt(sumv)
                else
                    l(i,j)=sumv/l(j,j)
                end if
            end do
        end do
        allocate(linv(n,n)); linv=0.0_rk
        do j=1,n
            do i=1,n
                if (i<j) cycle
                if (i==j) then
                    linv(i,j)=1.0_rk/l(i,i)
                else
                    sumv=0.0_rk
                    do p=j,i-1
                        sumv=sumv+l(i,p)*linv(p,j)
                    end do
                    linv(i,j)=-sumv/l(i,i)
                end if
            end do
        end do
        allocate(a(n,n))
        a=matmul(linv,matmul(k,transpose(linv)))
        a=0.5_rk*(a+transpose(a))
    end subroutine generalized_to_standard

    subroutine recover_generalized_modes(linv,standard_modes,m,modes,status)
        real(rk), intent(in) :: linv(:,:),standard_modes(:,:),m(:,:)
        real(rk), allocatable, intent(out) :: modes(:,:)
        type(status_t), intent(out) :: status
        call status%clear()
        if (size(linv,1)/=size(linv,2) .or. size(standard_modes,1)/=size(linv,1)) then
            allocate(modes(0,0)); call status%set_error(FEM_STATUS_SIZE_MISMATCH,"Mode recovery boyutlari uyusmuyor."); return
        end if
        allocate(modes(size(standard_modes,1),size(standard_modes,2)))
        modes=matmul(transpose(linv),standard_modes)
        call mass_normalize_modes(m,modes,status)
    end subroutine recover_generalized_modes

    subroutine mass_normalize_modes(m,modes,status)
        real(rk), intent(in) :: m(:,:)
        real(rk), intent(inout) :: modes(:,:)
        type(status_t), intent(out) :: status
        integer :: j,imax
        real(rk) :: normv
        call status%clear()
        if (size(m,1)/=size(m,2) .or. size(m,1)/=size(modes,1)) then
            call status%set_error(FEM_STATUS_SIZE_MISMATCH,"Mass normalization boyutlari uyusmuyor."); return
        end if
        do j=1,size(modes,2)
            normv=sqrt(dot_product(modes(:,j),matmul(m,modes(:,j))))
            if (normv<=sqrt(tiny(1.0_rk))) then
                call status%set_error(FEM_STATUS_NUMERICAL_FAILURE,"Mode shape mass norm sifir."); return
            end if
            modes(:,j)=modes(:,j)/normv
            imax=maxloc(abs(modes(:,j)),dim=1)
            if (modes(imax,j)<0.0_rk) modes(:,j)=-modes(:,j)
        end do
    end subroutine mass_normalize_modes

    subroutine sort_eigenpairs(values,vectors)
        real(rk), intent(inout) :: values(:),vectors(:,:)
        integer :: i,j,k
        real(rk) :: tmp
        real(rk), allocatable :: vtmp(:)
        allocate(vtmp(size(vectors,1)))
        do i=1,size(values)-1
            k=i
            do j=i+1,size(values)
                if (values(j)<values(k)) k=j
            end do
            if (k/=i) then
                tmp=values(i); values(i)=values(k); values(k)=tmp
                vtmp=vectors(:,i); vectors(:,i)=vectors(:,k); vectors(:,k)=vtmp
            end if
        end do
    end subroutine sort_eigenpairs

    integer function estimate_matrix_nullity(a) result(nullity)
        !! Symmetric stiffness nullity estimate. M SPD oldugunda generalized
        !! eigenproblemdeki zero-mode sayisi K nullity'sine esittir.
        real(rk), intent(in) :: a(:,:)
        real(rk), allocatable :: work(:,:),rowtmp(:)
        real(rk) :: scale,tol,pivot_value,factor
        integer :: n,row,col,pivot,rank
        n=size(a,1)
        if (n==0 .or. size(a,2)/=n) then
            nullity=0
            return
        end if
        allocate(work(n,n),rowtmp(n)); work=a
        scale=max(1.0_rk,maxval(abs(work)))
        tol=1.0e3_rk*epsilon(1.0_rk)*real(max(1,n),rk)*scale
        rank=0; row=1
        do col=1,n
            if (row>n) exit
            pivot=row-1+maxloc(abs(work(row:n,col)),dim=1)
            pivot_value=abs(work(pivot,col))
            if (pivot_value<=tol) cycle
            if (pivot/=row) then
                rowtmp=work(row,:); work(row,:)=work(pivot,:); work(pivot,:)=rowtmp
            end if
            do pivot=row+1,n
                if (abs(work(pivot,col))<=tol) cycle
                factor=work(pivot,col)/work(row,col)
                work(pivot,col:n)=work(pivot,col:n)-factor*work(row,col:n)
            end do
            rank=rank+1; row=row+1
        end do
        nullity=n-rank
    end function estimate_matrix_nullity

    subroutine compute_modal_residuals(k,m,values,modes,residuals,status)
        real(rk), intent(in) :: k(:,:),m(:,:),values(:),modes(:,:)
        real(rk), allocatable, intent(out) :: residuals(:)
        type(status_t), intent(out) :: status
        integer :: j
        real(rk), allocatable :: r(:)
        real(rk) :: denom
        call status%clear()
        if (size(values)/=size(modes,2) .or. size(k,1)/=size(modes,1) .or. size(m,1)/=size(modes,1)) then
            allocate(residuals(0)); call status%set_error(FEM_STATUS_SIZE_MISMATCH,"Modal residual boyutlari uyusmuyor."); return
        end if
        allocate(residuals(size(values)),r(size(modes,1)))
        do j=1,size(values)
            r=matmul(k,modes(:,j))-values(j)*matmul(m,modes(:,j))
            denom=max(1.0_rk,sqrt(dot_product(matmul(k,modes(:,j)),matmul(k,modes(:,j)))))
            residuals(j)=sqrt(dot_product(r,r))/denom
        end do
    end subroutine compute_modal_residuals
end module fem_generalized_eigen_utils
