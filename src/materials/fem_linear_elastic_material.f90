module fem_linear_elastic_material
    !! Kucuk sekil degistirme icin izotropik lineer elastik malzeme.
    !! SI birim politikasi: E [Pa], density [kg/m3]. Poisson orani boyutsuzdur.
    use fem_kinds, only : rk, id_kind, index_kind
    use fem_ids, only : INVALID_ID, id_is_valid
    use fem_status, only : status_t, FEM_STATUS_INVALID_ARGUMENT
    implicit none
    private

    integer, parameter :: MATERIAL_NAME_LENGTH = 64

    type, public :: linear_elastic_material_t
        integer(id_kind) :: id = INVALID_ID
        character(len=MATERIAL_NAME_LENGTH) :: name = ""
        real(rk) :: young_modulus = 0.0_rk
        real(rk) :: poisson_ratio = 0.0_rk
        real(rk) :: density = 0.0_rk
    contains
        procedure :: validate => material_validate
        procedure :: shear_modulus => material_shear_modulus
        procedure :: lame_lambda => material_lame_lambda
    end type linear_elastic_material_t

    type, public :: material_registry_t
        type(linear_elastic_material_t), allocatable :: materials(:)
    contains
        procedure :: clear => material_registry_clear
        procedure :: count => material_registry_count
        procedure :: add => material_registry_add
        procedure :: find_position => material_registry_find_position
    end type material_registry_t

    public :: constitutive_3d, constitutive_plane_stress
    public :: constitutive_plane_strain, constitutive_axisymmetric

