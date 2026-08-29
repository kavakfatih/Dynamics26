module fem_finite_strain_kinematics
    !! Finite-strain kinematigi icin reference/current configuration sozlesmesi.
    !!
    !! Total Lagrangian baseline'da tum shape gradientleri reference configuration
    !! uzerinden, dN/dX olarak degerlendirilir. Deformation gradient:
    !!
    !!   F = dx/dX = I + du/dX
    !!
    !! ve Green-Lagrange strain:
    !!
    !!   E = 1/2 (F^T F - I)
    !!
    !! kullanilir. Bu strain olcusu saf rigid-body rotation altinda sifirdir;
    !! V0.7 objectivity testleri bu sozlesmeyi kilitler.
    use fem_kinds, only : rk
    use fem_matrix_math, only : determinant_3x3, identity_matrix_3x3
    use fem_status, only : status_t, FEM_STATUS_INVALID_ARGUMENT, &
                           FEM_STATUS_SIZE_MISMATCH, FEM_STATUS_NUMERICAL_FAILURE
    implicit none
    private

    type, public :: configuration_pair_t
        real(rk), allocatable :: reference(:, :)
        real(rk), allocatable :: current(:, :)
    contains
        procedure :: initialize_from_displacement => configuration_initialize_from_displacement
        procedure :: node_count => configuration_node_count
        procedure :: clear => configuration_clear
    end type configuration_pair_t

    public :: deformation_gradient_from_displacement_gradient
    public :: deformation_gradient_from_coordinates
    public :: right_cauchy_green
    public :: left_cauchy_green
    public :: green_lagrange_strain
    public :: euler_almansi_strain
    public :: second_pk_to_first_pk
    public :: second_pk_to_kirchhoff
    public :: second_pk_to_cauchy

