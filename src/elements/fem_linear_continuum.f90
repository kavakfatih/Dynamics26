module fem_linear_continuum
    !! QUAD4 ve HEX8 icin small-strain lineer elastik stiffness ve IP recovery.
    use fem_kinds, only : rk
    use fem_topology, only : TOPOLOGY_QUAD4, TOPOLOGY_HEX8
    use fem_quadrature, only : quadrature_rule_t, standard_quadrature_rule
    use fem_element_kernel, only : element_geometry_point_t, evaluate_geometry_point
    use fem_element_kinematics, only : build_plane_b_matrix, build_solid_b_matrix, build_axisymmetric_b_matrix
    use fem_linear_elastic_material, only : linear_elastic_material_t, constitutive_3d, &
        constitutive_plane_stress, constitutive_plane_strain, constitutive_axisymmetric
    use fem_status, only : status_t, FEM_STATUS_INVALID_ARGUMENT, FEM_STATUS_SIZE_MISMATCH
    implicit none
    private

    integer, parameter, public :: PLANE_MODE_STRESS = 1
    integer, parameter, public :: PLANE_MODE_STRAIN = 2

    public :: quad4_stiffness_plane, quad4_stiffness_axisymmetric, hex8_stiffness
    public :: quad4_recover_plane, quad4_recover_axisymmetric, hex8_recover