contains

    subroutine material_validate(this, status)
        class(linear_elastic_material_t), intent(in) :: this
        type(status_t), intent(out) :: status
        call status%clear()
        if (.not. id_is_valid(this%id) .or. len_trim(this%name) == 0) then
            call status%set_error(FEM_STATUS_INVALID_ARGUMENT, "Material ID/name gecersiz.")
            return
        end if
        if (this%young_modulus <= 0.0_rk) then
            call status%set_error(FEM_STATUS_INVALID_ARGUMENT, "Young modulu pozitif olmali.")
            return
        end if
        if (this%poisson_ratio <= -1.0_rk .or. this%poisson_ratio >= 0.5_rk) then
            call status%set_error(FEM_STATUS_INVALID_ARGUMENT, "Poisson orani -1 < nu < 0.5 araliginda olmali.")
            return
        end if
        if (this%density < 0.0_rk) then
            call status%set_error(FEM_STATUS_INVALID_ARGUMENT, "Yogunluk negatif olamaz.")
        end if
    end subroutine material_validate

    pure real(rk) function material_shear_modulus(this) result(g)
        class(linear_elastic_material_t), intent(in) :: this
        g = this%young_modulus / (2.0_rk * (1.0_rk + this%poisson_ratio))
    end function material_shear_modulus

    pure real(rk) function material_lame_lambda(this) result(lambda)
        class(linear_elastic_material_t), intent(in) :: this
        lambda = this%young_modulus * this%poisson_ratio / &
                 ((1.0_rk + this%poisson_ratio) * (1.0_rk - 2.0_rk*this%poisson_ratio))
    end function material_lame_lambda

    subroutine constitutive_3d(material, d, status)
        type(linear_elastic_material_t), intent(in) :: material
        real(rk), intent(out) :: d(6,6)
        type(status_t), intent(out) :: status
        real(rk) :: lambda, g
        integer :: i, j
        call material%validate(status)
        d = 0.0_rk
        if (.not. status%is_ok()) return
        lambda = material%lame_lambda()
        g = material%shear_modulus()
        do i = 1, 3
            do j = 1, 3
                d(i,j) = lambda
            end do
            d(i,i) = lambda + 2.0_rk*g
        end do
        d(4,4) = g
        d(5,5) = g
        d(6,6) = g
    end subroutine constitutive_3d

    subroutine constitutive_plane_stress(material, d, status)
        type(linear_elastic_material_t), intent(in) :: material
        real(rk), intent(out) :: d(3,3)
        type(status_t), intent(out) :: status
        real(rk) :: c, nu
        call material%validate(status)
        d = 0.0_rk
        if (.not. status%is_ok()) return
        nu = material%poisson_ratio
        c = material%young_modulus / (1.0_rk - nu*nu)
        d(1,1) = c;       d(1,2) = c*nu
        d(2,1) = c*nu;    d(2,2) = c
        d(3,3) = material%shear_modulus()
    end subroutine constitutive_plane_stress

    subroutine constitutive_plane_strain(material, d, status)
        type(linear_elastic_material_t), intent(in) :: material
        real(rk), intent(out) :: d(3,3)
        type(status_t), intent(out) :: status
        real(rk) :: lambda, g
        call material%validate(status)
        d = 0.0_rk
        if (.not. status%is_ok()) return
        lambda = material%lame_lambda()
        g = material%shear_modulus()
        d(1,1) = lambda + 2.0_rk*g; d(1,2) = lambda
        d(2,1) = lambda;            d(2,2) = lambda + 2.0_rk*g
        d(3,3) = g
    end subroutine constitutive_plane_strain

    subroutine constitutive_axisymmetric(material, d, status)
        !! Siralama: [rr, zz, gamma_rz, tt].
        type(linear_elastic_material_t), intent(in) :: material
        real(rk), intent(out) :: d(4,4)
        type(status_t), intent(out) :: status
        real(rk) :: lambda, g
        integer :: a, b
        call material%validate(status)
        d = 0.0_rk
        if (.not. status%is_ok()) return
        lambda = material%lame_lambda()
        g = material%shear_modulus()
        do a = 1, 4
            if (a == 3) cycle
            do b = 1, 4
                if (b == 3) cycle
                d(a,b) = lambda
            end do
            d(a,a) = lambda + 2.0_rk*g
        end do
        d(3,3) = g
    end subroutine constitutive_axisymmetric

    subroutine material_registry_clear(this)
        class(material_registry_t), intent(inout) :: this
        if (allocated(this%materials)) deallocate(this%materials)
    end subroutine material_registry_clear

    pure integer(index_kind) function material_registry_count(this)
        class(material_registry_t), intent(in) :: this
        if (allocated(this%materials)) then
            material_registry_count = int(size(this%materials), index_kind)
        else
            material_registry_count = 0_index_kind
        end if
    end function material_registry_count

    pure integer(index_kind) function material_registry_find_position(this, material_id)
        class(material_registry_t), intent(in) :: this
        integer(id_kind), intent(in) :: material_id
        integer :: i
        material_registry_find_position = 0_index_kind
        if (.not. allocated(this%materials)) return
        do i = 1, size(this%materials)
            if (this%materials(i)%id == material_id) then
                material_registry_find_position = int(i, index_kind)
                return
            end if
        end do
    end function material_registry_find_position

    subroutine material_registry_add(this, material, status)
        class(material_registry_t), intent(inout) :: this
        type(linear_elastic_material_t), intent(in) :: material
        type(status_t), intent(out) :: status
        type(linear_elastic_material_t), allocatable :: tmp(:)
        integer :: n
        call material%validate(status)
        if (.not. status%is_ok()) return
        if (this%find_position(material%id) /= 0_index_kind) then
            call status%set_error(FEM_STATUS_INVALID_ARGUMENT, "Duplicate Material ID reddedildi.")
            return
        end if
        if (.not. allocated(this%materials)) then
            allocate(this%materials(1)); n = 0
        else
            n = size(this%materials)
            allocate(tmp(n+1)); tmp(1:n) = this%materials
            call move_alloc(tmp, this%materials)
        end if
        this%materials(n+1) = material
    end subroutine material_registry_add

end module fem_linear_elastic_material
