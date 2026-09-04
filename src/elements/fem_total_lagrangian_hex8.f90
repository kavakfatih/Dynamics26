module fem_total_lagrangian_hex8
    !! HEX8 Total-Lagrangian finite-strain element baseline.
    !!
    !! Reference geometry dN/dX ile sabitlenir. Her integration point'te:
    !!   F  -> E -> S
    !!   f_int = integral B_L^T S dV0
    !!   K_T   = K_material + K_geometric
    !!
    !! V0.7'de constitutive law StVK'dir; Newton/load stepping V0.8'e aittir.
    use fem_kinds, only : rk
    use fem_topology, only : TOPOLOGY_HEX8
    use fem_quadrature, only : quadrature_rule_t, standard_quadrature_rule
    use fem_element_kernel, only : element_geometry_point_t, evaluate_geometry_point
    use fem_finite_strain_kinematics, only : deformation_gradient_from_coordinates, &
        green_lagrange_strain, second_pk_to_cauchy
    use fem_stvk_material, only : stvk_response
    use fem_linear_elastic_material, only : linear_elastic_material_t
    use fem_hyperelastic_material, only : hyperelastic_material_t
    use fem_constitutive_interface, only : constitutive_response_t, evaluate_hyperelastic_material_point
    use fem_tensor_notation, only : stress_tensor_to_voigt, strain_tensor_to_voigt
    use fem_nonlinear_contracts, only : NONLINEAR_REASON_NONE, &
        NONLINEAR_REASON_INVALID_REFERENCE_JACOBIAN, &
        NONLINEAR_REASON_INVALID_DEFORMATION_JACOBIAN
    use fem_status, only : status_t, FEM_STATUS_SIZE_MISMATCH, &
        FEM_STATUS_NUMERICAL_FAILURE
    implicit none
    private

    type, public :: total_lagrangian_hex8_result_t
        real(rk) :: internal_force(24) = 0.0_rk
        real(rk) :: tangent(24,24) = 0.0_rk
        real(rk) :: material_tangent(24,24) = 0.0_rk
        real(rk) :: geometric_tangent(24,24) = 0.0_rk
        real(rk) :: strain_energy = 0.0_rk
        real(rk) :: min_j = huge(1.0_rk)
        real(rk), allocatable :: green_lagrange(:, :)
        real(rk), allocatable :: second_pk(:, :)
        real(rk), allocatable :: cauchy(:, :)
        real(rk), allocatable :: deformation_gradient(:, :, :)
        real(rk), allocatable :: j(:)
        integer :: termination_reason = NONLINEAR_REASON_NONE
    end type total_lagrangian_hex8_result_t

    public :: evaluate_total_lagrangian_hex8
    public :: build_green_lagrange_b_matrix

    interface evaluate_total_lagrangian_hex8
        module procedure evaluate_total_lagrangian_hex8_stvk
        module procedure evaluate_total_lagrangian_hex8_hyperelastic
    end interface
    public :: build_geometric_stiffness_block