contains

    subroutine quad4_stiffness_plane(coords, material, thickness, plane_mode, ke, status)
        real(rk), intent(in) :: coords(2,4)
        type(linear_elastic_material_t), intent(in) :: material
        real(rk), intent(in) :: thickness
        integer, intent(in) :: plane_mode
        real(rk), intent(out) :: ke(8,8)
        type(status_t), intent(out) :: status
        type(quadrature_rule_t) :: rule
        type(element_geometry_point_t) :: gp
        real(rk), allocatable :: b(:,:)
        real(rk) :: d(3,3)
        integer :: p
        call status%clear(); ke=0.0_rk
        if (thickness <= 0.0_rk) then
            call status%set_error(FEM_STATUS_INVALID_ARGUMENT, "QUAD4 thickness pozitif olmali."); return
        end if
        if (plane_mode == PLANE_MODE_STRESS) then
            call constitutive_plane_stress(material,d,status)
        else if (plane_mode == PLANE_MODE_STRAIN) then
            call constitutive_plane_strain(material,d,status)
        else
            call status%set_error(FEM_STATUS_INVALID_ARGUMENT, "Plane mode gecersiz."); return
        end if
        if (.not. status%is_ok()) return
        call standard_quadrature_rule(TOPOLOGY_QUAD4,rule,status)
        if (.not. status%is_ok()) return
        do p=1,rule%point_count
            call evaluate_geometry_point(TOPOLOGY_QUAD4,coords,rule%points(:,p),rule%weights(p),gp,status)
            if (.not. status%is_ok()) return
            call build_plane_b_matrix(gp%dshape_dphysical,b,status)
            if (.not. status%is_ok()) return
            ke = ke + matmul(transpose(b),matmul(d,b)) * gp%integration_measure * thickness
        end do
    end subroutine quad4_stiffness_plane

    subroutine quad4_stiffness_axisymmetric(coords, material, ke, status)
        real(rk), intent(in) :: coords(2,4)
        type(linear_elastic_material_t), intent(in) :: material
        real(rk), intent(out) :: ke(8,8)
        type(status_t), intent(out) :: status
        type(quadrature_rule_t) :: rule
        type(element_geometry_point_t) :: gp
        real(rk), allocatable :: b(:,:)
        real(rk) :: d(4,4)
        integer :: p
        call status%clear(); ke=0.0_rk
        call constitutive_axisymmetric(material,d,status)
        if (.not. status%is_ok()) return
        call standard_quadrature_rule(TOPOLOGY_QUAD4,rule,status)
        if (.not. status%is_ok()) return
        do p=1,rule%point_count
            call evaluate_geometry_point(TOPOLOGY_QUAD4,coords,rule%points(:,p),rule%weights(p),gp,status,.true.)
            if (.not. status%is_ok()) return
            call build_axisymmetric_b_matrix(gp%shape,gp%dshape_dphysical,gp%radius,b,status)
            if (.not. status%is_ok()) return
            ke = ke + matmul(transpose(b),matmul(d,b)) * gp%integration_measure
        end do
    end subroutine quad4_stiffness_axisymmetric

    subroutine hex8_stiffness(coords, material, ke, status)
        real(rk), intent(in) :: coords(3,8)
        type(linear_elastic_material_t), intent(in) :: material
        real(rk), intent(out) :: ke(24,24)
        type(status_t), intent(out) :: status
        type(quadrature_rule_t) :: rule
        type(element_geometry_point_t) :: gp
        real(rk), allocatable :: b(:,:)
        real(rk) :: d(6,6)
        integer :: p
        call status%clear(); ke=0.0_rk
        call constitutive_3d(material,d,status)
        if (.not. status%is_ok()) return
        call standard_quadrature_rule(TOPOLOGY_HEX8,rule,status)
        if (.not. status%is_ok()) return
        do p=1,rule%point_count
            call evaluate_geometry_point(TOPOLOGY_HEX8,coords,rule%points(:,p),rule%weights(p),gp,status)
            if (.not. status%is_ok()) return
            call build_solid_b_matrix(gp%dshape_dphysical,b,status)
            if (.not. status%is_ok()) return
            ke = ke + matmul(transpose(b),matmul(d,b)) * gp%integration_measure
        end do
    end subroutine hex8_stiffness

    subroutine quad4_recover_plane(coords, material, plane_mode, u, strain, stress, points, status, stress_zz)
        real(rk), intent(in) :: coords(2,4), u(8)
        type(linear_elastic_material_t), intent(in) :: material
        integer, intent(in) :: plane_mode
        real(rk), allocatable, intent(out) :: strain(:,:), stress(:,:), points(:,:)
        type(status_t), intent(out) :: status
        real(rk), allocatable, intent(out), optional :: stress_zz(:)
        type(quadrature_rule_t) :: rule
        type(element_geometry_point_t) :: gp
        real(rk), allocatable :: b(:,:)
        real(rk) :: d(3,3)
        integer :: p
        call status%clear()
        if (plane_mode == PLANE_MODE_STRESS) then
            call constitutive_plane_stress(material,d,status)
        else if (plane_mode == PLANE_MODE_STRAIN) then
            call constitutive_plane_strain(material,d,status)
        else
            call status%set_error(FEM_STATUS_INVALID_ARGUMENT,"Plane mode gecersiz."); return
        end if
        if (.not. status%is_ok()) return
        call standard_quadrature_rule(TOPOLOGY_QUAD4,rule,status)
        if (.not. status%is_ok()) return
        allocate(strain(3,rule%point_count),stress(3,rule%point_count),points(2,rule%point_count))
        if (present(stress_zz)) then
            allocate(stress_zz(rule%point_count))
            stress_zz = 0.0_rk
        end if
        do p=1,rule%point_count
            call evaluate_geometry_point(TOPOLOGY_QUAD4,coords,rule%points(:,p),rule%weights(p),gp,status)
            if (.not. status%is_ok()) return
            call build_plane_b_matrix(gp%dshape_dphysical,b,status)
            if (.not. status%is_ok()) return
            strain(:,p)=matmul(b,u)
            stress(:,p)=matmul(d,strain(:,p))
            points(:,p)=gp%physical_coordinate
            if (present(stress_zz) .and. plane_mode == PLANE_MODE_STRAIN) then
                !! Plane strain'de eps_zz=0 olsa da sigma_zz genellikle sifir degildir.
                !! von Mises hesabinda bu out-of-plane stress kaybedilmemelidir.
                stress_zz(p)=material%lame_lambda()*(strain(1,p)+strain(2,p))
            end if
        end do
    end subroutine quad4_recover_plane

    subroutine quad4_recover_axisymmetric(coords, material, u, strain, stress, points, status)
        real(rk), intent(in) :: coords(2,4), u(8)
        type(linear_elastic_material_t), intent(in) :: material
        real(rk), allocatable, intent(out) :: strain(:,:), stress(:,:), points(:,:)
        type(status_t), intent(out) :: status
        type(quadrature_rule_t) :: rule
        type(element_geometry_point_t) :: gp
        real(rk), allocatable :: b(:,:)
        real(rk) :: d(4,4)
        integer :: p
        call status%clear(); call constitutive_axisymmetric(material,d,status)
        if (.not. status%is_ok()) return
        call standard_quadrature_rule(TOPOLOGY_QUAD4,rule,status)
        if (.not. status%is_ok()) return
        allocate(strain(4,rule%point_count),stress(4,rule%point_count),points(2,rule%point_count))
        do p=1,rule%point_count
            call evaluate_geometry_point(TOPOLOGY_QUAD4,coords,rule%points(:,p),rule%weights(p),gp,status,.true.)
            if (.not. status%is_ok()) return
            call build_axisymmetric_b_matrix(gp%shape,gp%dshape_dphysical,gp%radius,b,status)
            if (.not. status%is_ok()) return
            strain(:,p)=matmul(b,u); stress(:,p)=matmul(d,strain(:,p)); points(:,p)=gp%physical_coordinate
        end do
    end subroutine quad4_recover_axisymmetric

    subroutine hex8_recover(coords, material, u, strain, stress, points, status)
        real(rk), intent(in) :: coords(3,8), u(24)
        type(linear_elastic_material_t), intent(in) :: material
        real(rk), allocatable, intent(out) :: strain(:,:), stress(:,:), points(:,:)
        type(status_t), intent(out) :: status
        type(quadrature_rule_t) :: rule
        type(element_geometry_point_t) :: gp
        real(rk), allocatable :: b(:,:)
        real(rk) :: d(6,6)
        integer :: p
        call status%clear(); call constitutive_3d(material,d,status)
        if (.not. status%is_ok()) return
        call standard_quadrature_rule(TOPOLOGY_HEX8,rule,status)
        if (.not. status%is_ok()) return
        allocate(strain(6,rule%point_count),stress(6,rule%point_count),points(3,rule%point_count))
        do p=1,rule%point_count
            call evaluate_geometry_point(TOPOLOGY_HEX8,coords,rule%points(:,p),rule%weights(p),gp,status)
            if (.not. status%is_ok()) return
            call build_solid_b_matrix(gp%dshape_dphysical,b,status)
            if (.not. status%is_ok()) return
            strain(:,p)=matmul(b,u); stress(:,p)=matmul(d,strain(:,p)); points(:,p)=gp%physical_coordinate
        end do
    end subroutine hex8_recover

end module fem_linear_continuum
