module fem_j2_plasticity
    !! V0.9 small-strain J2 von Mises plasticity baseline with linear isotropic
    !! hardening. The state object has explicit committed/trial storage so the
    !! V0.8 rollback contract can be reused by later element integration.
    use fem_kinds, only : rk
    use fem_matrix_math, only : identity_matrix_3x3
    use fem_tensor_notation, only : strain_tensor_to_voigt, strain_voigt_to_tensor, &
        stress_tensor_to_voigt
    use fem_status, only : status_t, FEM_STATUS_INVALID_ARGUMENT, FEM_STATUS_NOT_INITIALIZED
    implicit none
    private

    type, public :: j2_material_t
        real(rk) :: young_modulus = 0.0_rk
        real(rk) :: poisson_ratio = 0.0_rk
        real(rk) :: yield_stress = 0.0_rk
        real(rk) :: isotropic_hardening_modulus = 0.0_rk
    contains
        procedure :: validate => j2_material_validate
    end type j2_material_t

    type, public :: j2_state_t
        real(rk) :: committed_plastic_strain(3,3) = 0.0_rk
        real(rk) :: trial_plastic_strain(3,3) = 0.0_rk
        real(rk) :: committed_equivalent_plastic_strain = 0.0_rk
        real(rk) :: trial_equivalent_plastic_strain = 0.0_rk
        logical :: initialized = .false.
    contains
        procedure :: initialize => j2_state_initialize
        procedure :: begin_trial => j2_state_begin_trial
        procedure :: commit => j2_state_commit
        procedure :: revert => j2_state_revert
    end type j2_state_t

    type, public :: j2_response_t
        real(rk) :: stress(3,3) = 0.0_rk
        real(rk) :: tangent(6,6) = 0.0_rk
        real(rk) :: equivalent_stress = 0.0_rk
        real(rk) :: yield_function = 0.0_rk
        real(rk) :: plastic_multiplier_increment = 0.0_rk
        logical :: plastic = .false.
    end type j2_response_t

    public :: j2_update

