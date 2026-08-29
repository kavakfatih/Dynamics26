module fem_arpack_eigen_bridge
    !! ARPACK-NG reverse-communication backend. V0.6 baseline, generalized
    !! problem Cholesky ile symmetric standard forma donusturulur; ARPACK matvec
    !! standard operator uzerinde calisir. Sparse shift-invert optimizasyonu daha
    !! sonraki performans hardening kapsamidir.
    use fem_kinds, only : rk,index_kind
    use fem_status, only : status_t,FEM_STATUS_INVALID_ARGUMENT,FEM_STATUS_NUMERICAL_FAILURE
    use fem_generalized_eigen_utils, only : generalized_to_standard,recover_generalized_modes,sort_eigenpairs
    implicit none
    private
    public :: arpack_eigen_available,solve_arpack_generalized_eigen
    interface
        subroutine dsaupd(ido,bmat,n,which,nev,tol,resid,ncv,v,ldv,iparam,ipntr,workd,workl,lworkl,info)
            import rk
            integer,intent(inout)::ido
            character(len=1),intent(in)::bmat
            integer,intent(in)::n,nev,ncv,ldv,lworkl
            character(len=2),intent(in)::which
            real(rk),intent(in)::tol
            real(rk),intent(inout)::resid(*),v(ldv,*),workd(*),workl(*)
            integer,intent(inout)::iparam(*),ipntr(*),info
        end subroutine dsaupd
        subroutine dseupd(rvec,howmny,select,d,z,ldz,sigma,bmat,n,which,nev,tol,resid,ncv,v,ldv,iparam,ipntr,workd,workl,lworkl,info)
            import rk
            logical,intent(in)::rvec
            character(len=1),intent(in)::howmny,bmat
            logical,intent(inout)::select(*)
            integer,intent(in)::ldz,n,nev,ncv,ldv,lworkl
            character(len=2),intent(in)::which
            real(rk),intent(out)::d(*)
            real(rk),intent(inout)::z(ldz,*),resid(*),v(ldv,*),workd(*),workl(*)
            real(rk),intent(in)::sigma,tol
            integer,intent(inout)::iparam(*),ipntr(*),info
        end subroutine dseupd
    end interface
contains
    logical function arpack_eigen_available()
        arpack_eigen_available=.true.
    end function
    subroutine solve_arpack_generalized_eigen(k,m,requested_modes,values,modes,status)
        real(rk),intent(in)::k(:,:),m(:,:); integer,intent(in)::requested_modes
        real(rk),allocatable,intent(out)::values(:),modes(:,:); type(status_t),intent(out)::status
        real(rk),allocatable::a(:,:),linv(:,:),resid(:),v(:,:),workd(:),workl(:),d(:),z(:,:),selected(:,:)
        integer,allocatable::iparam(:),ipntr(:); logical,allocatable::select(:)
        integer::n,nev,ncv,ldv,lworkl,ido,info,i
        integer(index_kind)::xin,xout,nn
        real(rk)::tol,sigma
        character(len=1)::bmat; character(len=2)::which
        call status%clear(); n=size(k,1)
        if (requested_modes<1 .or. requested_modes>=n .or. n<2) then
            allocate(values(0),modes(0,0)); call status%set_error(FEM_STATUS_INVALID_ARGUMENT,"ARPACK icin 1 <= nev < N olmali."); return
        end if
        call generalized_to_standard(k,m,a,linv,status); if (.not.status%is_ok()) then
            allocate(values(0),modes(0,0)); return
        end if
        nev=requested_modes; ncv=min(n,max(nev+2,min(20,2*nev+1)))
        if (ncv<=nev) ncv=nev+1
        ldv=n; lworkl=ncv*(ncv+8)
        allocate(resid(n),v(ldv,ncv),workd(3*n),workl(lworkl),iparam(11),ipntr(11))
        resid=0.0_rk; v=0.0_rk; workd=0.0_rk; workl=0.0_rk; iparam=0; ipntr=0
        ido=0; bmat='I'; which='SA'; tol=0.0_rk; info=0
        iparam(1)=1; iparam(3)=max(300,20*n); iparam(7)=1
        do
            call dsaupd(ido,bmat,n,which,nev,tol,resid,ncv,v,ldv,iparam,ipntr,workd,workl,lworkl,info)
            if (ido==-1 .or. ido==1) then
                xin=int(ipntr(1),index_kind); xout=int(ipntr(2),index_kind); nn=int(n,index_kind)
                workd(xout:xout+nn-1_index_kind)=matmul(a,workd(xin:xin+nn-1_index_kind))
            else
                exit
            end if
        end do
        if (info/=0) then
            allocate(values(0),modes(0,0)); call status%set_error(FEM_STATUS_NUMERICAL_FAILURE,"ARPACK dsaupd convergence/parameter hatasi."); return
        end if
        allocate(select(ncv),d(nev),z(n,nev)); select=.false.; sigma=0.0_rk
        call dseupd(.true.,'A',select,d,z,n,sigma,bmat,n,which,nev,tol,resid,ncv,v,ldv,iparam,ipntr,workd,workl,lworkl,info)
        if (info/=0) then
            allocate(values(0),modes(0,0)); call status%set_error(FEM_STATUS_NUMERICAL_FAILURE,"ARPACK dseupd eigenvector recovery hatasi."); return
        end if
        allocate(values(nev),selected(n,nev)); values=d; selected=z
        call sort_eigenpairs(values,selected)
        do i=1,nev
            if (values(i)<=0.0_rk) then
                call status%set_error(FEM_STATUS_NUMERICAL_FAILURE,"ARPACK pozitif olmayan structural eigenvalue dondurdu."); return
            end if
        end do
        call recover_generalized_modes(linv,selected,m,modes,status)
    end subroutine
end module fem_arpack_eigen_bridge
