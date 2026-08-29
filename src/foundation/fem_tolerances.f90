module fem_tolerances
    !! Tek bir "magic epsilon" yerine amaca gore tolerans seti kullanilir.
    !! Bu degerler verification/regression toleranslariyla ayni sey degildir.
    use fem_kinds, only : rk
    implicit none
    private

    type, public :: tolerance_set_t
        real(rk) :: absolute = 1.0e-12_rk
        real(rk) :: relative = 1.0e-10_rk
        real(rk) :: geometry = 1.0e-10_rk
        real(rk) :: singular = 1.0e-14_rk
    end type tolerance_set_t

end module fem_tolerances