contains

    subroutine evaluate_total_lagrangian_hex8_stvk(reference_coords, displacement, material, result, status)
        real(rk), intent(in) :: reference_coords(3,8)
        real(rk), intent(in) :: displacement(3,8)
        type(linear_elastic_material_t), intent(in) :: material
        type(total_lagrangian_hex8_result_t), intent(out) :: result
        type(status_t), intent(out) :: status
        type(quadrature_rule_t) :: rule
        type(element_geometry_point_t) :: gp
        real(rk) :: current(3,8), f(3,3), e(3,3), s(3,3), sigma(3,3), cmat(6,6)
        real(rk) :: b(6,24), kg(24,24), evec(6), svec(6), dv0, j
        integer :: p

        call status%clear()
        result = total_lagrangian_hex8_result_t()
        current = reference_coords + displacement
        call standard_quadrature_rule(TOPOLOGY_HEX8, rule, status)
        if (.not. status%is_ok()) return
        allocate(result%green_lagrange(6,rule%point_count), result%second_pk(6,rule%point_count), &
                 result%cauchy(6,rule%point_count), result%deformation_gradient(3,3,rule%point_count), &
                 result%j(rule%point_count))
        result%green_lagrange = 0.0_rk
        result%second_pk = 0.0_rk
        result%cauchy = 0.0_rk
        result%deformation_gradient = 0.0_rk
        result%j = 0.0_rk

        do p = 1, rule%point_count
            call evaluate_geometry_point(TOPOLOGY_HEX8, reference_coords, rule%points(:,p), &
                                         rule%weights(p), gp, status)
            if (.not. status%is_ok()) then
                if (status%code == FEM_STATUS_NUMERICAL_FAILURE) then
                    result%termination_reason = NONLINEAR_REASON_INVALID_REFERENCE_JACOBIAN
                end if
                return
            end if
            call deformation_gradient_from_coordinates(current, gp%dshape_dphysical, f, j, status)
            if (.not. status%is_ok()) then
                if (status%code == FEM_STATUS_NUMERICAL_FAILURE) then
                    result%termination_reason = NONLINEAR_REASON_INVALID_DEFORMATION_JACOBIAN
                end if
                return
            end if
            call green_lagrange_strain(f, e)
            call stvk_response(material, e, s, cmat, status)
            if (.not. status%is_ok()) return
            call second_pk_to_cauchy(f, s, sigma, status)
            if (.not. status%is_ok()) then
                if (status%code == FEM_STATUS_NUMERICAL_FAILURE) then
                    result%termination_reason = NONLINEAR_REASON_INVALID_DEFORMATION_JACOBIAN
                end if
                return
            end if
            call build_green_lagrange_b_matrix(f, gp%dshape_dphysical, b, status)
            if (.not. status%is_ok()) return
            call build_geometric_stiffness_block(s, gp%dshape_dphysical, kg, status)
            if (.not. status%is_ok()) return

            call strain_tensor_to_voigt(e, evec)
            call stress_tensor_to_voigt(s, svec)
            dv0 = gp%integration_measure
            result%internal_force = result%internal_force + matmul(transpose(b), svec) * dv0
            result%material_tangent = result%material_tangent + &
                matmul(transpose(b), matmul(cmat,b)) * dv0
            result%geometric_tangent = result%geometric_tangent + kg * dv0
            result%strain_energy = result%strain_energy + 0.5_rk * dot_product(evec,svec) * dv0
            result%min_j = min(result%min_j, j)
            result%deformation_gradient(:,:,p) = f
            result%j(p) = j
            call strain_tensor_to_voigt(e, result%green_lagrange(:,p))
            call stress_tensor_to_voigt(s, result%second_pk(:,p))
            call stress_tensor_to_voigt(sigma, result%cauchy(:,p))
        end do
        result%tangent = result%material_tangent + result%geometric_tangent
    end subroutine evaluate_total_lagrangian_hex8_stvk

    subroutine evaluate_total_lagrangian_hex8_hyperelastic(reference_coords, displacement, material, result, status)
        real(rk), intent(in) :: reference_coords(3,8)
        real(rk), intent(in) :: displacement(3,8)
        type(hyperelastic_material_t), intent(in) :: material
        type(total_lagrangian_hex8_result_t), intent(out) :: result
        type(status_t), intent(out) :: status
        type(quadrature_rule_t) :: rule
        type(element_geometry_point_t) :: gp
        real(rk) :: current(3,8), f(3,3), e(3,3), s(3,3), sigma(3,3), cmat(6,6)
        real(rk) :: b(6,24), kg(24,24), evec(6), svec(6), dv0, j
        type(constitutive_response_t) :: constitutive
        integer :: p

        call status%clear()
        result = total_lagrangian_hex8_result_t()
        current = reference_coords + displacement
        call standard_quadrature_rule(TOPOLOGY_HEX8, rule, status)
        if (.not. status%is_ok()) return
        allocate(result%green_lagrange(6,rule%point_count), result%second_pk(6,rule%point_count), &
                 result%cauchy(6,rule%point_count), result%deformation_gradient(3,3,rule%point_count), &
                 result%j(rule%point_count))
        result%green_lagrange = 0.0_rk; result%second_pk = 0.0_rk; result%cauchy = 0.0_rk
        result%deformation_gradient = 0.0_rk; result%j = 0.0_rk

        do p = 1, rule%point_count
            call evaluate_geometry_point(TOPOLOGY_HEX8, reference_coords, rule%points(:,p), &
                                         rule%weights(p), gp, status)
            if (.not. status%is_ok()) then
                if (status%code == FEM_STATUS_NUMERICAL_FAILURE) then
                    result%termination_reason = NONLINEAR_REASON_INVALID_REFERENCE_JACOBIAN
                end if
                return
            end if
            call deformation_gradient_from_coordinates(current, gp%dshape_dphysical, f, j, status)
            if (.not. status%is_ok()) then
                if (status%code == FEM_STATUS_NUMERICAL_FAILURE) then
                    result%termination_reason = NONLINEAR_REASON_INVALID_DEFORMATION_JACOBIAN
                end if
                return
            end if
            call green_lagrange_strain(f, e)
            call evaluate_hyperelastic_material_point(material, f, constitutive, status)
            if (.not. status%is_ok()) return
            s = constitutive%stress
            cmat = constitutive%tangent
            call second_pk_to_cauchy(f, s, sigma, status)
            if (.not. status%is_ok()) then
                if (status%code == FEM_STATUS_NUMERICAL_FAILURE) then
                    result%termination_reason = NONLINEAR_REASON_INVALID_DEFORMATION_JACOBIAN
                end if
                return
            end if
            call build_green_lagrange_b_matrix(f, gp%dshape_dphysical, b, status)
            if (.not. status%is_ok()) return
            call build_geometric_stiffness_block(s, gp%dshape_dphysical, kg, status)
            if (.not. status%is_ok()) return

            call strain_tensor_to_voigt(e, evec); call stress_tensor_to_voigt(s, svec)
            dv0 = gp%integration_measure
            result%internal_force = result%internal_force + matmul(transpose(b), svec) * dv0
            result%material_tangent = result%material_tangent + matmul(transpose(b), matmul(cmat,b)) * dv0
            result%geometric_tangent = result%geometric_tangent + kg * dv0
            result%strain_energy = result%strain_energy + constitutive%strain_energy_density * dv0
            result%min_j = min(result%min_j, j)
            result%deformation_gradient(:,:,p) = f; result%j(p) = j
            call strain_tensor_to_voigt(e, result%green_lagrange(:,p))
            call stress_tensor_to_voigt(s, result%second_pk(:,p))
            call stress_tensor_to_voigt(sigma, result%cauchy(:,p))
        end do
        result%tangent = result%material_tangent + result%geometric_tangent
    end subroutine evaluate_total_lagrangian_hex8_hyperelastic

    subroutine build_green_lagrange_b_matrix(f, dshape_dreference, b, status)
        real(rk), intent(in) :: f(3,3)
        real(rk), intent(in) :: dshape_dreference(:, :)
        real(rk), intent(out) :: b(6,24)
        type(status_t), intent(out) :: status
        integer :: a, i, col
        real(rk) :: nx, ny, nz

        call status%clear()
        b = 0.0_rk
        if (size(dshape_dreference,1) /= 3 .or. size(dshape_dreference,2) /= 8) then
            call status%set_error(FEM_STATUS_SIZE_MISMATCH, &
                "HEX8 nonlinear B matrix 3x8 dN/dX gerektirir.")
            return
        end if
        do a = 1, 8
            nx=dshape_dreference(1,a); ny=dshape_dreference(2,a); nz=dshape_dreference(3,a)
            do i = 1, 3
                col = 3*(a-1)+i
                b(1,col) = f(i,1)*nx
                b(2,col) = f(i,2)*ny
                b(3,col) = f(i,3)*nz
                b(4,col) = f(i,1)*ny + f(i,2)*nx
                b(5,col) = f(i,2)*nz + f(i,3)*ny
                b(6,col) = f(i,1)*nz + f(i,3)*nx
            end do
        end do
    end subroutine build_green_lagrange_b_matrix

    subroutine build_geometric_stiffness_block(second_pk, dshape_dreference, kg, status)
        real(rk), intent(in) :: second_pk(3,3)
        real(rk), intent(in) :: dshape_dreference(:, :)
        real(rk), intent(out) :: kg(24,24)
        type(status_t), intent(out) :: status
        integer :: a, bnode, i
        real(rk) :: scalar

        call status%clear()
        kg = 0.0_rk
        if (size(dshape_dreference,1) /= 3 .or. size(dshape_dreference,2) /= 8) then
            call status%set_error(FEM_STATUS_SIZE_MISMATCH, &
                "HEX8 geometric stiffness 3x8 dN/dX gerektirir.")
            return
        end if
        do a=1,8
            do bnode=1,8
                scalar = dot_product(dshape_dreference(:,a), matmul(second_pk,dshape_dreference(:,bnode)))
                do i=1,3
                    kg(3*(a-1)+i,3*(bnode-1)+i)=scalar
                end do
            end do
        end do
    end subroutine build_geometric_stiffness_block

end module fem_total_lagrangian_hex8
