module fem_stvk_material
    !! St. Venant-Kirchhoff (StVK) Total-Lagrangian reference material.
    !!
    !!   S = lambda tr(E) I + 2 G E
    !!
    !! Bu model V0.7'de GEOMETRIC NONLINEARITY ve consistent tangent
    !! dogrulamasi icin kullanilir. Buyuk kauçuk gerinimleri icin fiziksel bir
    !! constitutive secim degildir; Neo-Hookean/Mooney/Yeoh/Ogden V0.9 kapsamidir.
    use fem_kinds, only : rk
    use fem_linear_elastic_material, only : linear_elastic_material_t, constitutive_3d
    use fem_tensor_notation, only : strain_tensor_to_voigt, stress_voigt_to_tensor
    use fem_status, only : status_t
    implicit none
    private

    public :: stvk_response

contains

    subroutine stvk_response(material, green_lagrange, second_pk, material_tangent, status)
        type(linear_elastic_material_t), intent(in) :: material
        real(rk), intent(in) :: green_lagrange(3,3)
        real(rk), intent(out) :: second_pk(3,3)
        real(rk), intent(out) :: material_tangent(6,6)
        type(status_t), intent(out) :: status
        real(rk) :: e_voigt(6), s_voigt(6)

        call constitutive_3d(material, material_tangent, status)
        second_pk = 0.0_rk
        if (.not. status%is_ok()) return
        call strain_tensor_to_voigt(green_lagrange, e_voigt)
        s_voigt = matmul(material_tangent, e_voigt)
        call stress_voigt_to_tensor(s_voigt, second_pk)
    end subroutine stvk_response

end module fem_stvk_material