contains
    subroutine j2_material_validate(this,status)
        class(j2_material_t),intent(in)::this;type(status_t),intent(out)::status
        call status%clear()
        if(this%young_modulus<=0.0_rk)then;call status%set_error(FEM_STATUS_INVALID_ARGUMENT,"J2 Young modulus pozitif olmali.");return;end if
        if(this%poisson_ratio<=-1.0_rk.or.this%poisson_ratio>=0.5_rk)then;call status%set_error(FEM_STATUS_INVALID_ARGUMENT,"J2 Poisson -1 < nu < 0.5 olmali.");return;end if
        if(this%yield_stress<=0.0_rk.or.this%isotropic_hardening_modulus<0.0_rk)then;call status%set_error(FEM_STATUS_INVALID_ARGUMENT,"J2 yield stress pozitif, hardening modulus negatif olmayan olmali.");return;end if
    end subroutine j2_material_validate

    subroutine j2_state_initialize(this)
        class(j2_state_t),intent(inout)::this
        this%committed_plastic_strain=0.0_rk;this%trial_plastic_strain=0.0_rk
        this%committed_equivalent_plastic_strain=0.0_rk;this%trial_equivalent_plastic_strain=0.0_rk;this%initialized=.true.
    end subroutine j2_state_initialize
    subroutine j2_state_begin_trial(this,status)
        class(j2_state_t),intent(inout)::this;type(status_t),intent(out)::status
        call status%clear();if(.not.this%initialized)then;call status%set_error(FEM_STATUS_NOT_INITIALIZED,"J2 state initialize edilmedi.");return;end if
        this%trial_plastic_strain=this%committed_plastic_strain;this%trial_equivalent_plastic_strain=this%committed_equivalent_plastic_strain
    end subroutine j2_state_begin_trial
    subroutine j2_state_commit(this,status)
        class(j2_state_t),intent(inout)::this;type(status_t),intent(out)::status
        call status%clear();if(.not.this%initialized)then;call status%set_error(FEM_STATUS_NOT_INITIALIZED,"J2 state initialize edilmedi.");return;end if
        this%committed_plastic_strain=this%trial_plastic_strain;this%committed_equivalent_plastic_strain=this%trial_equivalent_plastic_strain
    end subroutine j2_state_commit
    subroutine j2_state_revert(this,status)
        class(j2_state_t),intent(inout)::this;type(status_t),intent(out)::status
        call this%begin_trial(status)
    end subroutine j2_state_revert

    subroutine j2_update(material,total_strain,state,response,status)
        type(j2_material_t),intent(in)::material;real(rk),intent(in)::total_strain(3,3)
        type(j2_state_t),intent(inout)::state;type(j2_response_t),intent(out)::response;type(status_t),intent(out)::status
        real(rk)::g,kbulk,lambda,elastic(3,3),sigma_trial(3,3),s_trial(3,3),nflow(3,3),unit(3,3)
        real(rk)::p,qtrial,yield_current,ftrial,dgamma,snew(3,3),tracee
        real(rk)::hstrain(3,3),dsigma(3,3),dsdev(3,3),dq,dn(3,3),ddgamma,v(6)
        integer::col
        call material%validate(status);response=j2_response_t();if(.not.status%is_ok())return
        if(.not.state%initialized)then;call status%set_error(FEM_STATUS_NOT_INITIALIZED,"J2 state initialize edilmedi.");return;end if
        g=material%young_modulus/(2.0_rk*(1.0_rk+material%poisson_ratio))
        kbulk=material%young_modulus/(3.0_rk*(1.0_rk-2.0_rk*material%poisson_ratio))
        lambda=kbulk-2.0_rk*g/3.0_rk;unit=identity_matrix_3x3()
        elastic=total_strain-state%committed_plastic_strain;tracee=trace3(elastic)
        sigma_trial=lambda*tracee*unit+2.0_rk*g*elastic;p=trace3(sigma_trial)/3.0_rk;s_trial=sigma_trial-p*unit
        qtrial=sqrt(max(0.0_rk,1.5_rk*sum(s_trial*s_trial)));yield_current=material%yield_stress+material%isotropic_hardening_modulus*state%committed_equivalent_plastic_strain
        ftrial=qtrial-yield_current;response%yield_function=ftrial
        if(ftrial<=1.0e-10_rk*max(1.0_rk,yield_current))then
            response%stress=sigma_trial;response%equivalent_stress=qtrial;response%plastic=.false.
            call elastic_tangent(material,response%tangent)
            state%trial_plastic_strain=state%committed_plastic_strain;state%trial_equivalent_plastic_strain=state%committed_equivalent_plastic_strain
            return
        end if
        dgamma=ftrial/(3.0_rk*g+material%isotropic_hardening_modulus)
        nflow=1.5_rk*s_trial/qtrial;snew=s_trial-2.0_rk*g*dgamma*nflow
        response%stress=p*unit+snew;response%equivalent_stress=qtrial-3.0_rk*g*dgamma;response%plastic=.true.;response%plastic_multiplier_increment=dgamma
        state%trial_plastic_strain=state%committed_plastic_strain+dgamma*nflow
        state%trial_equivalent_plastic_strain=state%committed_equivalent_plastic_strain+dgamma
        do col=1,6
            v=0.0_rk;v(col)=1.0_rk;call strain_voigt_to_tensor(v,hstrain)
            dsdev=2.0_rk*g*(hstrain-trace3(hstrain)*unit/3.0_rk)
            dq=sum(nflow*dsdev)
            ddgamma=dq/(3.0_rk*g+material%isotropic_hardening_modulus)
            dn=1.5_rk*dsdev/qtrial-nflow*dq/qtrial
            dsigma=kbulk*trace3(hstrain)*unit+dsdev-2.0_rk*g*(ddgamma*nflow+dgamma*dn)
            call stress_tensor_to_voigt(dsigma,response%tangent(:,col))
        end do
    end subroutine j2_update

    subroutine elastic_tangent(material,d)
        type(j2_material_t),intent(in)::material;real(rk),intent(out)::d(6,6)
        real(rk)::g,lambda;integer::i,j
        g=material%young_modulus/(2.0_rk*(1.0_rk+material%poisson_ratio));lambda=material%young_modulus*material%poisson_ratio/((1.0_rk+material%poisson_ratio)*(1.0_rk-2.0_rk*material%poisson_ratio))
        d=0.0_rk;do i=1,3;do j=1,3;d(i,j)=lambda;end do;d(i,i)=lambda+2.0_rk*g;end do;d(4,4)=g;d(5,5)=g;d(6,6)=g
    end subroutine elastic_tangent
    pure real(rk) function trace3(a) result(t);real(rk),intent(in)::a(3,3);t=a(1,1)+a(2,2)+a(3,3);end function trace3
end module fem_j2_plasticity
