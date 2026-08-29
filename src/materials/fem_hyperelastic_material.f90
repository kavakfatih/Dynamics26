module fem_hyperelastic_material
    !! V0.9 compressible hyperelastic material-point library.
    !!
    !! Energy split:
    !!   W(F) = W_iso(barred stretches/invariants) + U(J)
    !!   U(J) = 1/2 K (J-1)^2
    !!
    !! Stress measure returned to Total-Lagrangian elements is second
    !! Piola-Kirchhoff stress S. material_tangent is dS_voigt/dE_voigt and
    !! therefore is directly compatible with the project engineering-shear
    !! Voigt convention.
    !!
    !! Neo-Hookean, Mooney-Rivlin and Yeoh use an invariant directional
    !! derivative that is analytic. Ogden uses a spectral analytic derivative;
    !! no production finite-difference tangent is used in this module.
    use fem_kinds, only : rk, id_kind, index_kind
    use fem_ids, only : INVALID_ID, id_is_valid
    use fem_matrix_math, only : determinant_3x3, identity_matrix_3x3
    use fem_tensor_notation, only : strain_voigt_to_tensor, stress_tensor_to_voigt
    use fem_status, only : status_t, FEM_STATUS_INVALID_ARGUMENT, FEM_STATUS_NUMERICAL_FAILURE
    implicit none
    private

    integer, parameter, public :: HYPER_NEO_HOOKEAN = 1
    integer, parameter, public :: HYPER_MOONEY_RIVLIN = 2
    integer, parameter, public :: HYPER_YEOH = 3
    integer, parameter, public :: HYPER_OGDEN = 4
    integer, parameter :: MATERIAL_NAME_LENGTH = 64
    integer, parameter :: OGDEN_MAX_TERMS = 3

    type, public :: hyperelastic_material_t
        integer(id_kind) :: id = INVALID_ID
        character(len=MATERIAL_NAME_LENGTH) :: name = ""
        integer :: model = HYPER_NEO_HOOKEAN
        real(rk) :: density = 0.0_rk
        real(rk) :: bulk_modulus = 0.0_rk
        real(rk) :: c10 = 0.0_rk
        real(rk) :: c01 = 0.0_rk
        real(rk) :: c20 = 0.0_rk
        real(rk) :: c30 = 0.0_rk
        integer :: ogden_term_count = 0
        real(rk) :: ogden_mu(OGDEN_MAX_TERMS) = 0.0_rk
        real(rk) :: ogden_alpha(OGDEN_MAX_TERMS) = 0.0_rk
    contains
        procedure :: validate => hyperelastic_validate
        procedure :: initial_shear_modulus => hyperelastic_initial_shear_modulus
    end type hyperelastic_material_t

    type, public :: hyperelastic_registry_t
        type(hyperelastic_material_t), allocatable :: materials(:)
    contains
        procedure :: clear => hyperelastic_registry_clear
        procedure :: count => hyperelastic_registry_count
        procedure :: add => hyperelastic_registry_add
        procedure :: find_position => hyperelastic_registry_find_position
    end type hyperelastic_registry_t

    public :: hyperelastic_response
    public :: hyperelastic_isochoric_response
    public :: hyperelastic_strain_energy
    public :: hyperelastic_model_name

