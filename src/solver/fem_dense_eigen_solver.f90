module fem_dense_eigen_solver
    !! Kucuk verification problemleri icin vendor-bagimsiz symmetric generalized eigensolver.
    use fem_kinds, only : rk
    use fem_status, only : status_t, FEM_STATUS_INVALID_ARGUMENT, FEM_STATUS_NUMERICAL_FAILURE
    use fem_generalized_eigen_utils, only : generalized_to_standard,recover_generalized_modes,sort_eigenpairs
    implicit none
    private
    public :: solve_dense_generalized_eigen
contains
    subroutine solve_dense_generalized_eigen(k,m,requested_modes,values,modes,status)
        real(rk), intent(in) :: k(:,:),m(:,:)
        integer, intent(in) :: requested_modes
        real(rk), allocatable, intent(out) :: values(:),modes(:,:)
        type(status_t), intent(out) :: status
        real(rk), allocatable :: a(:,:),linv(:,:),all_values(:),v(:,:),selected(:,:)
        integer :: n,nm,i,valid
        call status%clear(); n=size(k,1)
        if (requested_modes<1 .or. requested_modes>n) then
            allocate(values(0),modes(0,0)); call status%set_error(FEM_STATUS_INVALID_ARGUMENT,"Requested mode count 1..N araliginda olmali."); return
        end if
        call generalized_to_standard(k,m,a,linv,status); if (.not.status%is_ok()) then
            allocate(values(0),modes(0,0)); return
        end if
        call jacobi_symmetric(a,all_values,v,status); if (.not.status%is_ok()) then
            allocate(values(0),modes(0,0)); return
        end if
        call sort_eigenpairs(all_values,v)
        valid=0
        do i=1,n
            if (all_values(i)>1.0e-12_rk*max(1.0_rk,maxval(abs(all_values)))) valid=valid+1
        end do
        if (valid<requested_modes) then
            allocate(values(0),modes(n,0)); call status%set_error(FEM_STATUS_NUMERICAL_FAILURE,"Istenen sayida pozitif structural eigenvalue bulunamadi."); return
        end if
        allocate(values(requested_modes),selected(n,requested_modes))
        nm=0
        do i=1,n
            if (all_values(i)<=1.0e-12_rk*max(1.0_rk,maxval(abs(all_values)))) cycle
            nm=nm+1; values(nm)=all_values(i); selected(:,nm)=v(:,i)
            if (nm==size(values)) exit
        end do
        call recover_generalized_modes(linv,selected,m,modes,status)
    end subroutine solve_dense_generalized_eigen

    subroutine jacobi_symmetric(a_in,values,vectors,status)
        real(rk), intent(in) :: a_in(:,:)
        real(rk), allocatable, intent(out) :: values(:),vectors(:,:)
        type(status_t), intent(out) :: status
        real(rk), allocatable :: a(:,:)
        real(rk) :: app,aqq,apq,tau,t,c,s,aip,aiq,scale,tol
        integer :: n,p,q,i,iter,max_iter
        call status%clear(); n=size(a_in,1)
        allocate(a(n,n),values(n),vectors(n,n)); a=a_in; vectors=0.0_rk
        do i=1,n; vectors(i,i)=1.0_rk; end do
        scale=max(1.0_rk,maxval(abs(a))); tol=100.0_rk*epsilon(1.0_rk)*scale
        max_iter=max(50,50*n*n)
        do iter=1,max_iter
            p=1; q=min(2,n); apq=0.0_rk
            do i=1,n-1
                if (maxval(abs(a(i,i+1:n)))>abs(apq)) then
                    q=i+maxloc(abs(a(i,i+1:n)),dim=1); p=i; apq=a(p,q)
                end if
            end do
            if (abs(apq)<=tol .or. n==1) exit
            app=a(p,p); aqq=a(q,q)
            tau=(aqq-app)/(2.0_rk*apq)
            if (tau>=0.0_rk) then
                t=1.0_rk/(tau+sqrt(1.0_rk+tau*tau))
            else
                t=-1.0_rk/(-tau+sqrt(1.0_rk+tau*tau))
            end if
            c=1.0_rk/sqrt(1.0_rk+t*t); s=t*c
            do i=1,n
                if (i==p .or. i==q) cycle
                aip=a(i,p); aiq=a(i,q)
                a(i,p)=c*aip-s*aiq; a(p,i)=a(i,p)
                a(i,q)=s*aip+c*aiq; a(q,i)=a(i,q)
            end do
            a(p,p)=c*c*app-2.0_rk*s*c*apq+s*s*aqq
            a(q,q)=s*s*app+2.0_rk*s*c*apq+c*c*aqq
            a(p,q)=0.0_rk; a(q,p)=0.0_rk
            do i=1,n
                aip=vectors(i,p); aiq=vectors(i,q)
                vectors(i,p)=c*aip-s*aiq
                vectors(i,q)=s*aip+c*aiq
            end do
        end do
        if (iter>max_iter) then
            call status%set_error(FEM_STATUS_NUMERICAL_FAILURE,"Dense Jacobi eigensolver convergence saglamadi."); return
        end if
        do i=1,n; values(i)=a(i,i); end do
    end subroutine jacobi_symmetric
end module fem_dense_eigen_solver
