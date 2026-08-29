module fem_constitutive_interface
    !! Ortak material-point constitutive response contract.
    !!
    !! Element kodu malzeme modelinin ayrintisini bilmek yerine hangi stress
    !! measure ve hangi strain/tangent measure ile konustugunu acikca gorur.
    !! V0.9 hyperelastic TL yolu SECOND_PK / dS-dE kullanir. J2 baseline ise
    !! small-strain CAUCHY / dSigma-dEpsilon response verir.
    use fem_kinds, only : rk
    use fem_hyperelastic_material, only : hyperelastic_material_t, hyperelastic_response
    use fem_j2_plasticity, only : j2_material_t, j2_state_t, j2_response_t, j2_update
    use fem_status, only : status_t
    implicit none
    private

    integer, parameter, public :: STRESS_MEASURE_SECOND_PK = 1
    integer, parameter, public :: STRESS_MEASURE_CAUCHY = 2
    integer, parameter, public :: TANGENT_MEASURE_DS_DE = 1
    integer, parameter, public :: TANGENT_MEASURE_DSIGMA_DEPS = 2

    type, public :: constitutive_response_t
        integer :: stress_measure = 0
        integer :: tangent_measure = 0
        real(rk) :: stress(3,3) = 0.0_rk
        real(rk) :: tangent(6,6) = 0.0_rk
        real(rk) :: strain_energy_density = 0.0_rk
        real(rk) :: equivalent_stress = 0.0_rk
        real(rk) :: internal_variable = 0.0_rk
        logical :: stateful = .false.
        logical :: tangent_is_consistent = .false.
    end type constitutive_response_t

    public :: evaluate_hyperelastic_material_point
    public :: evaluate_j2_material_point

contains
    subroutine evaluate_hyperelastic_material_point(material,f,response,status)
        type(hyperelastic_material_t),intent(in)::material
        real(rk),intent(in)::f(3,3)
        type(constitutive_response_t),intent(out)::response
        type(status_t),intent(out)::status
        response=constitutive_response_t()
        call hyperelastic_response(material,f,response%stress,response%tangent,response%strain_energy_density,status)
        if(.not.status%is_ok())return
        response%stress_measure=STRESS_MEASURE_SECOND_PK
        response%tangent_measure=TANGENT_MEASURE_DS_DE
        response%stateful=.false.
        response%tangent_is_consistent=.true.
    end subroutine evaluate_hyperelastic_material_point

    subroutine evaluate_j2_material_point(material,total_strain,state,response,status)
        type(j2_material_t),intent(in)::material
        real(rk),intent(in)::total_strain(3,3)
        type(j2_state_t),intent(inout)::state
        type(constitutive_response_t),intent(out)::response
        type(status_t),intent(out)::status
        type(j2_response_t)::j2
        response=constitutive_response_t()
        call j2_update(material,total_strain,state,j2,status)
        if(.not.status%is_ok())return
        response%stress_measure=STRESS_MEASURE_CAUCHY
        response%tangent_measure=TANGENT_MEASURE_DSIGMA_DEPS
        response%stress=j2%stress;response%tangent=j2%tangent
        response%equivalent_stress=j2%equivalent_stress
        response%internal_variable=state%trial_equivalent_plastic_strain
        response%stateful=.true.;response%tangent_is_consistent=.true.
    end subroutine evaluate_j2_material_point
end module fem_constitutive_interface