contains

    pure function hyperelastic_model_name(model) result(name)
        integer, intent(in) :: model
        character(len=32) :: name
        select case(model)
        case(HYPER_NEO_HOOKEAN);  name = "Neo-Hookean"
        case(HYPER_MOONEY_RIVLIN); name = "Mooney-Rivlin"
        case(HYPER_YEOH); name = "Yeoh"
        case(HYPER_OGDEN); name = "Ogden"
        case default; name = "Unknown"
        end select
    end function hyperelastic_model_name

    subroutine hyperelastic_validate(this,status)
        class(hyperelastic_material_t),intent(in)::this
        type(status_t),intent(out)::status
        integer :: p
        real(rk) :: ogden_mu_sum
        call status%clear()
        if(.not.id_is_valid(this%id).or.len_trim(this%name)==0)then
            call status%set_error(FEM_STATUS_INVALID_ARGUMENT,"Hyperelastic material ID/name gecersiz.");return
        end if
        if(this%density<0.0_rk)then
            call status%set_error(FEM_STATUS_INVALID_ARGUMENT,"Hyperelastic density negatif olamaz.");return
        end if
        if(this%bulk_modulus<=0.0_rk)then
            call status%set_error(FEM_STATUS_INVALID_ARGUMENT,"Hyperelastic bulk modulus K pozitif olmali.");return
        end if
        select case(this%model)
        case(HYPER_NEO_HOOKEAN)
            if(this%c10<=0.0_rk)then
                call status%set_error(FEM_STATUS_INVALID_ARGUMENT,"Neo-Hookean C10 pozitif olmali.");return
            end if
        case(HYPER_MOONEY_RIVLIN)
            if(this%c10<0.0_rk.or.this%c01<0.0_rk.or.this%c10+this%c01<=0.0_rk)then
                call status%set_error(FEM_STATUS_INVALID_ARGUMENT,"Mooney-Rivlin C10/C01 negatif olamaz ve toplami pozitif olmali.");return
            end if
        case(HYPER_YEOH)
            if(this%c10<=0.0_rk)then
                call status%set_error(FEM_STATUS_INVALID_ARGUMENT,"Yeoh C10 pozitif olmali.");return
            end if
        case(HYPER_OGDEN)
            if(this%ogden_term_count<1.or.this%ogden_term_count>OGDEN_MAX_TERMS)then
                call status%set_error(FEM_STATUS_INVALID_ARGUMENT,"Ogden term count 1..3 araliginda olmali.");return
            end if
            ogden_mu_sum=0.0_rk
            do p=1,this%ogden_term_count
                if(abs(this%ogden_mu(p))<=sqrt(epsilon(1.0_rk)).or.abs(this%ogden_alpha(p))<=sqrt(epsilon(1.0_rk)))then
                    call status%set_error(FEM_STATUS_INVALID_ARGUMENT,"Ogden her aktif terimde mu ve alpha sifirdan farkli olmali.");return
                end if
                ogden_mu_sum=ogden_mu_sum+this%ogden_mu(p)
            end do
            if(ogden_mu_sum<=0.0_rk)then
                call status%set_error(FEM_STATUS_INVALID_ARGUMENT,"Ogden baslangic kayma modulu sum(mu) pozitif olmali.");return
            end if
        case default
            call status%set_error(FEM_STATUS_INVALID_ARGUMENT,"Taninmayan hyperelastic model.");return
        end select
    end subroutine hyperelastic_validate

    pure real(rk) function hyperelastic_initial_shear_modulus(this) result(g0)
        class(hyperelastic_material_t),intent(in)::this
        select case(this%model)
        case(HYPER_NEO_HOOKEAN); g0=2.0_rk*this%c10
        case(HYPER_MOONEY_RIVLIN); g0=2.0_rk*(this%c10+this%c01)
        case(HYPER_YEOH); g0=2.0_rk*this%c10
        case(HYPER_OGDEN); g0=sum(this%ogden_mu(1:max(1,this%ogden_term_count)))
        case default; g0=0.0_rk
        end select
    end function hyperelastic_initial_shear_modulus

    subroutine hyperelastic_response(material,f,second_pk,material_tangent,strain_energy,status)
        type(hyperelastic_material_t),intent(in)::material
        real(rk),intent(in)::f(3,3)
        real(rk),intent(out)::second_pk(3,3),material_tangent(6,6),strain_energy
        type(status_t),intent(out)::status
        real(rk)::c(3,3),j
        call material%validate(status)
        second_pk=0.0_rk;material_tangent=0.0_rk;strain_energy=0.0_rk
        if(.not.status%is_ok())return
        j=determinant_3x3(f)
        if(j<=0.0_rk)then
            call status%set_error(FEM_STATUS_NUMERICAL_FAILURE,"Hyperelastic response J>0 gerektirir.");return
        end if
        c=matmul(transpose(f),f)
        select case(material%model)
        case(HYPER_NEO_HOOKEAN,HYPER_MOONEY_RIVLIN,HYPER_YEOH)
            call invariant_response(material,c,j,second_pk,material_tangent,strain_energy,status)
        case(HYPER_OGDEN)
            call ogden_response(material,c,j,second_pk,material_tangent,strain_energy,status)
        case default
            call status%set_error(FEM_STATUS_INVALID_ARGUMENT,"Taninmayan hyperelastic response modeli.")
        end select
    end subroutine hyperelastic_response

    subroutine hyperelastic_strain_energy(material,f,energy,status)
        type(hyperelastic_material_t),intent(in)::material
        real(rk),intent(in)::f(3,3)
        real(rk),intent(out)::energy
        type(status_t),intent(out)::status
        real(rk)::s(3,3),d(6,6)
        call hyperelastic_response(material,f,s,d,energy,status)
    end subroutine hyperelastic_strain_energy


    subroutine hyperelastic_isochoric_response(material,f,second_pk_iso,material_tangent_iso,strain_energy_iso,status)
        !! Mixed u-p elemanlar icin yalniz distortional/isochoric response.
        !! V0.9 compressible response'taki U(J)=1/2 K(J-1)^2 parcasi analitik
        !! olarak cikartilir. Boylece pressure unknown volumetrik stress'i ayri
        !! Lagrange/perturbed-Lagrange alani olarak tasiyabilir.
        type(hyperelastic_material_t),intent(in)::material
        real(rk),intent(in)::f(3,3)
        real(rk),intent(out)::second_pk_iso(3,3),material_tangent_iso(6,6),strain_energy_iso
        type(status_t),intent(out)::status
        real(rk)::s_full(3,3),c_full(6,6),w_full
        real(rk)::s_vol(3,3),c_vol(6,6),w_vol

        call hyperelastic_response(material,f,s_full,c_full,w_full,status)
        if(.not.status%is_ok())then
            second_pk_iso=0.0_rk;material_tangent_iso=0.0_rk;strain_energy_iso=0.0_rk
            return
        end if
        call hyperelastic_volumetric_penalty_response(material%bulk_modulus,f,s_vol,c_vol,w_vol,status)
        if(.not.status%is_ok())then
            second_pk_iso=0.0_rk;material_tangent_iso=0.0_rk;strain_energy_iso=0.0_rk
            return
        end if
        second_pk_iso=s_full-s_vol
        material_tangent_iso=c_full-c_vol
        strain_energy_iso=w_full-w_vol
    end subroutine hyperelastic_isochoric_response

    subroutine hyperelastic_volumetric_penalty_response(bulk_modulus,f,second_pk_vol,material_tangent_vol,energy_vol,status)
        real(rk),intent(in)::bulk_modulus,f(3,3)
        real(rk),intent(out)::second_pk_vol(3,3),material_tangent_vol(6,6),energy_vol
        type(status_t),intent(out)::status
        real(rk)::c(3,3),cinv(3,3),j,detc,a,da,dj
        real(rk)::v(6),de(3,3),dcinv(3,3),ds(3,3)
        integer::col
        call status%clear();second_pk_vol=0.0_rk;material_tangent_vol=0.0_rk;energy_vol=0.0_rk
        if(bulk_modulus<=0.0_rk)then
            call status%set_error(FEM_STATUS_INVALID_ARGUMENT,"Volumetric penalty bulk modulus pozitif olmali.");return
        end if
        j=determinant_3x3(f)
        if(j<=0.0_rk)then
            call status%set_error(FEM_STATUS_NUMERICAL_FAILURE,"Volumetric penalty J>0 gerektirir.");return
        end if
        c=matmul(transpose(f),f);call inverse3(c,cinv,detc)
        if(detc<=tiny(1.0_rk))then
            call status%set_error(FEM_STATUS_NUMERICAL_FAILURE,"Volumetric penalty C tensor singular.");return
        end if
        energy_vol=0.5_rk*bulk_modulus*(j-1.0_rk)**2
        a=bulk_modulus*j*(j-1.0_rk)
        second_pk_vol=a*cinv
        do col=1,6
            v=0.0_rk;v(col)=1.0_rk;call strain_voigt_to_tensor(v,de)
            dj=j*sum(cinv*transpose(de))
            da=bulk_modulus*(2.0_rk*j-1.0_rk)*dj
            dcinv=-2.0_rk*matmul(cinv,matmul(de,cinv))
            ds=da*cinv+a*dcinv
            call stress_tensor_to_voigt(ds,material_tangent_vol(:,col))
        end do
    end subroutine hyperelastic_volumetric_penalty_response

    subroutine invariant_response(material,c,j,s,cmat,energy,status)
        type(hyperelastic_material_t),intent(in)::material
        real(rk),intent(in)::c(3,3),j
        real(rk),intent(out)::s(3,3),cmat(6,6),energy
        type(status_t),intent(out)::status
        real(rk)::cinv(3,3),i1,i2,a,b,x,y,z,fx,fy,fxx,fxy,fyy,uj,ujj
        real(rk)::amat(3,3),bmat(3,3),gmat(3,3),unit(3,3),h(3,3),ds(3,3),v(6)
        real(rk)::deta,da_scalar,db_scalar,dj,di1,di2,dx,dy,dfx,dfy,duj
        real(rk)::dacinv(3,3),dAmat(3,3),dBmat(3,3),dGmat(3,3),qmat(3,3),rmat(3,3)
        integer::col
        call status%clear();s=0.0_rk;cmat=0.0_rk;energy=0.0_rk
        call inverse3(c,cinv,deta)
        if(deta<=tiny(1.0_rk))then
            call status%set_error(FEM_STATUS_NUMERICAL_FAILURE,"Hyperelastic C tensor singular.");return
        end if
        unit=identity_matrix_3x3();i1=trace3(c);i2=0.5_rk*(i1*i1-sum(c*transpose(c)))
        a=j**(-2.0_rk/3.0_rk);b=j**(-4.0_rk/3.0_rk);x=a*i1;y=b*i2;z=x-3.0_rk
        call iso_derivatives(material,x,y,fx,fy,fxx,fxy,fyy,energy,status);if(.not.status%is_ok())return
        energy=energy+0.5_rk*material%bulk_modulus*(j-1.0_rk)**2
        uj=material%bulk_modulus*(j-1.0_rk);ujj=material%bulk_modulus
        qmat=unit-(i1/3.0_rk)*cinv;amat=a*qmat
        rmat=i1*unit-c-(2.0_rk*i2/3.0_rk)*cinv;bmat=b*rmat
        gmat=0.5_rk*j*cinv
        s=2.0_rk*(fx*amat+fy*bmat+uj*gmat)
        do col=1,6
            v=0.0_rk;v(col)=1.0_rk
            call strain_voigt_to_tensor(v,h);h=2.0_rk*h ! dC = 2 dE
            di1=trace3(h)
            di2=i1*di1-sum(c*transpose(h))
            dj=0.5_rk*j*sum(cinv*transpose(h))
            dx=a*(di1-(2.0_rk/3.0_rk)*i1*dj/j)
            dy=b*(di2-(4.0_rk/3.0_rk)*i2*dj/j)
            dfx=fxx*dx+fxy*dy;dfy=fxy*dx+fyy*dy;duj=ujj*dj
            dacinv=-matmul(cinv,matmul(h,cinv))
            da_scalar=a*(-2.0_rk/3.0_rk)*dj/j
            db_scalar=b*(-4.0_rk/3.0_rk)*dj/j
            dAmat=da_scalar*qmat+a*(-(di1/3.0_rk)*cinv-(i1/3.0_rk)*dacinv)
            dBmat=db_scalar*rmat+b*(di1*unit-h-(2.0_rk/3.0_rk)*(di2*cinv+i2*dacinv))
            dGmat=0.5_rk*(dj*cinv+j*dacinv)
            ds=2.0_rk*(dfx*amat+fx*dAmat+dfy*bmat+fy*dBmat+duj*gmat+uj*dGmat)
            call stress_tensor_to_voigt(ds,cmat(:,col))
        end do
    end subroutine invariant_response

    subroutine iso_derivatives(material,x,y,fx,fy,fxx,fxy,fyy,energy,status)
        type(hyperelastic_material_t),intent(in)::material
        real(rk),intent(in)::x,y
        real(rk),intent(out)::fx,fy,fxx,fxy,fyy,energy
        type(status_t),intent(out)::status
        real(rk)::z
        call status%clear();fx=0.0_rk;fy=0.0_rk;fxx=0.0_rk;fxy=0.0_rk;fyy=0.0_rk;energy=0.0_rk
        select case(material%model)
        case(HYPER_NEO_HOOKEAN)
            energy=material%c10*(x-3.0_rk);fx=material%c10
        case(HYPER_MOONEY_RIVLIN)
            energy=material%c10*(x-3.0_rk)+material%c01*(y-3.0_rk);fx=material%c10;fy=material%c01
        case(HYPER_YEOH)
            z=x-3.0_rk
            energy=material%c10*z+material%c20*z*z+material%c30*z*z*z
            fx=material%c10+2.0_rk*material%c20*z+3.0_rk*material%c30*z*z
            fxx=2.0_rk*material%c20+6.0_rk*material%c30*z
        case default
            call status%set_error(FEM_STATUS_INVALID_ARGUMENT,"Invariant hyperelastic model bekleniyordu.")
        end select
    end subroutine iso_derivatives

    subroutine ogden_response(material,c,j,s,cmat,energy,status)
        type(hyperelastic_material_t),intent(in)::material
        real(rk),intent(in)::c(3,3),j
        real(rk),intent(out)::s(3,3),cmat(6,6),energy
        type(status_t),intent(out)::status
        real(rk)::eig(3),q(3,3),lam(3),lbar(3),tau(3),spr(3),dsdc(3,3)
        real(rk)::dtaudc(3,3),dqdc(3,3),qmean,djdc,mu,alpha,tauvol
        real(rk)::h(3,3),hp(3,3),dsp(3,3),ds(3,3),v(6),gap,gab
        real(rk)::sumq
        integer::a,b,p,col
        call status%clear();s=0.0_rk;cmat=0.0_rk;energy=0.0_rk
        call symmetric_eigen_3x3(c,eig,q,status);if(.not.status%is_ok())return
        if(minval(eig)<=tiny(1.0_rk))then
            call status%set_error(FEM_STATUS_NUMERICAL_FAILURE,"Ogden principal C eigenvalue pozitif olmali.");return
        end if
        lam=sqrt(eig);lbar=lam*j**(-1.0_rk/3.0_rk);tau=0.0_rk;dtaudc=0.0_rk
        do p=1,material%ogden_term_count
            mu=material%ogden_mu(p);alpha=material%ogden_alpha(p)
            sumq=sum(lbar**alpha);energy=energy+(2.0_rk*mu/(alpha*alpha))*(sumq-3.0_rk)
            qmean=sumq/3.0_rk
            do a=1,3
                tau(a)=tau(a)+(2.0_rk*mu/alpha)*(lbar(a)**alpha-qmean)
            end do
            do b=1,3
                do a=1,3
                    dqdc(a,b)=alpha*lbar(a)**alpha*(merge(1.0_rk/(2.0_rk*eig(a)),0.0_rk,a==b)-1.0_rk/(6.0_rk*eig(b)))
                end do
                qmean=sum(dqdc(:,b))/3.0_rk
                do a=1,3
                    dtaudc(a,b)=dtaudc(a,b)+(2.0_rk*mu/alpha)*(dqdc(a,b)-qmean)
                end do
            end do
        end do
        energy=energy+0.5_rk*material%bulk_modulus*(j-1.0_rk)**2
        tauvol=material%bulk_modulus*j*(j-1.0_rk)
        tau=tau+tauvol
        do b=1,3
            djdc=j/(2.0_rk*eig(b))
            do a=1,3
                dtaudc(a,b)=dtaudc(a,b)+material%bulk_modulus*(2.0_rk*j-1.0_rk)*djdc
            end do
        end do
        do a=1,3
            spr(a)=tau(a)/eig(a)
            do b=1,3
                dsdc(a,b)=dtaudc(a,b)/eig(a)
                if(a==b)dsdc(a,b)=dsdc(a,b)-tau(a)/(eig(a)*eig(a))
            end do
        end do
        s=matmul(q,matmul(diag3(spr),transpose(q)))
        do col=1,6
            v=0.0_rk;v(col)=1.0_rk;call strain_voigt_to_tensor(v,h);h=2.0_rk*h
            hp=matmul(transpose(q),matmul(h,q));dsp=0.0_rk
            do a=1,3
                dsp(a,a)=dot_product(dsdc(a,:),[hp(1,1),hp(2,2),hp(3,3)])
            end do
            do a=1,3
                do b=a+1,3
                    gap=eig(a)-eig(b)
                    if(abs(gap)>1.0e-10_rk*max(1.0_rk,max(abs(eig(a)),abs(eig(b)))))then
                        gab=(spr(a)-spr(b))/gap
                    else
                        gab=dsdc(a,a)-dsdc(a,b)
                    end if
                    dsp(a,b)=gab*hp(a,b);dsp(b,a)=dsp(a,b)
                end do
            end do
            ds=matmul(q,matmul(dsp,transpose(q)))
            call stress_tensor_to_voigt(ds,cmat(:,col))
        end do
    end subroutine ogden_response

    subroutine symmetric_eigen_3x3(a_in,values,vectors,status)
        real(rk),intent(in)::a_in(3,3)
        real(rk),intent(out)::values(3),vectors(3,3)
        type(status_t),intent(out)::status
        real(rk)::a(3,3),app,aqq,apq,tau,t,c,s,aip,aiq,scale,tol
        integer::iter,p,q,i,j,k
        call status%clear();a=0.5_rk*(a_in+transpose(a_in));vectors=identity_matrix_3x3()
        scale=max(1.0_rk,maxval(abs(a)));tol=100.0_rk*epsilon(1.0_rk)*scale
        do iter=1,100
            p=1;q=2;apq=abs(a(1,2))
            if(abs(a(1,3))>apq)then;p=1;q=3;apq=abs(a(1,3));end if
            if(abs(a(2,3))>apq)then;p=2;q=3;apq=abs(a(2,3));end if
            if(apq<=tol)exit
            app=a(p,p);aqq=a(q,q);apq=a(p,q);tau=(aqq-app)/(2.0_rk*apq)
            if(tau>=0.0_rk)then;t=1.0_rk/(tau+sqrt(1.0_rk+tau*tau));else;t=-1.0_rk/(-tau+sqrt(1.0_rk+tau*tau));end if
            c=1.0_rk/sqrt(1.0_rk+t*t);s=t*c
            do i=1,3
                if(i==p.or.i==q)cycle
                aip=a(i,p);aiq=a(i,q);a(i,p)=c*aip-s*aiq;a(p,i)=a(i,p);a(i,q)=s*aip+c*aiq;a(q,i)=a(i,q)
            end do
            a(p,p)=c*c*app-2.0_rk*s*c*apq+s*s*aqq;a(q,q)=s*s*app+2.0_rk*s*c*apq+c*c*aqq;a(p,q)=0.0_rk;a(q,p)=0.0_rk
            do i=1,3
                aip=vectors(i,p);aiq=vectors(i,q);vectors(i,p)=c*aip-s*aiq;vectors(i,q)=s*aip+c*aiq
            end do
        end do
        if(iter>100)then;call status%set_error(FEM_STATUS_NUMERICAL_FAILURE,"3x3 symmetric eigensolver converge olmadi.");return;end if
        values=[a(1,1),a(2,2),a(3,3)]
        do i=1,2
            k=i
            do j=i+1,3;if(values(j)<values(k))k=j;end do
            if(k/=i)call swap_eigenpair(values,vectors,i,k)
        end do
    end subroutine symmetric_eigen_3x3

    subroutine swap_eigenpair(values,vectors,i,j)
        real(rk),intent(inout)::values(3),vectors(3,3)
        integer,intent(in)::i,j
        real(rk)::tmp,col(3)
        tmp=values(i);values(i)=values(j);values(j)=tmp;col=vectors(:,i);vectors(:,i)=vectors(:,j);vectors(:,j)=col
    end subroutine swap_eigenpair

    pure function diag3(v) result(a)
        real(rk),intent(in)::v(3);real(rk)::a(3,3);integer::i
        a=0.0_rk;do i=1,3;a(i,i)=v(i);end do
    end function diag3

    pure real(rk) function trace3(a) result(t)
        real(rk),intent(in)::a(3,3);t=a(1,1)+a(2,2)+a(3,3)
    end function trace3

    pure subroutine inverse3(a,ainv,det)
        real(rk),intent(in)::a(3,3);real(rk),intent(out)::ainv(3,3),det
        det=determinant_3x3(a);ainv=0.0_rk
        if(abs(det)<=tiny(1.0_rk))return
        ainv(1,1)=(a(2,2)*a(3,3)-a(2,3)*a(3,2))/det
        ainv(1,2)=-(a(1,2)*a(3,3)-a(1,3)*a(3,2))/det
        ainv(1,3)=(a(1,2)*a(2,3)-a(1,3)*a(2,2))/det
        ainv(2,1)=-(a(2,1)*a(3,3)-a(2,3)*a(3,1))/det
        ainv(2,2)=(a(1,1)*a(3,3)-a(1,3)*a(3,1))/det
        ainv(2,3)=-(a(1,1)*a(2,3)-a(1,3)*a(2,1))/det
        ainv(3,1)=(a(2,1)*a(3,2)-a(2,2)*a(3,1))/det
        ainv(3,2)=-(a(1,1)*a(3,2)-a(1,2)*a(3,1))/det
        ainv(3,3)=(a(1,1)*a(2,2)-a(1,2)*a(2,1))/det
    end subroutine inverse3

    subroutine hyperelastic_registry_clear(this)
        class(hyperelastic_registry_t),intent(inout)::this
        if(allocated(this%materials))deallocate(this%materials)
    end subroutine hyperelastic_registry_clear
    pure integer(index_kind) function hyperelastic_registry_count(this)
        class(hyperelastic_registry_t),intent(in)::this
        if(allocated(this%materials))then;hyperelastic_registry_count=int(size(this%materials),index_kind);else;hyperelastic_registry_count=0_index_kind;end if
    end function hyperelastic_registry_count
    pure integer(index_kind) function hyperelastic_registry_find_position(this,material_id)
        class(hyperelastic_registry_t),intent(in)::this;integer(id_kind),intent(in)::material_id;integer::i
        hyperelastic_registry_find_position=0_index_kind;if(.not.allocated(this%materials))return
        do i=1,size(this%materials);if(this%materials(i)%id==material_id)then;hyperelastic_registry_find_position=int(i,index_kind);return;end if;end do
    end function hyperelastic_registry_find_position
    subroutine hyperelastic_registry_add(this,material,status)
        class(hyperelastic_registry_t),intent(inout)::this;type(hyperelastic_material_t),intent(in)::material;type(status_t),intent(out)::status
        type(hyperelastic_material_t),allocatable::tmp(:);integer::n
        call material%validate(status);if(.not.status%is_ok())return
        if(this%find_position(material%id)/=0_index_kind)then;call status%set_error(FEM_STATUS_INVALID_ARGUMENT,"Duplicate hyperelastic Material ID reddedildi.");return;end if
        if(.not.allocated(this%materials))then;allocate(this%materials(1));n=0;else;n=size(this%materials);allocate(tmp(n+1));tmp(1:n)=this%materials;call move_alloc(tmp,this%materials);end if
        this%materials(n+1)=material
    end subroutine hyperelastic_registry_add

end module fem_hyperelastic_material
