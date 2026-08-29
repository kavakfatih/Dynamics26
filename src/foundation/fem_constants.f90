module fem_constants
    !! Solver genelinde kullanilan matematiksel sabitler.
    !!
    !! Birim donusumu burada veya solver'in baska bir yerinde gizli olarak
    !! yapilmaz. FEMCAE cekirdegi yalnizca tutarli birim sistemi varsayar.
    use fem_kinds, only : rk
    implicit none
    private

    real(rk), parameter, public :: FEM_ZERO = 0.0_rk
    real(rk), parameter, public :: FEM_ONE  = 1.0_rk
    real(rk), parameter, public :: FEM_TWO  = 2.0_rk
    real(rk), parameter, public :: FEM_HALF = 0.5_rk
    real(rk), parameter, public :: FEM_PI   = acos(-1.0_rk)

end module fem_constants
