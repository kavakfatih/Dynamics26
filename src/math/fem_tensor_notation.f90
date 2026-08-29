module fem_tensor_notation
    !! 3B simetrik tensorler icin proje genelindeki Voigt standardi.
    !!
    !! Sira:
    !!   1 -> xx
    !!   2 -> yy
    !!   3 -> zz
    !!   4 -> xy
    !!   5 -> yz
    !!   6 -> xz
    !!
    !! Stress vectorunde kayma bilesenleri tensorial degerdir (tau_xy).
    !! Strain vectorunde ise engineering shear kullanilir:
    !!
    !!   gamma_xy = 2 * epsilon_xy
    !!
    !! Bu ayrim constitutive tangent implementasyonlarinda kritik oldugu icin
    !! farkli rutinlerle ifade edilir ve unit test ile kilitlenir.
    use fem_kinds, only : rk
    implicit none
    private

    integer, parameter, public :: VOIGT_XX = 1
    integer, parameter, public :: VOIGT_YY = 2
    integer, parameter, public :: VOIGT_ZZ = 3
    integer, parameter, public :: VOIGT_XY = 4
    integer, parameter, public :: VOIGT_YZ = 5
    integer, parameter, public :: VOIGT_XZ = 6

    public :: stress_tensor_to_voigt, stress_voigt_to_tensor
    public :: strain_tensor_to_voigt, strain_voigt_to_tensor
    public :: tensor_trace

contains

    pure subroutine stress_tensor_to_voigt(tensor, voigt)
        real(rk), intent(in) :: tensor(3, 3)
        real(rk), intent(out) :: voigt(6)

        voigt = [tensor(1,1), tensor(2,2), tensor(3,3), &
                 tensor(1,2), tensor(2,3), tensor(1,3)]
    end subroutine stress_tensor_to_voigt

    pure subroutine stress_voigt_to_tensor(voigt, tensor)
        real(rk), intent(in) :: voigt(6)
        real(rk), intent(out) :: tensor(3, 3)

        tensor = 0.0_rk
        tensor(1,1) = voigt(1)
        tensor(2,2) = voigt(2)
        tensor(3,3) = voigt(3)
        tensor(1,2) = voigt(4); tensor(2,1) = voigt(4)
        tensor(2,3) = voigt(5); tensor(3,2) = voigt(5)
        tensor(1,3) = voigt(6); tensor(3,1) = voigt(6)
    end subroutine stress_voigt_to_tensor

    pure subroutine strain_tensor_to_voigt(tensor, voigt)
        real(rk), intent(in) :: tensor(3, 3)
        real(rk), intent(out) :: voigt(6)

        voigt = [tensor(1,1), tensor(2,2), tensor(3,3), &
                 2.0_rk*tensor(1,2), 2.0_rk*tensor(2,3), 2.0_rk*tensor(1,3)]
    end subroutine strain_tensor_to_voigt

    pure subroutine strain_voigt_to_tensor(voigt, tensor)
        real(rk), intent(in) :: voigt(6)
        real(rk), intent(out) :: tensor(3, 3)

        tensor = 0.0_rk
        tensor(1,1) = voigt(1)
        tensor(2,2) = voigt(2)
        tensor(3,3) = voigt(3)
        tensor(1,2) = 0.5_rk*voigt(4); tensor(2,1) = tensor(1,2)
        tensor(2,3) = 0.5_rk*voigt(5); tensor(3,2) = tensor(2,3)
        tensor(1,3) = 0.5_rk*voigt(6); tensor(3,1) = tensor(1,3)
    end subroutine strain_voigt_to_tensor

    pure real(rk) function tensor_trace(tensor) result(value)
        real(rk), intent(in) :: tensor(3, 3)
        value = tensor(1,1) + tensor(2,2) + tensor(3,3)
    end function tensor_trace

end module fem_tensor_notation
