module fem_accelerate_eigen_bridge
    !! Apple Accelerate LAPACK DSYGV backend; K phi = lambda M phi, M SPD.
    use fem_kinds,only:rk
    use fem_status,only:status_t,FEM_STATUS_INVALID_ARGUMENT,FEM_STATUS_NUMERICAL_FAILURE
    use fem_generalized_eigen_utils,only:mass_normalize_modes,sort_eigenpairs
    implicit none; private
    public :: accelerate_eigen_available,solve_accelerate_generalized_eigen
    interface
        subroutine dsygv(itype,jobz,uplo,n,a,lda,b,ldb,w,work,lwork,info)
            import rk
            integer,intent(in)::itype,n,lda,ldb,lwork
            character(len=1),intent(in)::jobz,uplo
            real(rk),intent(inout)::a(lda,*),b(ldb,*),work(*)
            real(rk),intent(out)::w(*)
            integer,intent(out)::info
        end subroutine dsygv
    end interface
contains
    logical function accelerate_eigen_available(); accelerate_eigen_available=.true.; end function
    subroutine solve_accelerate_generalized_eigen(k,m,requested_modes,values,modes,status)
        real(rk),intent(in)::k(:,:),m(:,:); integer,intent(in)::requested_modes
        real(rk),allocatable,intent(out)::values(:),modes(:,:); type(status_t),intent(out)::status
        real(rk),allocatable::a(:,:),b(:,:),w(:),work(:),all_modes(:,:)
        real(rk)::work_query(1); integer::n,info,lwork,first,i,npositive
        call status%clear(); n=size(k,1)
        if (n==0 .or. size(k,2)/=n .or. size(m,1)/=n .or. size(m,2)/=n .or. requested_modes<1 .or. requested_modes>n) then
            allocate(values(0),modes(0,0)); call status%set_error(FEM_STATUS_INVALID_ARGUMENT,"Accelerate eigen K/M veya mode count gecersiz."); return
        end if
        allocate(a(n,n),b(n,n),w(n)); a=k; b=m
        lwork=-1
        call dsygv(1,'V','U',n,a,n,b,n,w,work_query,lwork,info)
        if (info/=0) then
            allocate(values(0),modes(0,0)); call status%set_error(FEM_STATUS_NUMERICAL_FAILURE,"Accelerate DSYGV workspace query basarisiz."); return
        end if
        lwork=max(1,int(work_query(1))); allocate(work(lwork)); a=k; b=m
        call dsygv(1,'V','U',n,a,n,b,n,w,work,lwork,info)
        if (info/=0) then
            allocate(values(0),modes(0,0)); call status%set_error(FEM_STATUS_NUMERICAL_FAILURE,"Accelerate DSYGV factor/eigen solve basarisiz."); return
        end if
        npositive=count(w>1.0e-12_rk*max(1.0_rk,maxval(abs(w))))
        if (npositive<requested_modes) then
            allocate(values(0),modes(0,0)); call status%set_error(FEM_STATUS_NUMERICAL_FAILURE,"Istenen sayida pozitif structural mode bulunamadi."); return
        end if
        first=0
        do i=1,n
            if (w(i)>1.0e-12_rk*max(1.0_rk,maxval(abs(w)))) then; first=i; exit; end if
        end do
        allocate(values(requested_modes),all_modes(n,requested_modes)); values=w(first:first+requested_modes-1); all_modes=a(:,first:first+requested_modes-1)
        call sort_eigenpairs(values,all_modes); call mass_normalize_modes(m,all_modes,status); if (.not.status%is_ok()) return
        call move_alloc(all_modes,modes)
    end subroutine
end module fem_accelerate_eigen_bridge
