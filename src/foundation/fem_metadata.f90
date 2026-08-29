module fem_metadata
    !! Gelecekteki solver katmanlarinin kritik semantik metadata sabitleri.
    !!
    !! Ozellikle matrisin simetri/definiteness bilgisi lineer solver secimini,
    !! stress measure bilgisi ise finite-strain constitutive arayuzunu etkiler.
    implicit none
    private

    integer, parameter, public :: MATRIX_GENERAL = 1
    integer, parameter, public :: MATRIX_SYMMETRIC = 2
    integer, parameter, public :: MATRIX_SPD = 3
    integer, parameter, public :: MATRIX_SYMMETRIC_INDEFINITE = 4

    integer, parameter, public :: STRESS_CAUCHY = 1
    integer, parameter, public :: STRESS_KIRCHHOFF = 2
    integer, parameter, public :: STRESS_SECOND_PIOLA_KIRCHHOFF = 3
    integer, parameter, public :: STRESS_FIRST_PIOLA_KIRCHHOFF = 4

    integer, parameter, public :: RESULT_NODAL = 1
    integer, parameter, public :: RESULT_INTEGRATION_POINT = 2
    integer, parameter, public :: RESULT_NODAL_EXTRAPOLATED = 3
    integer, parameter, public :: RESULT_NODAL_AVERAGED = 4

end module fem_metadata