contains

    subroutine configuration_clear(this)
        class(configuration_pair_t), intent(inout) :: this
        if (allocated(this%reference)) deallocate(this%reference)
        if (allocated(this%current)) deallocate(this%current)
    end subroutine configuration_clear

    pure integer function configuration_node_count(this) result(n)
        class(configuration_pair_t), intent(in) :: this
        if (allocated(this%reference)) then
            n = size(this%reference, 2)
        else
            n = 0
        end if
    end function configuration_node_count

    subroutine configuration_initialize_from_displacement(this, reference, displacement, status)
        class(configuration_pair_t), intent(inout) :: this
        real(rk), intent(in) :: reference(:, :), displacement(:, :)
        type(status_t), intent(out) :: status

        call status%clear()
        call this%clear()
        if (size(reference,1) /= 3 .or. size(displacement,1) /= 3 .or. &
            size(reference,2) /= size(displacement,2) .or. size(reference,2) < 1) then
            call status%set_error(FEM_STATUS_SIZE_MISMATCH, &
                "Finite-strain configuration 3 x n reference/displacement gerektirir.")
            return
        end if
        allocate(this%reference(3,size(reference,2)), this%current(3,size(reference,2)))
        this%reference = reference
        this%current = reference + displacement
    end subroutine configuration_initialize_from_displacement

    pure subroutine deformation_gradient_from_displacement_gradient(displacement_gradient, f)
        !! displacement_gradient(i,J) = du_i/dX_J
        real(rk), intent(in) :: displacement_gradient(3,3)
        real(rk), intent(out) :: f(3,3)
        f = identity_matrix_3x3() + displacement_gradient
    end subroutine deformation_gradient_from_displacement_gradient

    subroutine deformation_gradient_from_coordinates(current_coordinates, dshape_dreference, f, j, status)
        !! Isoparametric Total-Lagrangian kimlik:
        !!
        !!   F_iJ = sum_a x_ai * N_a,J
        !!
        !! dN/dX partition-of-unity ve reference mapping ile uyumlu oldugunda
        !! reference configuration'da F=I verir.
        real(rk), intent(in) :: current_coordinates(:, :)
        real(rk), intent(in) :: dshape_dreference(:, :)
        real(rk), intent(out) :: f(3,3)
        real(rk), intent(out) :: j
        type(status_t), intent(out) :: status

        call status%clear()
        f = 0.0_rk
        j = 0.0_rk
        if (size(current_coordinates,1) /= 3 .or. size(dshape_dreference,1) /= 3 .or. &
            size(current_coordinates,2) /= size(dshape_dreference,2)) then
            call status%set_error(FEM_STATUS_SIZE_MISMATCH, &
                "Deformation gradient icin coordinate ve dN/dX boyutlari uyusmuyor.")
            return
        end if
        f = matmul(current_coordinates, transpose(dshape_dreference))
        j = determinant_3x3(f)
        if (j <= 0.0_rk) then
            call status%set_error(FEM_STATUS_NUMERICAL_FAILURE, &
                "Finite-strain deformation gradient J<=0 verdi; current configuration gecersiz/inverted.")
        end if
    end subroutine deformation_gradient_from_coordinates

    pure subroutine right_cauchy_green(f, c)
        real(rk), intent(in) :: f(3,3)
        real(rk), intent(out) :: c(3,3)
        c = matmul(transpose(f), f)
    end subroutine right_cauchy_green

    pure subroutine left_cauchy_green(f, b)
        real(rk), intent(in) :: f(3,3)
        real(rk), intent(out) :: b(3,3)
        b = matmul(f, transpose(f))
    end subroutine left_cauchy_green

    pure subroutine green_lagrange_strain(f, e)
        real(rk), intent(in) :: f(3,3)
        real(rk), intent(out) :: e(3,3)
        real(rk) :: c(3,3)
        call right_cauchy_green(f, c)
        e = 0.5_rk * (c - identity_matrix_3x3())
    end subroutine green_lagrange_strain

    subroutine euler_almansi_strain(f, e, status)
        !! e = 1/2 (I - b^{-1}). Yalnizca stress-measure/post-processing
        !! karsilastirmalari icin sunulur; TL eleman integrasyonu bunu kullanmaz.
        real(rk), intent(in) :: f(3,3)
        real(rk), intent(out) :: e(3,3)
        type(status_t), intent(out) :: status
        real(rk) :: b(3,3), binv(3,3), detb

        call status%clear()
        e = 0.0_rk
        call left_cauchy_green(f, b)
        detb = determinant_3x3(b)
        if (detb <= tiny(1.0_rk)) then
            call status%set_error(FEM_STATUS_NUMERICAL_FAILURE, &
                "Euler-Almansi strain icin left Cauchy-Green singular.")
            return
        end if
        call inverse3(b, detb, binv)
        e = 0.5_rk * (identity_matrix_3x3() - binv)
    end subroutine euler_almansi_strain

    pure subroutine second_pk_to_first_pk(f, second_pk, first_pk)
        real(rk), intent(in) :: f(3,3), second_pk(3,3)
        real(rk), intent(out) :: first_pk(3,3)
        first_pk = matmul(f, second_pk)
    end subroutine second_pk_to_first_pk

    pure subroutine second_pk_to_kirchhoff(f, second_pk, kirchhoff)
        real(rk), intent(in) :: f(3,3), second_pk(3,3)
        real(rk), intent(out) :: kirchhoff(3,3)
        kirchhoff = matmul(f, matmul(second_pk, transpose(f)))
    end subroutine second_pk_to_kirchhoff

    subroutine second_pk_to_cauchy(f, second_pk, cauchy, status)
        real(rk), intent(in) :: f(3,3), second_pk(3,3)
        real(rk), intent(out) :: cauchy(3,3)
        type(status_t), intent(out) :: status
        real(rk) :: j

        call status%clear()
        cauchy = 0.0_rk
        j = determinant_3x3(f)
        if (j <= 0.0_rk) then
            call status%set_error(FEM_STATUS_NUMERICAL_FAILURE, &
                "Cauchy stress push-forward icin J pozitif olmali.")
            return
        end if
        call second_pk_to_kirchhoff(f, second_pk, cauchy)
        cauchy = cauchy / j
    end subroutine second_pk_to_cauchy

    pure subroutine inverse3(a, determinant, inverse)
        real(rk), intent(in) :: a(3,3), determinant
        real(rk), intent(out) :: inverse(3,3)
        inverse(1,1) =  (a(2,2)*a(3,3)-a(2,3)*a(3,2))/determinant
        inverse(1,2) = -(a(1,2)*a(3,3)-a(1,3)*a(3,2))/determinant
        inverse(1,3) =  (a(1,2)*a(2,3)-a(1,3)*a(2,2))/determinant
        inverse(2,1) = -(a(2,1)*a(3,3)-a(2,3)*a(3,1))/determinant
        inverse(2,2) =  (a(1,1)*a(3,3)-a(1,3)*a(3,1))/determinant
        inverse(2,3) = -(a(1,1)*a(2,3)-a(1,3)*a(2,1))/determinant
        inverse(3,1) =  (a(2,1)*a(3,2)-a(2,2)*a(3,1))/determinant
        inverse(3,2) = -(a(1,1)*a(3,2)-a(1,2)*a(3,1))/determinant
        inverse(3,3) =  (a(1,1)*a(2,2)-a(1,2)*a(2,1))/determinant
    end subroutine inverse3

end module fem_finite_strain_kinematics
