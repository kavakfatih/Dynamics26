module fem_nonlinear_loads
    !! Geometric nonlinear analizlerde external load'in hangi configuration'da
    !! tanimlandigini acikca tasiyan metadata sozlesmesi.
    !!
    !! V0.7'de surface pressure/traction integrasyonu henuz uygulanmaz. Ancak
    !! V0.8+ follower load geldiginde residual linearization'in sessizce yanlis
    !! olmasini onlemek icin load configuration ve external tangent gereksinimi
    !! simdiden explicit hale getirilir.
    use fem_status, only : status_t, FEM_STATUS_INVALID_ARGUMENT
    implicit none
    private

    integer,parameter,public :: LOAD_CONFIGURATION_REFERENCE = 1
    integer,parameter,public :: LOAD_CONFIGURATION_CURRENT = 2

    type,public :: nonlinear_load_metadata_t
        integer :: configuration = LOAD_CONFIGURATION_REFERENCE
        logical :: follows_deformation = .false.
        logical :: contributes_external_tangent = .false.
    contains
        procedure :: validate => nonlinear_load_metadata_validate
    end type nonlinear_load_metadata_t

contains

    subroutine nonlinear_load_metadata_validate(this,status)
        class(nonlinear_load_metadata_t),intent(in)::this
        type(status_t),intent(out)::status
        call status%clear()
        if(this%configuration/=LOAD_CONFIGURATION_REFERENCE .and. &
           this%configuration/=LOAD_CONFIGURATION_CURRENT)then
            call status%set_error(FEM_STATUS_INVALID_ARGUMENT,"Nonlinear load configuration gecersiz.")
            return
        end if
        if(this%follows_deformation .and. this%configuration/=LOAD_CONFIGURATION_CURRENT)then
            call status%set_error(FEM_STATUS_INVALID_ARGUMENT, &
                "Follower load current configuration'da tanimlanmalidir.")
            return
        end if
        if(this%follows_deformation .and. .not.this%contributes_external_tangent)then
            call status%set_error(FEM_STATUS_INVALID_ARGUMENT, &
                "Follower load Newton linearization icin external tangent katkisini explicit istemelidir.")
        end if
    end subroutine nonlinear_load_metadata_validate

end module fem_nonlinear_loads
